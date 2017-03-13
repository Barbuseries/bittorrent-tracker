#ifndef TRACKER_TORRENT_H
#define TRACKER_TORRENT_H

#include <sys/socket.h>

#include "tracker_common.h"

/* Modify tracker information upon peer request.
   Also send peer list if asked to. */
REQUEST_HANDLER(handle_torrent_request);

#endif
