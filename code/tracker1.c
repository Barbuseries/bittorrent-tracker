#include "tracker_common.h"

/* TODO: Remove use of epoll. */

/* Always answer by sending an empty peer list. */
static
REQUEST_HANDLER(handle_request)
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
    int listen_fd = inetListen(TO_STRING(TORRENT_PORT), 5, NULL);
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

	TrackerInfo tracker_info = {};

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
				if (accept_client(listen_fd, epfd, &ev) != -1)
					printf("Client connection.\n");
            } else { // a client fd is ready
				handle_request(&tracker_info, fd);
            }
        }
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
