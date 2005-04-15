/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2005 University of Southampton
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 */

#include "config.h"

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

#include "mcast_socket.h"


/* Added to ensure compilation with KAME */
#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

/* better way to check for socklen_t without using autoconf ? */
#if defined(_FREEBSD2) || defined(_FREEBSD3) || \
	(defined(_MAC_UNIX) && (MAC_OS_X_VERSION_MAX_ALLOWED < 1030)) || \
	defined(_SOLARIS26) || defined(_SOLARIS27)
typedef int socklen_t;
#endif



/*
static void
print_sockaddr(struct sockaddr *addr)
{
	char host[NI_MAXHOST];
	
//	fprintf(stderr, "AF_UNSPEC=%d\n", AF_UNSPEC);
//	fprintf(stderr, "AF_INET=%d\n", AF_INET);
//	fprintf(stderr, "AF_INET6=%d\n", AF_INET6);

//	fprintf(stderr, "SOCK_STREAM=%d\n", SOCK_STREAM);
//	fprintf(stderr, "SOCK_DGRAM=%d\n", SOCK_DGRAM);
//	fprintf(stderr, "SOCK_RAW=%d\n", SOCK_RAW);
//	fprintf(stderr, "SOCK_RDM=%d\n", SOCK_RDM);
//	fprintf(stderr, "SOCK_SEQPACKET=%d\n", SOCK_SEQPACKET);

//	fprintf(stderr, "IPPROTO_IP=%d\n", IPPROTO_IP);
//	fprintf(stderr, "IPPROTO_IPV6=%d\n", IPPROTO_IPV6);
//	fprintf(stderr, "IPPROTO_TCP=%d\n", IPPROTO_TCP);
//	fprintf(stderr, "IPPROTO_UDP=%d\n", IPPROTO_UDP);



	switch(addr->sa_family) {

		case AF_INET: {
			struct sockaddr_in *sin = (struct sockaddr_in*)addr;
			getnameinfo(addr, sizeof(struct sockaddr_in),
							  host, sizeof(host), NULL, 0, NI_NUMERICHOST);
			fprintf(stderr,"sin_family=AF_INET  sin_port=%d  sin_addr=%s",
							sin->sin_port, host);
		} break;

		case AF_INET6: {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)addr;
			getnameinfo(addr, sizeof(struct sockaddr_in6),
							  host, sizeof(host), NULL, 0, NI_NUMERICHOST);
			fprintf(stderr,"sa_family=AF_INET6  sin6_port=%d  sin6_addr=%s",
							sin6->sin6_port, host);
		} break;

		default: {
			fprintf(stderr,"sa_family=UNKNOWN, ");
		} break;
	}

	fprintf(stderr,"\n");
}
*/


static int
_get_addrinfo ( const char *host, int port, struct addrinfo *ainfo, struct sockaddr_storage *saddr )
{
    struct addrinfo hints, *res, *cur;
    char service[NI_MAXSERV];
    char hostname[NI_MAXHOST];
    int error = -1;
    int retval = -1;
    
    // Convert port number to a string
    snprintf( service, sizeof( service ), "%d", port );
    snprintf( hostname, sizeof( hostname ), "%s", host );

	// Setup hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    //hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    error = getaddrinfo(hostname, service, &hints, &res);
    if (error || res == NULL) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(error));
        return error;
    }


	/* Check each of the results */
	cur = res;
	while (cur) {
		int sockfd = socket(cur->ai_family,
							cur->ai_socktype,
							cur->ai_protocol);
		if (!(sockfd < 0)) {
            if (bind(sockfd, cur->ai_addr, cur->ai_addrlen) == 0) {
                close(sockfd);
				memcpy( saddr, cur->ai_addr, cur->ai_addrlen );
				memcpy( ainfo, cur, sizeof(struct addrinfo) );
                ainfo->ai_canonname = NULL;
                ainfo->ai_addr = (struct sockaddr*)saddr;
                ainfo->ai_next = NULL;
                retval = 0;
                break;
            }

            close(sockfd);
        }
        cur=cur->ai_next;
    }

    freeaddrinfo( res );
    
    // Unsuccessful ?
    if (retval == -1) {
    	fprintf(stderr, "Failed to find an address for getaddrinfo() to bind to.\n");
    }

    return retval;
}




