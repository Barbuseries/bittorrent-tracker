#ifndef INET_SOCKET_H
#define INET_SOCKET_H

#include <sys/socket.h>
#include <sys/un.h>

/* The following arguments are common to several of the routines
   below:

        'host':         NULL for loopback IP address, or
                        a host name or numeric IP address
        'service':      either a name or a port number
        'type':         either SOCK_STREAM or SOCK_DGRAM
*/

/* Create socket and connect it to the address specified by
   'host' + 'service'/'type'. Return socket descriptor on success,
   or -1 on error */
int inetConnect(const char *host, const char *service, int type);

/* Create stream socket, bound to wildcard IP address + port given in
   'service'. Make the socket a listening socket, with the specified
   'backlog'. Return socket descriptor on success, or -1 on error. */
int inetListen(const char *service, int backlog, socklen_t *addrlen);

/* Create socket bound to wildcard IP address + port given in
   'service'. Return socket descriptor on success, or -1 on error. */
int inetBind(const char *service, int type, socklen_t *addrlen);

/* Given a socket address in 'addr', whose length is specified in
   'addrlen', return a null-terminated string containing the host and
   service names in the form "(hostname, port#)". The string is
   returned in the buffer pointed to by 'addrStr', and this value is
   also returned as the function result. The caller must specify the
   size of the 'addrStr' buffer in 'addrStrLen'. */
char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen,
                char *addrStr, int addrStrLen);

/* Suggested length for string buffer that caller
   should pass to inetAddressStr(). Must be greater
   than (NI_MAXHOST + NI_MAXSERV + 4) */
#define IS_ADDR_STR_LEN 4096

#endif
