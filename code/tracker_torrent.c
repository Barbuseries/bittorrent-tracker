#include "tracker_torrent.h"

static inline int
same_string(char *a, char *b, size_t len_b)
{
  return ((strlen(a) == len_b) &&
          (strncmp(a, b, len_b) == 0));
}

/* Decode (into DEST) an url-encoded string (SRC). */
static void
url_decode(char *dest, char *src, int src_size)
{
  char a, b;
  char *start = src;

  while (*src &&
         ((src - start) < src_size)) {
    if (*src == '%') {
      ++src;

      char c;
			
      c = *src++;
      *dest++ = c + (((c >= 'A') && (c <= 'Z')) ? 32 : 0);
			
      c = *src++;
      *dest++ = c + (((c >= 'A') && (c <= 'Z')) ? 32 : 0);
    } else
      dest += sprintf(dest, "%x", (unsigned char) *(src++));
  }
	
  *dest++ = '\0';
}

/* Return a pointer to the new torrent on success.
   NULL otherwhise. */
static Torrent *
add_torrent(TrackerInfo *tracker, char *hash)
{
  if (!(tracker->torrent_count < ARRAY_SIZE(tracker->all_torrents)))
    return NULL;

  Torrent *new_torrent = tracker->all_torrents;

  while (new_torrent->hash[0])
    ++new_torrent;
	
  /* TODO: Setting peer count and the first char of each peer
   *       id to 0 is sufficient. */
  memset(new_torrent, 0, sizeof(Torrent));

  memcpy(new_torrent->hash, hash, sizeof(((Torrent *) 0)->hash));

  ++tracker->torrent_count;

  return new_torrent;
}

/* Return a pointer to a torrent on success.
   NULL otherwhise. */
static Torrent *
get_torrent(TrackerInfo *tracker, char *hash)
{
  if (!(tracker->torrent_count > 0))
    return NULL;
	
  for (int i = 0; i < ARRAY_SIZE(tracker->all_torrents); ++i) {
    Torrent *torrent = tracker->all_torrents + i;
		
    if (!torrent->hash[0])
      continue;

    if (strncmp(torrent->hash, hash, sizeof(((Torrent *) 0)->hash)) == 0)
      return torrent;
  }

  return NULL;
}

/* Return a pointer to a peer on success.
   NULL otherwhise. */
static Peer *
get_peer(Torrent *torrent, char *id)
{
  if (!get_peer_count(torrent))
    return NULL;
	
  for (int i = 0; i < ARRAY_SIZE(torrent->all_peers); ++i) {
    Peer *peer = torrent->all_peers + i;

    if (strncmp(peer->id, id, sizeof(((Peer *) 0)->id)) == 0) {
      return peer;
    }
  }

  return NULL;
}

/* Return a pointer to a new peer on success.
   NULL otherwhise. */
static Peer *
add_peer(Torrent *torrent, Peer *new_peer)
{
  if (!(get_peer_count(torrent) < ARRAY_SIZE(torrent->all_peers))) {
    return NULL;
  }

  ASSERT(get_peer(torrent, new_peer->id) == NULL);

  Peer *peer = torrent->all_peers;

  while (peer->id[0])
    ++peer;

  memcpy(peer, new_peer, sizeof(Peer));

  (peer->is_seeder) ? ++torrent->seeder_count : ++torrent->leecher_count;
	
  return peer;
}

/* Return 0 on success.
   1 otherwhise. */
static int
remove_peer(TrackerInfo *tracker, Torrent *torrent, char *peer_id)
{
  if (!get_peer_count(torrent)) 
    return 1;

  Peer *peer = get_peer(torrent, peer_id);

  if (!peer)
    return 1;
	
  peer->id[0] = '\0';
			
  (peer->is_seeder) ? --torrent->seeder_count : --torrent->leecher_count;

  if (!get_peer_count(torrent)) {
    torrent->hash[0] = 0;
    --tracker->torrent_count;
  }
	
  return 0;
}