static int
_is_multicast(struct sockaddr_storage *addr)
{

    switch (addr->ss_family) {
        case AF_INET: {
            struct sockaddr_in *addr4=(struct sockaddr_in *)addr;
            return IN_MULTICAST(ntohl(addr4->sin_addr.s_addr));
        } break;

        case AF_INET6: {
            struct sockaddr_in6 *addr6=(struct sockaddr_in6 *)addr;
            return IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr);
        } break;

        default: {
           return -1;
		}
    }
} 

static int
_join_group( mcast_socket_t *mcast_socket /* , interface */ )
{
    int retval = -1;

    switch (mcast_socket->saddr.ss_family) {
        case AF_INET: {
 
 			mcast_socket->imr.imr_multiaddr.s_addr=
                ((struct sockaddr_in*)&mcast_socket->saddr)->sin_addr.s_addr;
            mcast_socket->imr.imr_interface.s_addr= INADDR_ANY;

            retval= setsockopt(mcast_socket->fd_recv, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                           &mcast_socket->imr, sizeof(mcast_socket->imr));
            if (retval<0)
                perror("_join_group failed on IP_ADD_MEMBERSHIP");

/*
 			if (setsockopt(sock->fd_recv, IPPROTO_IP, IP_MULTICAST_IF, (char *)&iface_addr, sizeof(iface_addr)) != 0) {
				perror("_join_group failed on IP_MULTICAST_IF");
				return NULL;
			}
*/
 
 		} break;

        case AF_INET6: {

			memcpy(&mcast_socket->imr6.ipv6mr_multiaddr,
				&(((struct sockaddr_in6*)&mcast_socket->saddr)->sin6_addr),
				sizeof(struct in6_addr));

			// FIXME: should be a method for chosing interface to use
			mcast_socket->imr6.ipv6mr_interface=0;


			retval= setsockopt(mcast_socket->fd_recv, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
					&mcast_socket->imr6, sizeof(mcast_socket->imr6));
			if (retval<0)
				perror("_join_group failed on IPV6_ADD_MEMBERSHIP");

        } break;
    }


    return retval;
}


static int
_leave_group( mcast_socket_t* sock )
{
    int retval = -1;

	switch (sock->saddr.ss_family) {
		case AF_INET: {
			retval= setsockopt(sock->fd_recv, IPPROTO_IP, IP_DROP_MEMBERSHIP,
						   &(sock->imr), sizeof(sock->imr));
			if (retval<-1)
				perror("IP_DROP_MEMBERSHIP failed");
		} break;

		case AF_INET6: {
			retval= setsockopt(sock->fd_recv, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
						   &(sock->imr6), sizeof(sock->imr6));
			if (retval<-1)
				perror("IPV6_DROP_MEMBERSHIP failed");
		} break;
		
	}

	return retval;
}

static int
_bind_socket( int sockfd, mcast_socket_t* sock )
{
	int one=1;

	// These socket options help re-binding to a socket
	// after a previous process was killed

#ifdef SO_REUSEADDR
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
			perror("SO_REUSEADDR failed");
	}
#endif

#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one))) {
			perror("SO_REUSEPORT failed");
	}
#endif


	// Bind the recieve socket
	if (bind(sockfd, (struct sockaddr *)&sock->saddr, sock->ainfo.ai_addrlen)<0) 
	{
		perror("bind() failed");
		return -1;
	}
	
	
	// Success
	return 0;
}


