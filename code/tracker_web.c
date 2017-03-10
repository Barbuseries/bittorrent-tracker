#include "tracker_web.h"

#define BUFFER_SIZE 4096

/* Send a web page containing information about the tracker. */
REQUEST_HANDLER(handle_web_request)
{
  char buffer[255];
  int read_at_least_one = 0;

  int peer_stopped = 0;
  int valid_page = 0;

  while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
    if (!read_at_least_one) {
      read_at_least_one = 1;
			
      printf("Client web request.\n");
    }
		
    if (strcmp(buffer, "\r\n") == 0)
      break;
    else {
      char *get_request = strstr(buffer, "GET / ");
      
      if (!get_request)
        get_request = strstr(buffer, "GET /index.html ");

      if (get_request && (get_request == buffer)) {
        valid_page = 1;
      }
    }
  }

  if (!read_at_least_one) {
    close(fd);
		
    printf("Client web disconnection.\n");
    return;
  }

  if (!valid_page) {
    char http_response_buffer[BUFFER_SIZE + 255];
    int buffer_size = sizeof(http_response_buffer);
		
    char *response_write_pos = http_response_buffer;
    

    response_write_pos = http_header(response_write_pos, &buffer_size,
                                     "404 Not found", "text/html");
    response_write_pos = http_content(response_write_pos, &buffer_size,
                                      "Not found" , -1);

    write(fd, http_response_buffer, response_write_pos - http_response_buffer);
    
    return;
  }

  char http_response_buffer[BUFFER_SIZE + 255];
  int buffer_size = sizeof(http_response_buffer);
		
  char *response_write_pos = http_response_buffer;
    

  response_write_pos = http_header(response_write_pos, &buffer_size,
                                   "200 Ok", "text/html");

  // No peers found, ask again in 2 minutes.
  int singular = tracker_info->torrent_count < 2;
  char content[BUFFER_SIZE];
  char *write_pos = content;
    
  write_pos += snprintf(write_pos , sizeof(content) - (write_pos - content),"<head>" \
                        "<title>Hello, world</title>"	\
                        "</head>"						\
                        "<body>"						\
                        "<h1>bittorrent-tracker</h1>"				\
                        "<p>There %s %d torrent%s.</p>",
                        singular ? "is" : "are",
                        tracker_info->torrent_count,
                        singular ? "" : "s" );

    

  for (int i = 0; i < ARRAY_SIZE(tracker_info->all_torrents); ++i) {
    Torrent *torrent = tracker_info->all_torrents + i;

    if (torrent->hash[0] == '\0')
      continue;
      
    singular = torrent->peer_count < 2;
      
    write_pos += snprintf(write_pos , sizeof(content) - (write_pos - content),
                          "<div>" \
                          "<h2>%.*s</h2>"\
                          "<p>There %s %d peer%s. (S: %d, L: %d)</p>",
                          40, torrent->hash,
                          singular ? "is" : "are",
                          torrent->peer_count,
                          singular ? "" : "s",
                          torrent->seeder_count, torrent->leecher_count);
      
    for (int j = 0; j < ARRAY_SIZE(torrent->all_peers); ++j)
    {
      Peer *peer = torrent->all_peers + j;

      if (peer->id[0] == '\0')
        continue;
      
      write_pos += snprintf(write_pos , sizeof(content) - (write_pos - content),
                            "<div>" \
                            "<h3>%.*s (%s:%hu) </h3>"\
                            "<p>This is a %s.</p>"
                            "</div>",
                            20, peer->id,
                            inet_ntoa(peer->addr_in), peer->port,
                            peer->is_seeder ? "seeder" : "leecher");
    }

    write_pos += snprintf(write_pos , sizeof(content) - (write_pos - content),
                          "</div>");

  }
  write_pos += snprintf(write_pos , sizeof(content) - (write_pos - content),
                        "</body>");
  response_write_pos = http_content(response_write_pos, &buffer_size,
                                    content, -1);

  write(fd, http_response_buffer, response_write_pos - http_response_buffer);
}
