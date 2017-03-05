#include "tracker_common.h"

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
				handle_torrent_request(&tracker_info, fd);
            }
        }
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
