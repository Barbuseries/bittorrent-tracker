#ifndef TRACKER_COMMON_H
#define TRACKER_COMMON_H

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "utility.h"
#include "inet_socket.h"
#include "read_line.h"

#include "http_utility.h"

#define STRINGIFY(x)    #x
#define TO_STRING(x)	STRINGIFY(x)

#define MAX_PEER_COUNT 10
#define MAX_EVENTS     10

#define TORRENT_PORT 5555
#define WEB_PORT     8080

typedef struct Peer {
	char    id[42];				/* Peer's id (as given in request) */
	char    ip[255];			/* Peer's ip (as given in request) */
	int16_t port;				/* Peer's listening port (as given in request) */
	
    int     fd;					/* Tracker file descriptor associated with peer */
	// unsigned long timestamp_last_request;
	// int torrent_index; ?
} Peer;

typedef struct Torrent {
	Peer all_peers[MAX_PEER_COUNT]; /* All peers following this torrent */
	
    int *all_seeders[MAX_PEER_COUNT]; /* Index of peers seeding (finished downloading)  */
	int *all_leechers[MAX_PEER_COUNT]; /* Index of peers leeching (downloading) */

	int  seeder_count;
	int  leecher_count;

	// char hash[255]; /* TODO: Find max hash size. */
} Torrent;

typedef struct TrackerInfo {
	Torrent all_torrents[MAX_PEER_COUNT];
	
	int     torrent_count;
} TrackerInfo;

#define REQUEST_HANDLER(name) void name(TrackerInfo *tracker_info, int fd)
typedef REQUEST_HANDLER(request_handler);

REQUEST_HANDLER(handle_web_request);
REQUEST_HANDLER(handle_torrent_request);

int accept_client(int listen_fd, int epfd, struct epoll_event *ev);

#endif /* TRACKER_COMMON_H */