/* Return 0 on success.
   1 otherwhise. */
static int
update_peer_info(Torrent *torrent, Peer *new_peer_info)
{
  Peer *peer = get_peer(torrent, new_peer_info->id);

  if (!peer)
    return 1;

  if (new_peer_info->is_seeder != peer->is_seeder) {
    if (new_peer_info->is_seeder) {
      --torrent->leecher_count; ++torrent->seeder_count;
    } else {
      --torrent->seeder_count; ++torrent->leecher_count;
    }
  }
	
  memcpy(peer, new_peer_info, sizeof(Peer));

  return 0;
}

/* Send the peer list to the asking peer.
   If the asking peer is a seeder, ignores all other seeders. */
static void
send_peer_list(int fd, Torrent *torrent, Peer *asking_peer, int compact)
{
  int peer_count = get_peer_count(torrent);

#if DISABLE_COMPACT_PEER_LIST
  compact = 0;
#endif

  /* TODO: Both of these could be collapsed into a shorter
   *       version. */
  if (!compact) {
    char peer_list[peer_count * MAX_PEER_BENCODE_LEN];
		
    char *peer_list_write_pos = peer_list;
    *(peer_list_write_pos++) = 'l';

    for (int i = 0; i < ARRAY_SIZE(torrent->all_peers); ++i) {
      Peer *peer = torrent->all_peers + i;

      if (!peer->id[0])
        continue;

      if (peer->is_seeder && asking_peer->is_seeder)
        continue;

      if (strncmp(peer->id, asking_peer->id, 20) == 0)
        continue;

      char *ip_addr = inet_ntoa(peer->addr_in);

      peer_list_write_pos += snprintf(peer_list_write_pos,
                                      MAX_PEER_BENCODE_LEN,
                                      "d"							\
                                      "2:id20:%.*s"				\
                                      "2:ip%d:%s"					\
                                      "4:porti%hue"				\
                                      "e",
                                      20, peer->id,
                                      strlen(ip_addr), ip_addr,
                                      peer->port);
    }

    *(peer_list_write_pos++) = 'e';
    *(peer_list_write_pos++) = '\0';

    char full_tracker_response[128 + sizeof(peer_list)];
    int full_tracker_response_size = snprintf(full_tracker_response, sizeof(full_tracker_response),
                                              "d"					\
                                              "8:intervali"			\
                                              TO_STRING(MIN_REQUEST_INTERVAL) \
                                              "e"					\
                                              "5:peers%se",
                                              peer_list);

    char http_response_buffer[128 + sizeof(full_tracker_response)];
    int  buffer_size = sizeof(http_response_buffer);

    char *response_write_pos = http_response_buffer;

    response_write_pos = http_header(response_write_pos, &buffer_size,
                                     "200 OK", "text/plain");
    response_write_pos = http_content(response_write_pos, &buffer_size,
                                      full_tracker_response, full_tracker_response_size);

    write(fd, http_response_buffer, response_write_pos - http_response_buffer);
  } else {
    /* TODO: This does not seem to work! */
    char peer_list[peer_count * PEER_COMPACT_BENCODE_LEN];
    int actual_peer_list_count = 0;
		
    char *peer_list_write_pos = peer_list;
		
    for (int i = 0; i < ARRAY_SIZE(torrent->all_peers); ++i) {
      Peer *peer = torrent->all_peers + i;
			
      if (!peer->id[0])
        continue;

      if (peer->is_seeder && asking_peer->is_seeder)
        continue;

      if (strncmp(peer->id, asking_peer->id, 20) == 0)
        continue;

      /* Ip address as 4 bytes */
      memcpy(peer_list_write_pos, &peer->addr_in.s_addr, 4);
      peer_list_write_pos += 4;

      /* Port as 2 bytes */
      uint16_t n_port = htons(peer->port);
      memcpy(peer_list_write_pos, &n_port, 2);
      peer_list_write_pos += 2;

      ++actual_peer_list_count;
    }

    char full_tracker_response[30 + sizeof(peer_list)];
    int body_size = 26 + 6 * peer_count;
    int full_tracker_response_size = snprintf(full_tracker_response, sizeof(full_tracker_response),
                                              "d8:intervali"		\
                                              TO_STRING(MIN_REQUEST_INTERVAL) \
                                              "e5:peers%d:",
                                              actual_peer_list_count * PEER_COMPACT_BENCODE_LEN,
                                              peer_list);
		
    memcpy(full_tracker_response + full_tracker_response_size,
           peer_list, peer_list_write_pos - peer_list);

    full_tracker_response_size += peer_list_write_pos - peer_list;

    *(full_tracker_response + full_tracker_response_size++) = 'e';

    char http_response_buffer[128 + sizeof(full_tracker_response)];
    int  buffer_size = sizeof(http_response_buffer);

    char *response_write_pos = http_response_buffer;

    response_write_pos = http_header(response_write_pos, &buffer_size,
                                     "200 OK", "text/plain");

    response_write_pos += sprintf(response_write_pos,
                                  "Content-Length: %d\r\n"	\
                                  "\r\n",
                                  body_size);

    memcpy(response_write_pos, full_tracker_response, body_size);
    response_write_pos += body_size;
    response_write_pos += sprintf(response_write_pos, "\r\n");

    write(fd, http_response_buffer, response_write_pos - http_response_buffer);
  }
}

