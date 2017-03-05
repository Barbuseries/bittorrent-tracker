#include "tracker_common.h"

/* Send a web page containing information about the tracker. */
REQUEST_HANDLER(handle_web_request)
{
	char buffer[255];
	int read_at_least_one = 0;

	int peer_stopped = 0;

	while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
		if (!read_at_least_one) {
			read_at_least_one = 1;
			
			printf("Client web request.\n");
		}
		
		if (strcmp(buffer, "\r\n") == 0)
			break;
		else {
			char *get_request = strstr(buffer, "GET");

			if (get_request && (get_request == buffer)) {
				printf("%s", buffer);
			}
		}
	}

	if (read_at_least_one) {
        /* NOTE: A 'no peer' response takes 105 + server_ip_length chars.*/
		char http_response_buffer[255];
		int buffer_size = sizeof(http_response_buffer);
		
		char *response_write_pos = http_response_buffer;

		response_write_pos = http_header(response_write_pos, &buffer_size,
										 "200 Ok", "text/html");

        // No peers found, ask again in 2 minutes.
		response_write_pos = http_content(response_write_pos, &buffer_size,
										  "<head>"				\
										  "<title>Hello, world</title>"	\
										  "</head>"\
										  "<body>"\
										  "<h1>hello</h1>"\
										  "<p>world</p>"\
										  "</body>\r\n", -1);

		write(fd, http_response_buffer, response_write_pos - http_response_buffer);
	}
	else {
		close(fd);
		
		printf("Client web disconnection.\n");
	}
}

/* Modify tracker information upon peer request. */
REQUEST_HANDLER(handle_torrent_request)
{
	char buffer[255];
	int read_at_least_one = 0;

	int peer_stopped = 0;

	while (readLine(fd, buffer, sizeof(buffer) - 1) > 0) {
		if (!read_at_least_one) {
			read_at_least_one = 1;
			
			printf("Client tracker request.\n");
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
		
		printf("Client tracker disconnection.\n");
	}
}

int
accept_client(int listen_fd, int epfd, struct epoll_event *ev)
{
	int client_fd = accept(listen_fd, NULL, NULL);

	// The accept system call returns an error
	// this error should be handled more gracefully
	// /* TODO:  Handle this error more gracefully! */
	if (client_fd == -1 && errno != EINTR) {
		errExit("accept");
		return -1;
	}

	// There is a new connection
	if (client_fd != -1) {
		ev->data.fd = client_fd;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, ev) == -1) {
			close(client_fd);
			
			return -1;
		}
	}

	return client_fd;
}
