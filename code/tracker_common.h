#ifndef TRACKER_COMMON_H
#define TRACKER_COMMON_H

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "utility.h"
#include "inet_socket.h"
#include "read_line.h"

#include "http_utility.h"

#define STRINGIFY(x)    #x
#define TO_STRING(x)	STRINGIFY(x)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#if DEBUG
#define ASSERT(expression)	do										\
	{																\
		if(!(expression))											\
		{															\
			fprintf(stderr, "ASSERT FAILED : '%s', %s line %d.\n",	\
					#expression, __FILE__, __LINE__);				\
			*(int *)NULL = 0;										\
		}															\
	}while (0)
	
#else
#define ASSERT(expression)
#endif

#define MAX_PEER_COUNT 10
#define MAX_EVENTS     10

#define TORRENT_PORT 5555
#define WEB_PORT     8080

#define PEER_BENCODE_LEN         (27 + 263 + 12 + 2)
#define PEER_COMPACT_BENCODE_LEN (6)

typedef enum PeerEventType {
	PEER_EVENT_NONE,
	PEER_EVENT_STARTED,
	PEER_EVENT_STOPPED,
	PEER_EVENT_COMPLETED,
} PeerEventType;

/* NOTE: A peer exists if its id is not empty (just check the first
 *       char). */
typedef struct Peer {
	char     id[20]; /* Peer's id (as given in request) */
	int16_t  port;	 /* Peer's listening port (as given in request) */
	struct in_addr  addr_in;	 /* Peer's ip in network-byte order (as given in
					  * request, or as infered when accepting peer) */
	
	int      is_seeder;
	// unsigned long timestamp_last_request;
} Peer;

/* NOTE: A torrent exists if its hash is not empty (just check the
 *       first char). */
typedef struct Torrent {
	Peer all_peers[MAX_PEER_COUNT];
	
	char hash[40];
	
	int  peer_count;
	int  seeder_count;
	int  leecher_count;
} Torrent;

typedef struct TrackerInfo {
	Torrent all_torrents[MAX_PEER_COUNT];
	
	int     torrent_count;
} TrackerInfo;

#define REQUEST_HANDLER(name) void name(int fd, TrackerInfo *tracker_info, struct in_addr *addr_in)
typedef REQUEST_HANDLER(request_handler);

int accept_client(int listen_fd, int epfd, struct epoll_event *ev);

#endif /* TRACKER_COMMON_H */
