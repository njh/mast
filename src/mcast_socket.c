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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "udp_socket.h"


/* Added be DE to ensure compilation with KAME */
#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif


udp_socket_t*
udp_socket_create(char *udp_host, int udp_port, int ttl, int do_bind)
{
	udp_socket_t *udp_socket;
	int    one=1;
	
	/* Allocate memory for the record */
	udp_socket = (udp_socket_t*)malloc(sizeof(udp_socket_t));
	if (!udp_socket) {
		perror("Failed to allocate memory for udp_socket_t");
	}
	memset( udp_socket, 0, sizeof(*udp_socket));
	
	
	/* Initalise timeout */
	udp_socket->timeout = 0;
		

	fprintf(stderr, "Connecting to %s/%d.\n",udp_host,udp_port);

	udp_socket->sin.sin6_family=AF_INET6;
	udp_socket->sin.sin6_port=htons((u_short)udp_port);
	if (inet_pton (AF_INET6, udp_host, &(udp_socket->sin.sin6_addr.s6_addr)) != 1)
	{
    	perror ("inet_pton");
    	exit (1);
	}


	if ((udp_socket->sock = socket(AF_INET6, SOCK_DGRAM, 0)) <0) {
		perror("socket(AF_INET6, SOCK_DGRAM, 0) failed");
    	exit (1);
	}
	
	
	if (setsockopt(udp_socket->sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
			perror("SO_REUSEADDR failed");
	}

#ifdef SO_REUSEPORT
	if (setsockopt(udp_socket->sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one))) {
			perror("SO_REUSEPORT failed");
	}
#endif

	if (do_bind) {
		if (bind(udp_socket->sock, (struct sockaddr *)&udp_socket->sin, sizeof(udp_socket->sin))) 
		{
			perror("Failed to bind");
    		exit (1);
		}
	}


	if (IN6_IS_ADDR_MULTICAST(&(udp_socket->sin.sin6_addr)))
	{

		if (setsockopt(udp_socket->sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl)))
		{
			perror("Can't set multicast ttl");
		}

		inet_pton(AF_INET6,udp_host, &(udp_socket->imr.ipv6mr_multiaddr));
		udp_socket->imr.ipv6mr_interface = 0;

		if (setsockopt(udp_socket->sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
					   &(udp_socket->imr), sizeof(udp_socket->imr))==-1)
		{
			perror("Can't join multicast group");
			exit (1);
		}
		else
		{
			fprintf(stderr, "Joined multicast group.\n");
		}
	}
	else /*not multicast address*/
	{
		if (setsockopt(udp_socket->sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)))
		{
			perror("Failed to do broadcast sockopt:");
			exit(1);
		}
	}
	
	return udp_socket;
}


void
udp_socket_set_timeout( udp_socket_t* udp_socket, int timeout )
{
	udp_socket->timeout = timeout;
}



unsigned int
udp_socket_send( udp_socket_t* udp_socket, void* data, unsigned int len)
{
	ssize_t result =  sendto(udp_socket->sock, data, len, 0,
		(struct sockaddr *)&udp_socket->sin, sizeof(udp_socket->sin));
		
	return result;
}



unsigned int
udp_socket_recv( udp_socket_t* udp_socket, void* data, unsigned  int len,
					struct sockaddr_in6 *from)
{
	int sockaddr_size = sizeof( *from );
	fd_set readfds;
	struct timeval timeout, *timeout_ptr = NULL;
	int packet_len, retval;
	
	
	// Use a timeout ?
	if (udp_socket->timeout) {
		//timeout = &udp_socket->timeout;
		timeout.tv_sec = udp_socket->timeout;
		timeout.tv_usec = 0;
		timeout_ptr = &timeout;
	}
	

	// Watch socket to see when it has input.
	FD_ZERO(&readfds);
	FD_SET(udp_socket->sock, &readfds);
	retval = select(FD_SETSIZE, &readfds, NULL, NULL, timeout_ptr);

	// Check return value 
	if (retval == -1) {
		perror("select()");
		return -1;
		
	} else if (retval==0) {
		fprintf(stderr, "Timed out waiting for packet after %d seconds.\n", udp_socket->timeout);
	   
		return 0;
	}
	
	
	/* Packet is waiting - read it in */
	packet_len = recvfrom(udp_socket->sock, data, len, 0,
					 (struct sockaddr *)from, &sockaddr_size);
					 
	if (packet_len<=0) {
		perror("recvfrom()");
	}
	
	return packet_len;	 
}


void
udp_socket_close( udp_socket_t* udp_socket )
{

	/* Drop Multicast membership */
	if (IN6_IS_ADDR_MULTICAST(&(udp_socket->sin.sin6_addr)))
	{

		if (setsockopt(udp_socket->sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
		               &(udp_socket->imr), sizeof(udp_socket->imr))==-1) {
			perror("can't drop multicast group");
		} else {
			fprintf(stderr, "Dropped multicast group.\n");
			}
		}
		
	/* Close the socket */
	close(udp_socket->sock);
	
	/* Free the memory used by the connection record */
	free(udp_socket);
}

