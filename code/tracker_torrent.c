#include "tracker_torrent.h"

static inline int
same_string(char *a, char *b, size_t len_b)
{
	return ((strlen(a) == len_b) &&
			(strncmp(a, b, len_b) == 0));
}

static void
url_decode(char *dest, char *src, int src_size)
{
	char a, b;
	char *start = src;

	while (*src &&
		   ((src - start) < src_size)) {
		if (*src == '%') {
			++src;
			
			*(dest++) = *(src++);
			*(dest++) = *(src++);
		} else
			dest += sprintf(dest, "%x", (unsigned char) *(src++));
	}
	
	*dest++ = '\0';
}

static Torrent *
add_torrent(TrackerInfo *tracker, char *hash)
{
	if (!(tracker->torrent_count < ARRAY_SIZE(tracker->all_torrents)))
		return NULL;

	Torrent *new_torrent = tracker->all_torrents + tracker->torrent_count++;
	memset(new_torrent, 0, sizeof(Torrent));
	
	snprintf(new_torrent->hash, sizeof(((Torrent *) 0)->hash), "%s", hash);

	return new_torrent;
}

static Torrent *
get_torrent(TrackerInfo *tracker, char *hash)
{
	for (int i = 0; i < ARRAY_SIZE(tracker->all_torrents); ++i)
	{
		Torrent *torrent = tracker->all_torrents + i;
		
		if (!torrent->hash[0])
			continue;

		if (strncmp(torrent->hash, hash, sizeof(((Torrent *) 0)->hash)) == 0)
			return torrent;
	}

	return NULL;
}

static void *
send_peer_list(Torrent *torrent, Peer *peer, int fd) {
	/* NOTE: A 'no peer' response takes 105 + server_ip_length chars.*/
	char http_response_buffer[255];
	int buffer_size = sizeof(http_response_buffer);
		
	char *response_write_pos = http_response_buffer;

	response_write_pos = http_header(response_write_pos, &buffer_size,
									 "200 Ok", "text/plain");

	// No peers found, ask again in 120 seconds.
	response_write_pos = http_content(response_write_pos, &buffer_size,
									  "d8:intervali120e5:peerslee\r\n", -1);

	write(fd, http_response_buffer, response_write_pos - http_response_buffer);
}

static void *
send_peer_failure(int fd, char *reason) {
	char http_response_buffer[512];
	int buffer_size = sizeof(http_response_buffer);
		
	char *response_write_pos = http_response_buffer;

	response_write_pos = http_header(response_write_pos, &buffer_size,
									 "200 OK", "text/plain");

	char buffer[255];
	int num_written = snprintf(buffer, sizeof(buffer), "d14:failure reason%d:%s.e\r\n",
							   strlen(reason), reason);
	
	response_write_pos = http_content(response_write_pos, &buffer_size,
									  buffer, num_written);

	write(fd, http_response_buffer, response_write_pos - http_response_buffer);
}

