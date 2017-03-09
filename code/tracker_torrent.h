#ifndef TRACKER_TORRENT_H
#define TRACKER_TORRENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tracker_common.h"


REQUEST_HANDLER(handle_torrent_request);

#endif
