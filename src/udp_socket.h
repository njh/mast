/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003 University of Southampton
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


#ifndef	_UDP_SOCKET_H_
#define	_UDP_SOCKET_H_


#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



typedef struct {

	int sock;
	struct sockaddr_in6 sin;
	struct ipv6_mreq imr;
	
	int timeout;
	
} udp_socket_t;


extern udp_socket_t* udp_socket_create(char *udp_host, int udp_port, int udp_ttl, int bind);
extern void udp_socket_set_timeout( udp_socket_t* udp_socket, int timeout );
extern unsigned int udp_socket_send( udp_socket_t* udp_socket, void* data, unsigned int len);
extern unsigned int udp_socket_recv( udp_socket_t* udp_socket, 
							void* data, unsigned  int len, struct sockaddr_in6 *from);
extern void udp_socket_close( udp_socket_t* udp_socket);


#endif