static int
_set_socketopts( mcast_socket_t* mcast_socket )
{
    int r1, r2, retval;

    retval=-1;

    switch (mcast_socket->saddr.ss_family) {
        case AF_INET: {
 
            r1= setsockopt(mcast_socket->fd_trans, IPPROTO_IP, IP_MULTICAST_LOOP,
                           &mcast_socket->loopback, sizeof(mcast_socket->loopback));
            if (r1<0)
                perror("_set_socketopts failed on IP_MULTICAST_LOOP");


            r2= setsockopt(mcast_socket->fd_trans, IPPROTO_IP, IP_MULTICAST_TTL,
                           &mcast_socket->hops, sizeof(mcast_socket->hops));
            if (r2<0)
               perror("_set_socketopts failed on IP_MULTICAST_TTL");

 
 		} break;

        case AF_INET6: {

			r1= setsockopt(mcast_socket->fd_trans, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
					&mcast_socket->loopback, sizeof(mcast_socket->loopback));
			if (r1<0)
				perror("_set_socketopts failed on IPV6_MULTICAST_LOOP");
			
			r2= setsockopt(mcast_socket->fd_trans, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
					&mcast_socket->hops, sizeof(mcast_socket->hops));
			if (r2<0)
				perror("_set_socketopts failed on IPV6_MULTICAST_HOPS");


        } break;

        default:
            r1=r2=-1;
    }

    if ((r1>=0) && (r2>=0))
        retval=0;

    return retval;
}


/* 

static void
_print_settings(mcast_socket_t* mcast_socket)
{
    char group[NI_MAXHOST];
    char service[NI_MAXSERV];

	memset(group, 0, sizeof(group));
	memset(service, 0, sizeof(service));

	getnameinfo(
		mcast_socket->ainfo.ai_addr,
		mcast_socket->ainfo.ai_addrlen,
		group, sizeof(group),
		service, sizeof(service),
		NI_NUMERICHOST | NI_NUMERICSERV);


	fprintf(stderr,"mcast_socket = {\n" );
	fprintf(stderr,"  fd_recv=%d\n", mcast_socket->fd_recv );
	switch( mcast_socket->ainfo.ai_family ) {
		case AF_INET:
			fprintf(stderr,"  family=AF_INET\n" );
		break;
		
		case AF_INET6:
			fprintf(stderr,"  family=AF_INET6\n" );
		break;
		
		default:
			fprintf(stderr,"  family=%d\n", mcast_socket->ainfo.ai_family );
		break;
	}
	
	fprintf(stderr,"  group=%s\n", group );
	fprintf(stderr,"  port=%s\n", service );
	fprintf(stderr,"  timeout=%d\n", mcast_socket->timeout );
	fprintf(stderr,"  loopback=%d\n", mcast_socket->loopback );
	fprintf(stderr,"  hops=%d\n", mcast_socket->hops );
	fprintf(stderr,"  multicast=%d\n", mcast_socket->multicast );
	fprintf(stderr,"}\n" );
}

*/

mcast_socket_t*
mcast_socket_create(char *host, int port, int hops, int loopback)
{
	mcast_socket_t *sock = NULL;	
	
	/* Allocate memory for the record */
	sock = (mcast_socket_t*)malloc(sizeof(mcast_socket_t));
	if (!sock) {
		perror("Failed to allocate memory for mcast_socket_t");
	}
	
	/* Initalise */
	memset( sock, 0, sizeof(mcast_socket_t));
	sock->fd_recv = 0;
	sock->fd_trans = 0;
	sock->multicast = 0;	// not joined yet
	sock->timeout = 0;
	sock->loopback = loopback;
	sock->hops = hops;
	
	
	/* Lookup address, get info */
	if (_get_addrinfo( host, port, &sock->ainfo, &sock->saddr )) {
		mcast_socket_close(sock);
		return NULL;
	}
	



	// Create socket for recieving
	if ((sock->fd_recv = socket(sock->ainfo.ai_family,
							   sock->ainfo.ai_socktype,
							   sock->ainfo.ai_protocol )) <0) {
		perror("recieving socket() failed");
		mcast_socket_close(sock);
		return NULL;
	}

	// Create socket for transmitting
	if ((sock->fd_trans = socket(sock->ainfo.ai_family,
							   sock->ainfo.ai_socktype,
							   sock->ainfo.ai_protocol )) <0) {
		perror("transmitting socket() failed");
		mcast_socket_close(sock);
		return NULL;
	}
	
	
	

	// Join multicast group ?
	sock->multicast = _is_multicast( &sock->saddr );
	if( sock->multicast == 1) {

		// Bind the recieving socket
		if (_bind_socket( sock->fd_recv, sock )) {
			mcast_socket_close(sock);
			return NULL;
		}
		
		if (_set_socketopts( sock )<0) {
			fprintf(stderr, "Failed to set socket options.\n");
			sock->multicast = 0;
			mcast_socket_close(sock);
			return NULL;
		}

		if (_join_group( sock )<0) {
			fprintf(stderr, "Failed to join multicast group.\n");
			sock->multicast = 0;
			mcast_socket_close(sock);
			return NULL;
		}


	} else if (sock->multicast == 0) {
		int one=1;
		
		// Bind the recieving socket
		//if (_bind_socket( sock->fd_recv, sock )) {
		//	mcast_socket_close(sock);
		//	return NULL;
		//}
		
		if (setsockopt(sock->fd_recv, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)))
		{
			perror("SO_BROADCAST failed");
		}
	
	} else {

		fprintf(stderr, "Error checking if address is multicast.\n");

	}
	
	return sock;
}