static void
send_peer_failure(int fd, char *key, char *value)
{
  char http_response_buffer[512];
  int buffer_size = sizeof(http_response_buffer);
		
  char *response_write_pos = http_response_buffer;

  response_write_pos = http_header(response_write_pos, &buffer_size,
                                   "200 OK", "text/plain");

  char buffer[255];
  int num_written = snprintf(buffer, sizeof(buffer), "d%d:%s%d:%s.e\r\n",
                             strlen(key), key, strlen(value), value);
	
  response_write_pos = http_content(response_write_pos, &buffer_size,
                                    buffer, num_written);

  write(fd, http_response_buffer, response_write_pos - http_response_buffer);
}

static inline void
send_peer_failure_reason(int fd, char *reason)
{
  return send_peer_failure(fd, "failure reason", reason);
}

static inline void
send_peer_failure_code(int fd, char *code)
{
  return send_peer_failure(fd, "failure code", code);
}


/* Return 1 if something was read.
   0 otherwhise.
   Parameters (except fd) are zeroed-out. */
/* TODO: Check id and info_hash length (return negative value if
 * invalid) */
static int
parse_peer_request(int fd, PeerEventType *peer_event_type,
                   char *torrent_hash, Peer *peer_info,
                   int *is_compact)
{
  char buffer[255];
  int read_at_least_one = 0;

  /* Zero-out peer_info */
  *peer_event_type = PEER_EVENT_NONE;
  torrent_hash[0] = '\0';
  *is_compact = 0;
  memset(peer_info, 0, sizeof(Peer));

  while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
    read_at_least_one = 1;
		
    if (strcmp(buffer, "\r\n") == 0)
      break;
    else {
      char *get_request = strstr(buffer, "GET /?");
			
      if (get_request && (get_request == buffer)) {
        printf("%s", buffer);

        get_request += 6; /* Skip 'GET /?' */

        char *event_state = strstr(get_request, "event");

        if (event_state) {
          if (strstr(event_state, "started"))
            *peer_event_type = PEER_EVENT_STARTED;
          else  if (strstr(event_state, "stopped"))
            *peer_event_type = PEER_EVENT_STOPPED;
          else  if (strstr(event_state, "completed")) {
            *peer_event_type = PEER_EVENT_COMPLETED;
            peer_info->is_seeder = 1;
          }
        }
				
        char *current_key = get_request;

        /* Parse 'key=value' as current_key and current_val */
        do {
          char *current_val = strchr(current_key, '=');
          ++current_val;

          int  len_key = (current_val - current_key) - 1;
					
          char *next_key = strchr(current_val, '&');
          int  len_val = 0;
					
          if (next_key) {
            ++next_key;
            len_val = (next_key - current_val) - 1;
          } else
            len_val = (strchr(current_val, ' ') - current_val);

          if (same_string("info_hash", current_key, len_key))
            url_decode(torrent_hash, current_val, len_val);
          else if (same_string("peer_id", current_key, len_key)) {
            size_t actual_len = MIN(sizeof(((Peer *) 0)->id), len_val + 1);
						
            memcpy(peer_info->id, current_val, actual_len);
          } else if (same_string("port", current_key, len_key)) {
            char   port_buffer[6];

            size_t actual_len = MIN(sizeof(port_buffer), len_val + 1);
            snprintf(port_buffer, actual_len, "%.*s", actual_len, current_val);

            peer_info->port = atoi(port_buffer);
          } else if (same_string("left", current_key, len_key)) {
            if (len_val && ((current_val[0]) == '0'))
              peer_info->is_seeder = 1;
          } else if (same_string("compact", current_key, len_key)) {
            if (len_val && ((current_val[0]) == '1'))
              *is_compact = 1;
          }
					 
          current_key = next_key;
        } while (current_key);
      }
    }
  }

  return read_at_least_one;
}

