#ifndef TRACKER_COMMON_H
#define TRACKER_COMMON_H

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#include "utility.h"
#include "inet_socket.h"
#include "read_line.h"

#include "http_utility.h"

/* NOTE: Compact peer list does not work, so it's disabled by
 *       default. */
#define DISABLE_COMPACT_PEER_LIST  1

#define MIN_REQUEST_INTERVAL       20 /* In seconds */

#define MAX_TORRENT_COUNT          10
#define MAX_PEER_PER_TORRENT_COUNT 25

#define MAX_WEB_CLIENT_COUNT       10

#define TORRENT_PORT               55555
#define WEB_PORT                   8080

/* id + ip + port + dictionnary overhead */
#define MAX_PEER_BENCODE_LEN       (23 + 259 + 7 + 19)
#define PEER_COMPACT_BENCODE_LEN   6

typedef enum PeerEventType
{
	PEER_EVENT_NONE,
	PEER_EVENT_STARTED,
	PEER_EVENT_STOPPED,
	PEER_EVENT_COMPLETED,
} PeerEventType;

/* NOTE: A peer exists if its id is not empty (just check the first
 *       char is not '\0'). */
typedef struct Peer
{
	char     id[20];         /* Peer's id (as given in request) */
	int16_t  port;	         /* Peer's listening port (as given in request) */
	struct in_addr  addr_in; /* Peer's ip in network-byte order (as given in
							  * request, or as infered when accepting peer) */
	
	int      is_seeder;
	// unsigned long timestamp_last_request;
} Peer;

/* NOTE: A torrent exists if its hash is not empty (just check the
 *       first char is not '\0'). */
typedef struct Torrent
{
	Peer all_peers[MAX_PEER_PER_TORRENT_COUNT];
	
	char hash[40];
	
	int  seeder_count;
	int  leecher_count;
} Torrent;

typedef struct TrackerInfo
{
	Torrent all_torrents[MAX_TORRENT_COUNT];
	
	int     torrent_count;
} TrackerInfo;

#define REQUEST_HANDLER(name) void name(int fd, TrackerInfo *tracker_info, struct in_addr *addr_in)
typedef REQUEST_HANDLER(request_handler);

int accept_client(int listen_fd, int epfd, struct epoll_event *ev);
int get_peer_count(Torrent *torrent);

#endif /* TRACKER_COMMON_H */
