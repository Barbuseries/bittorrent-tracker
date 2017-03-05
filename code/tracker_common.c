#include "tracker_common.h"

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
