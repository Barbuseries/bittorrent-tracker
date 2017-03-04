#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "utility.h"
#include "inet_socket.h"
#include "read_line.h"

#define MAX_PEER_COUNT 10
#define MAX_EVENTS 10
#define SERVER_IP "0.0.0.0"

/*
  Write a basic HTTP header in buffer (up until Content-Length):
  
  'HTTP/1.1 <code>
   Server: <SERVER_IP>
   Content-Type: <content_type>'

   Return where it stopped writing.
   
   buffer_size_left is modified to reflect the new space left in
   buffer.
 */
static char *
http_header(char *buffer, int *buffer_size_left, char *code,
			char *content_type)
{
	int total_num_written = 0;
	int size = *buffer_size_left;
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "HTTP/1.1 %s\r\n", code);
				
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Server: %s\r\n", SERVER_IP);
				
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Content-Type: %s\r\n", content_type);

	*buffer_size_left = size;

	return buffer + total_num_written;
}

/*
  Write content in buffer (in HTTP format).
  
  'Content_length: <content_size>

  <content>
  '
  
  If content_size is -1, set content_size to strlen(content).
  
  Return where it stopped writing.
   
  buffer_size_left is modified to reflect the new space left in
  buffer.
 */
static char*
http_content(char *buffer, int *buffer_size_left,
			 char *content, int content_size) {
	int total_num_written = 0;
	int size = *buffer_size_left;

	if (content_size == -1) {
		content_size = strlen(content);
	}

	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Content-Length: %d\r\n", content_size);
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "\r\n");
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  content);
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "\r\n");

	return buffer + total_num_written;
}

static void
handle_torrent_request(int fd)
{
	char buffer[255];
	int read_at_least_one = 0;

	int peer_stopped = 0;

	while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
		if (!read_at_least_one) {
			read_at_least_one = 1;
			
			printf("Client request.\n");
		}
		
		if (strcmp(buffer, "\r\n") == 0)
			break;
		else {
			char *get_request = strstr(buffer, "GET");

			if (get_request && (get_request == buffer)) {
				printf("%s", buffer);

				char *event_state = strstr(get_request, "event");

				if (event_state && strstr(event_state, "stopped")) {
					peer_stopped = 1;
				}
				else {
					printf("Client is following a torrent.\n");
				}
			}
		}
	}

	if (read_at_least_one) {
		if (peer_stopped)
			printf("Client unfollowed a torrent.\n");
		else {
			/* NOTE: A 'no peer' response takes 105 + server_ip_length chars.*/
			char http_response_buffer[255];
			int buffer_size = sizeof(http_response_buffer);
		
			char *response_write_pos = http_response_buffer;

			response_write_pos = http_header(response_write_pos, &buffer_size,
											 "200 Ok", "text/plain");

			// No peers found, ask again in 2 minutes.
			response_write_pos = http_content(response_write_pos, &buffer_size,
											  "d8:intervali120e5:peerslee\r\n", -1);

			write(fd, http_response_buffer, response_write_pos - http_response_buffer);
		}
	} else {
		close(fd);
		
		printf("Client disconnection.\n");
	}
}

int
main()
{
    int listen_fd = inetListen("5555", 5, NULL);
    if (listen_fd == -1)
        errExit("inetListen");

    // Initialize the epoll instance 
    int epfd = epoll_create(MAX_PEER_COUNT + 2);
    if (epfd == -1)
        errExit("epoll_create");

    // event : data can be read
    struct epoll_event ev = { .events = EPOLLIN };

    // add listen_fd to the interest list
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
        errExit("epoll_ctl");

    // add STDIN to the interest list
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1)
        errExit("epoll_ctl");

    // the event list used with epoll_wait()
    struct epoll_event evlist[MAX_EVENTS];

    char x = 0;
    while (x != 'q') {
        int nb_fd_ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);

        if (nb_fd_ready == -1) {
            if (errno == EINTR) // restart if interrupted by signal
                continue;
            else
                errExit("epoll");
        }

        for (int i = 0 ; i < nb_fd_ready ; ++i) {
			int fd = evlist[i].data.fd;

            if (fd == STDIN_FILENO) {
                // Close the server by typing the letter q
                if ((read(STDIN_FILENO, &x, 1) == 1) && (x == 'q'))
                    break;
            } else if (fd == listen_fd) {
                int client_fd = accept(listen_fd, NULL, NULL);

                // The accept system call returns an error
                // this error should be handled more gracefully
                if (client_fd == -1 && errno != EINTR)
                    errExit("accept");

                // There is a new connection
                if (client_fd != -1) {
					ev.data.fd = client_fd;
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
						close(client_fd);
					else
						printf("Client connection.\n");
                }
            } else { // a client fd is ready
				handle_torrent_request(fd);
            }
        }
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
