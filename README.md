# Coding Style #

GNU Coding Standards

# Note #

Because we used epoll from the start, `tracker3.c` and `tracker4.c`
are the same.

Sending a compact peer list does not work properly, so by default,
it's disabled. To enable it, `DISABLE_COMPACT_PEER_LIST` must be set
to 0 in `tracker_common.h`.