REQUEST_HANDLER(handle_torrent_request)
{
  PeerEventType peer_event_type;
  char hash_buffer[40];
  Peer peer_info;
  int  is_compact;

  memcpy(&peer_info.addr_in, addr_in, sizeof(struct in_addr));
	
  int status = parse_peer_request(fd, &peer_event_type,
                                  hash_buffer, &peer_info,
                                  &is_compact);

  /* NOTE: Depending on the client program used and weither it's
     in CLI or GUI mode, EOF may signify that the peer
     disconnected (transmission-cli) or not (bittorrent)...
  */
  if (status == 0) {
    close(fd);
		
    printf("Client tracker disconnection.\n");
		
    return;
  }

  if (status != 1) {
    send_peer_failure_reason(fd, "Unknown error.");
		
    printf("An unknown error occured.\n");
    return;
  }
	
  printf("Client tracker request.\n");

  if (!hash_buffer[0]) {
    send_peer_failure_code(fd, "101");
    return;
  }

  if (!peer_info.id[0]) {
    send_peer_failure_code(fd, "102");
    return;
  }

  if (!peer_info.port) {
    send_peer_failure_code(fd, "103");
    return;
  }

  Torrent *torrent = get_torrent(tracker_info, hash_buffer);

  if ((peer_event_type != PEER_EVENT_STARTED) &&
      (torrent == NULL)) {
    send_peer_failure_code(fd, "200");
    return;
  }

  switch (peer_event_type) {
    case PEER_EVENT_NONE:
      printf("Client eventless request.\n");

      update_peer_info(torrent, &peer_info);
			
      break;
    case PEER_EVENT_STARTED:
      printf("Client started following a torrent.\n");

      if (torrent == NULL)
        torrent = add_torrent(tracker_info, hash_buffer);

      if (torrent == NULL) {
        send_peer_failure_reason(fd, "Torrent count exceeded for this tracker");
        return;
      }

      if (add_peer(torrent, &peer_info) == NULL) {
        send_peer_failure_reason(fd, "Peer count exceeded for this torrent");
        return;
      }

      break;
    case PEER_EVENT_STOPPED:
      printf("Client stopped following a torrent.\n");

      if (remove_peer(tracker_info, torrent, peer_info.id) != 0)
        printf("Unknown peer removed...\n");

      /* It seems the peer repeats (once) the 'stopped' event if
       * it's not explicitly acknowledged by the tracker. */
      http_send_code(fd, "200 OK");
			
      close(fd);
			
      break;
    case PEER_EVENT_COMPLETED:
      printf("Client completed a torrent.\n");

      update_peer_info(torrent, &peer_info);
      break;
  }

  if (peer_event_type != PEER_EVENT_STOPPED)
    send_peer_list(fd, torrent, &peer_info, is_compact);
}