static int
parse_peer_request(int fd, PeerEventType *peer_event_type,
				   char *torrent_hash, Peer *peer_info,
				   int *is_seeder) {
	char buffer[255];
	int read_at_least_one = 0;

	*peer_event_type = PEER_EVENT_NONE;
	torrent_hash[0] = '\0';
	*is_seeder = 0;
	memset(peer_info, 0, sizeof(Peer));

	while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
		read_at_least_one = 1;
		
		if (strcmp(buffer, "\r\n") == 0)
			break;
		else {
			char *get_request = strstr(buffer, "GET /?");
			
			if (get_request && (get_request == buffer)) {
				printf("%s", buffer);

				get_request += 6; /* Skip 'GER /?' */

				char *event_state = strstr(get_request, "event");

				if (event_state) {
					if (strstr(event_state, "started")) {
						*peer_event_type = PEER_EVENT_STARTED;
					} else  if (strstr(event_state, "stopped")) {
						*peer_event_type = PEER_EVENT_STOPPED;
					} else  if (strstr(event_state, "completed")) {
						*peer_event_type = PEER_EVENT_COMPLETED;
						*is_seeder = 1;
					}
				}
				
				char *left_to_read = get_request;
				
				do
				{
					char *current_val = strchr(left_to_read, '=');
					++current_val;

					int len_key = (current_val - left_to_read) - 1;

					
					char *next_key = strchr(current_val, '&');
					int len_val = 0;
					
					if (next_key) {
						++next_key;
						len_val = (next_key - current_val) - 1;
					}
					else
						len_val = (strchr(current_val, ' ') - current_val);

					if (same_string("info_hash", left_to_read, len_key)) {
						url_decode(torrent_hash, current_val, len_val);
					}
					else if (same_string("peer_id", left_to_read, len_key)) {
						size_t actual_len = MIN(sizeof(((Peer *) 0)->id), len_val + 1);
						snprintf(peer_info->id, actual_len, "%s", current_val);
					}
					else if (same_string("port", left_to_read, len_key)) {
						char port_buffer[6];
						size_t actual_len = MIN(sizeof(port_buffer), len_val + 1);
						snprintf(port_buffer, actual_len, "%s", current_val);

						peer_info->port = atoi(port_buffer);
					}
					else if (same_string("left", left_to_read, len_key)) {
						if (len_val) {
							if ((current_val[0]) == '0') {
								*is_seeder = 1;
							}
						}
					}
					 
					left_to_read = next_key;
				} while (left_to_read);
			}
		}
	}

	return read_at_least_one;
}

/* Modify tracker information upon peer request. */
REQUEST_HANDLER(handle_torrent_request)
{
	PeerEventType peer_event_type;
	char hash_buffer[40];
	Peer peer_info;
	int  is_seeder;
	
	int status = parse_peer_request(fd, &peer_event_type,
									hash_buffer, &peer_info,
									&is_seeder);

	if (status == 0) {
		close(fd);
		printf("Client tracker disconnection.\n");
		
		return;
	}

	if (status != 1) {
		printf("An unknown error occured.\n");
		return;
	}
	
	printf("Client tracker request.\n");

	Torrent *torrent = get_torrent(tracker_info, hash_buffer);
	Peer *peer = NULL;

	if (!hash_buffer[0]) {
		/* TODO: Send info_hash missing (101) */
	}

	if (!peer_info.id[0]) {
		/* TODO: Send peer_id missing (102) */
	}

	if (!peer_info.port) {
		/* TODO: Send port missing (103) */
	}

	if ((peer_event_type != PEER_EVENT_STARTED) &&
		(torrent == NULL)) {
		/* TODO: Send info_hash not found (200) */
	}

	printf("torrent_hash: %.*s\n", 40, hash_buffer);
	printf("peer_id: %.*s\n", 20, peer_info.id);
	printf("peer_port: %hu\n", peer_info.port);
	printf("peer_is_seeder: %d\n", is_seeder);

	switch (peer_event_type) {
		case PEER_EVENT_NONE:
			printf("Client eventless request.\n");
			break;
		case PEER_EVENT_STARTED:
			printf("Client started following a torrent.\n");

			if (torrent == NULL)
				torrent = add_torrent(tracker_info, hash_buffer);

			if (torrent == NULL) {
				send_peer_failure(fd, "Torrent count exceeded for this tracker");
				return;
			}

			/* TODO: Update torrent peer list. */
			
			if (peer == NULL) {
				/* send_peer_failure(fd, "Peer count exceeded for this torrent"); */
				/* return; */
			}
			break;
		case PEER_EVENT_STOPPED:
			printf("Client stopped following a torrent.\n");

			/* TODO: Update torrent peer list. */
			close(fd);
			
			break;
		case PEER_EVENT_COMPLETED:
			printf("Client completed a torrent.\n");

			/* TODO: Update torrent peer list. */
			break;
	}

	if (peer_event_type != PEER_EVENT_STOPPED) {
		send_peer_list(torrent, &peer_info, fd);
	}
}