void
mcast_socket_set_timeout( mcast_socket_t* sock, int timeout )
{
	sock->timeout = timeout;
}


int
mcast_socket_get_timeout( mcast_socket_t* sock )
{
	return sock->timeout;
}


char*
mcast_socket_get_family( mcast_socket_t* sock )
{
	if (sock->ainfo.ai_family == AF_INET) {
		return "ipv4";
	} else if (sock->ainfo.ai_family == AF_INET6) {
		return "ipv6";
	} else {
		return "unknown";
	}
}


int
mcast_socket_send( mcast_socket_t* sock, void* data, unsigned int len)
{
	ssize_t result =  sendto(
		sock->fd_trans, data, len, 0,
		(struct sockaddr *)&sock->saddr,
		sock->ainfo.ai_addrlen);
	
	if (result<=0) {
		perror("sendto()");
	}

	return result;
}



int
mcast_socket_recv( mcast_socket_t* sock, void* data, unsigned  int len, char *from, int from_len)
{
	struct sockaddr_storage saddr;
	socklen_t saddr_size = sizeof(saddr);
	fd_set readfds;
	struct timeval timeout, *timeout_ptr = NULL;
	int packet_len, retval;
	
	
	// Use a timeout ?
	if (sock->timeout) {
		//timeout = &sock->timeout;
		timeout.tv_sec = sock->timeout;
		timeout.tv_usec = 0;
		timeout_ptr = &timeout;
	}
	

	// Watch socket to see when it has input.
	FD_ZERO(&readfds);
	FD_SET(sock->fd_recv, &readfds);
	retval = select(FD_SETSIZE, &readfds, NULL, NULL, timeout_ptr);

	// Check return value 
	if (retval == -1) {
		perror("select()");
		return -1;
		
	} else if (retval==0) {
		fprintf(stderr, "Timed out waiting for packet after %d seconds.\n", sock->timeout);
		return 0;
	}
	
	
	/* Packet is waiting - read it in */
	packet_len = recvfrom(sock->fd_recv, data, len, 0,
					 (struct sockaddr *)&saddr, &saddr_size);
		
	/* Get the from address */
	getnameinfo(
		(struct sockaddr*)&saddr, saddr_size,
		from, from_len,
		NULL, 0,			// Don't get the port
		NI_NUMERICHOST | NI_NUMERICSERV);
		
	if (packet_len<=0) {
		perror("recvfrom()");
	}
	
	return packet_len;	 
}




void
mcast_socket_close( mcast_socket_t* sock )
{

	/* Drop Multicast membership */
	if (sock->multicast)
	{
		_leave_group( sock );
	}
		
	/* Close the sockets */
	if (sock->fd_recv) close(sock->fd_recv);
	if (sock->fd_trans) close(sock->fd_trans);
	
	/* Free the memory used by the connection record */
	free(sock);
}

