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


#ifndef	_MCAST_SOCKET_H_
#define	_MCAST_SOCKET_H_


#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



typedef struct {


	/* I am using a pair of Socket File Descriptors
	   because I have had problems sending packets out on
	   a socket which has been bound to a port and an address.
	   Only seems to be an issue on BSD - works ok on Linux.
	   
	   sendto(): 'Operation not supported'
	   
	   Not the best solution.. but makes things work.
	*/
	int fd_recv;
	int fd_trans;

	struct addrinfo ainfo;
	struct sockaddr_storage saddr;
	struct ipv6_mreq imr6;
	struct ip_mreq imr;
	
	int timeout;	// timeout for receiving packets, or 0 for no timeout
	int loopback;	// receive packets we send ?
	int hops;		// multicast hops/ttl
	int multicast;	// true if we joined multicast group
	
	char origin_addr[NI_MAXHOST];
	
} mcast_socket_t;

/*** Fixme: mcast_socket_t could be opaque to outside code */


extern mcast_socket_t* mcast_socket_create(char *host, int port, int hops, int loopback);
extern void mcast_socket_set_timeout( mcast_socket_t* mcast_socket, int timeout );
extern int mcast_socket_get_timeout( mcast_socket_t* mcast_socket );
extern char* mcast_socket_get_family( mcast_socket_t* mcast_socket );
extern int mcast_socket_send( mcast_socket_t* mcast_socket, void* data, unsigned int len);
extern int mcast_socket_recv( mcast_socket_t* mcast_socket, void* data, unsigned int len, char *from, int from_len);
extern void mcast_socket_close( mcast_socket_t* mcast_socket);


#endif
