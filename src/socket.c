/*

  socket.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#include "config.h"
#include "mast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/time.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>


// Added to ensure compilation with KAME
#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

enum {
    DO_BIND_SOCKET,
    DONT_BIND_SOCKET
};


static int _create_socket(mast_socket_t *sock, int flags, const char* address, const char* port)
{
    struct addrinfo hints, *res, *cur;
    int error = -1;
    int retval = -1;

    // Setup hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    error = getaddrinfo(address, port, &hints, &res);
    if (error || res == NULL) {
        mast_warn("getaddrinfo failed: %s", gai_strerror(error));
        return error;
    }

    // Check each of the results
    cur = res;
    while (cur) {
        sock->fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);

        if (sock->fd >= 0) {
            int one = 1;
            // These socket options help re-binding to a socket
            // after a previous process was killed

#ifdef SO_REUSEADDR
            if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
                perror("SO_REUSEADDR failed");
            }
#endif

#ifdef SO_REUSEPORT
            if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one))) {
                perror("SO_REUSEPORT failed");
            }
#endif

            if (flags == DONT_BIND_SOCKET || bind(sock->fd, cur->ai_addr, cur->ai_addrlen) == 0) {
                // Success!
                memcpy( &sock->saddr, cur->ai_addr, cur->ai_addrlen );
                retval = 0;
                break;
            }

            close(sock->fd);
        }
        cur=cur->ai_next;
    }

    freeaddrinfo( res );

    return retval;
}


static int _is_multicast(struct sockaddr_storage *addr)
{
    switch (addr->ss_family) {
    case AF_INET: {
        struct sockaddr_in *addr4=(struct sockaddr_in *)addr;
        return IN_MULTICAST(ntohl(addr4->sin_addr.s_addr));
    }
    break;

    case AF_INET6: {
        struct sockaddr_in6 *addr6=(struct sockaddr_in6 *)addr;
        return IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr);
    }
    break;

    default: {
        return -1;
    }
    }
}

static int _get_interface_ipv4_addr(const char* ifname, struct in_addr *addr)
{
    struct ifaddrs *addrs, *cur;
    int retval = -1;
    int foundAddress = FALSE;

    if (ifname == NULL || strlen(ifname) < 1) {
        addr->s_addr = INADDR_ANY;
        return 0;
    }

    // Get a linked list of all the interfaces
    retval = getifaddrs(&addrs);
    if (retval < 0) {
        return retval;
    }

    // Iterate through each of the interfaces
    for(cur = addrs; cur; cur = cur->ifa_next)
    {
        if (cur->ifa_addr && cur->ifa_addr->sa_family == AF_INET) {
            if (strcmp(cur->ifa_name, ifname) == 0) {
                struct sockaddr_in *sockaddr = (struct sockaddr_in *)cur->ifa_addr;
                addr->s_addr = sockaddr->sin_addr.s_addr;
                foundAddress = TRUE;
                break;
            }
        }
    }

    freeifaddrs(addrs);

    return foundAddress ? 0 : -1;
}


static int _lookup_interface( mast_socket_t *sock, const char* ifname)
{
    unsigned int if_index = 0;
    int retval = -1;

    // If a network interface name was given, check it exists
    if (ifname != NULL && strlen(ifname) > 0) {
        if_index = if_nametoindex(ifname);
        if (if_index == 0) {
            if (errno == ENXIO) {
                mast_error("Network interface not found: %s", ifname);
                return -1;
            } else {
                mast_error("Error looking up interface: %s", strerror(errno));
            }
        }
    }

    switch (sock->saddr.ss_family) {
    case AF_INET:

        sock->imr.imr_multiaddr.s_addr=
            ((struct sockaddr_in*)&sock->saddr)->sin_addr.s_addr;

        retval = _get_interface_ipv4_addr( ifname, &sock->imr.imr_interface);
        if (retval) {
            mast_error("Failed to get IPv4 address of network interface");
        }
        break;

    case AF_INET6:

        memcpy(&sock->imr6.ipv6mr_multiaddr,
               &(((struct sockaddr_in6*)&sock->saddr)->sin6_addr),
               sizeof(struct in6_addr));

        sock->imr6.ipv6mr_interface = if_index;
        break;

    default:
        mast_error("Unknown socket address family: %d", sock->saddr.ss_family);
        retval = -1;
        break;
    }

    return retval;
}

static int _join_group( mast_socket_t *sock, const char* ifname)
{
    int retval = -1;

    if (_lookup_interface(sock, ifname))
        return -1;

    switch (sock->saddr.ss_family) {
    case AF_INET:
        retval = setsockopt(sock->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                            &sock->imr, sizeof(sock->imr));
        if (retval < 0)
            mast_warn("IP_ADD_MEMBERSHIP failed: %s", strerror(errno));
        break;

    case AF_INET6:
        retval = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                            &sock->imr6, sizeof(sock->imr6));
        if (retval < 0)
            mast_warn("IPV6_ADD_MEMBERSHIP failed: %s", strerror(errno));
        break;
    }

    if (retval == 0)
        sock->joined_group = TRUE;

    return retval;
}


static int _leave_group( mast_socket_t* sock )
{
    int retval = -1;

    switch (sock->saddr.ss_family) {
    case AF_INET:
        retval = setsockopt(sock->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                            &(sock->imr), sizeof(sock->imr));
        if (retval<0)
            mast_warn("IP_DROP_MEMBERSHIP failed: %s", strerror(errno));
        break;

    case AF_INET6:
        retval = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
                            &(sock->imr6), sizeof(sock->imr6));
        if (retval<0)
            mast_warn("IPV6_DROP_MEMBERSHIP failed: %s", strerror(errno));
        break;
    }

    return retval;
}


static int _set_multicast_interface( mast_socket_t* sock, const char* ifname )
{
    int retval = -1;

    if (_lookup_interface(sock, ifname))
        return -1;

    switch (sock->saddr.ss_family) {
    case AF_INET:
        retval = setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_IF,
                            &sock->imr.imr_interface, sizeof(sock->imr.imr_interface));
        if (retval < 0)
            mast_warn("IP_MULTICAST_IF failed: %s", strerror(errno));
        break;

    case AF_INET6:
        retval = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                            &sock->imr6.ipv6mr_interface, sizeof(sock->imr6.ipv6mr_interface));
        if (retval < 0)
            mast_warn("IPV6_MULTICAST_IF failed: %s", strerror(errno));
        break;
    }

    return retval;
}


static int _set_multicast_hops( mast_socket_t* sock, int hops )
{
    int retval = -1;

    switch (sock->saddr.ss_family) {
    case AF_INET:
        retval = setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_TTL, &hops, sizeof(hops));
        if (retval < 0)
            mast_warn("IP_MULTICAST_TTL failed: %s", strerror(errno));
        break;

    case AF_INET6:
        retval = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));
        if (retval < 0)
            mast_warn("IPV6_MULTICAST_HOPS failed: %s", strerror(errno));
        break;
    }

    return retval;
}


static int _set_multicast_loopback(mast_socket_t* sock, char loop )
{
    int retval = -1;

    switch (sock->saddr.ss_family) {
    case AF_INET:
        retval = setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (retval < 0)
            mast_warn("IP_MULTICAST_LOOP failed: %s", strerror(errno));
        break;

    case AF_INET6:
        retval = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));
        if (retval < 0)
            mast_warn("IPV6_MULTICAST_LOOP failed: %s", strerror(errno));
        break;
    }

    return retval;
}


int mast_socket_open_recv(mast_socket_t* sock, const char* address, const char* port, const char *ifname)
{
    int is_multicast;

    // Initialise
    memset(sock, 0, sizeof(mast_socket_t));

    mast_info("Opening socket: %s/%s", address, port);
    if (_create_socket(sock, DO_BIND_SOCKET, address, port)) {
        mast_error("Failed to open socket for receiving.");
        return -1;
    }

    // Join multicast group ?
    is_multicast = _is_multicast( &sock->saddr );
    if (is_multicast == 1) {
        mast_debug("Joining multicast group");
        if (_join_group(sock, ifname)) {
            mast_socket_close(sock);
            return -1;
        }

    } else if (is_multicast != 0) {
        mast_warn("Error checking if address is multicast");
    }

    return 0;
}

int mast_socket_open_send(mast_socket_t* sock, const char* address, const char* port, const char *ifname)
{
    int is_multicast;

    // Initialise
    memset(sock, 0, sizeof(mast_socket_t));

    mast_info("Opening transmit socket: %s/%s", address, port);
    if (_create_socket(sock, DONT_BIND_SOCKET, address, port)) {
        mast_error("Failed to open socket for sending.");
        return -1;
    }

    // Set multicast socket options?
    is_multicast = _is_multicast( &sock->saddr );
    if (is_multicast == 1) {
        _set_multicast_interface(sock, ifname);
        _set_multicast_hops(sock, 255);
        _set_multicast_loopback(sock, TRUE);
    } else if (is_multicast != 0) {
        mast_warn("Error checking if address is multicast");
    }

    return 0;
}


int mast_socket_recv( mast_socket_t* sock, void* data, unsigned  int len)
{
    fd_set readfds;
    struct timeval timeout;
    int packet_len, retval;

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    // Watch socket to see when it has input.
    FD_ZERO(&readfds);
    FD_SET(sock->fd, &readfds);
    retval = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

    // Check return value
    if (retval == -1) {
        perror("select()");
        return -1;

    } else if (retval==0) {
        mast_warn("Timed out waiting for packet after %ld seconds", timeout.tv_sec);
        return 0;
    }

    // Packet is waiting - read it in
    packet_len = recv(sock->fd, data, len, 0);

    return packet_len;
}


int mast_socket_send( mast_socket_t* sock, void* data, unsigned  int len)
{
    mast_debug("Sending %d byte packet", len);

    int nbytes = sendto(
                     sock->fd,
                     data, len,
                     0, // Flags
                     (struct sockaddr*)&(sock->saddr),
                     sizeof(struct sockaddr_in)
                 );
    if (nbytes < 0) {
        perror("sendto");
        return 1;
    }

    return nbytes;
}


void mast_socket_close(mast_socket_t* sock )
{
    // Drop Multicast membership
    if (sock->joined_group)
    {
        _leave_group( sock );
        sock->joined_group = 0;
    }

    // Close the sockets
    if (sock->fd >= 0) {
        close(sock->fd);
        sock->fd = -1;
    }
}
