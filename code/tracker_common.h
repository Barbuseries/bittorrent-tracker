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

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_PEER_COUNT 10
#define MAX_EVENTS     10

#define TORRENT_PORT 5555
#define WEB_PORT     8080

typedef enum PeerEventType {
	PEER_EVENT_NONE,
	PEER_EVENT_STARTED,
	PEER_EVENT_STOPPED,
	PEER_EVENT_COMPLETED,
} PeerEventType;

typedef struct Peer {
	char     ip[255];/* Peer's ip (as given in request, or as infered
					  * when accepting peer) */
	char     id[20]; /* Peer's id (as given in request) */
	uint16_t port;	 /* Peer's listening port (as given in request) */
	
	int      is_seeder;
	// unsigned long timestamp_last_request;
	// int torrent_index; ?

	int      fd;	/* Tracker file descriptor associated with peer */
} Peer;

typedef struct Torrent {
	Peer all_peers[MAX_PEER_COUNT];
	char hash[40];
	
	int  peer_count;
	/* char peer_list[SOME_LEN] */
	/* char compact_peer_list[SOME_SMALLER_LEN] */
} Torrent;

typedef struct TrackerInfo {
	Torrent all_torrents[MAX_PEER_COUNT];
	
	int     torrent_count;
} TrackerInfo;

#define REQUEST_HANDLER(name) void name(TrackerInfo *tracker_info, int fd)
typedef REQUEST_HANDLER(request_handler);

int accept_client(int listen_fd, int epfd, struct epoll_event *ev);

#endif /* TRACKER_COMMON_H */
