/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  Copyright (C) 2003 Nicholas J. Humfrey
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "udp_socket.h"



int create_udp_socket(char *udp_host, int udp_port, int ttl)
{
	struct sockaddr_in udp_sin;
	struct hostent *hp;
	struct ip_mreq imr;
	int    udp_socket;
	int    source_addr;
	int    one=1;
	char   my_hostname[256];

	fprintf(stderr, "connecting to %s/%d\n",udp_host,udp_port);

	udp_sin.sin_family=AF_INET;
	udp_sin.sin_port=htons((u_short)udp_port);

	source_addr=inet_addr(udp_host);
	udp_sin.sin_addr.s_addr=source_addr;



	udp_socket=socket(AF_INET, SOCK_DGRAM, 0);
	
	// Reuse the socket
	if (setsockopt(udp_socket,SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one))) {
			perror("cant set reuse");
	}
	

	if (bind(udp_socket, (struct sockaddr *)&udp_sin,
	         sizeof(struct sockaddr_in))) {
		perror("failed to bind");
		}

	if (IN_MULTICAST(ntohl(source_addr))) {
		udp_sin.sin_addr.s_addr=source_addr;
    fprintf(stderr, "multicast\n"); fflush(stderr);
		if (setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		               sizeof(ttl))) {
			perror("cant set ttl");
			}
		gethostname(my_hostname, 256);
		hp=gethostbyname(my_hostname);
		memcpy((char *)&imr.imr_interface, hp->h_addr, hp->h_length);
		imr.imr_multiaddr.s_addr=source_addr; // naughty
		imr.imr_interface.s_addr=INADDR_ANY; // naughty
		if (setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		               &imr, sizeof(struct ip_mreq))==-1) {
			perror("cant join group");
		} else {
			fprintf(stderr."joined group\n");
			}
	} else {
//		fprintf(stderr, "not multicast\n"); fflush(stderr);
		if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one))) {
			perror("failed to do broadcast sockopt");
			exit(-1);
			}
		}
	return(udp_socket);
}



int close_udp_socket(int udp_socket,  char *udp_host, int udp_port)
{
	struct sockaddr_in udp_sin;
	struct hostent *hp;
	struct ip_mreq imr;
	int    source_addr;
	char   my_hostname[256];

	udp_sin.sin_family=AF_INET;
	udp_sin.sin_port=htons((u_short)udp_port);

	source_addr=inet_addr(udp_host);

	if (IN_MULTICAST(ntohl(source_addr))) {
		udp_sin.sin_addr.s_addr=source_addr;
		gethostname(my_hostname, 256);
		hp=gethostbyname(my_hostname);
		memcpy((char *)&imr.imr_interface, hp->h_addr, hp->h_length);
		imr.imr_multiaddr.s_addr=source_addr; // naughty
		imr.imr_interface.s_addr=INADDR_ANY; // naughty
		if (setsockopt(udp_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		               &imr, sizeof(struct ip_mreq))==-1) {
			perror("cant drop group");
		} else {
			fprintf(stderr,"dropped group\n");
			}
		}
	close(udp_socket);
}

