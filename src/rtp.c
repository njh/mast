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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include "random32.h"

#include "config.h"
#include "mast.h"
#include "rtp.h"


#define PAYLOAD_BUFFER_LEN (2048)



rtp_packet_t* rtp_packet_init( config_t* config )
{
	rtp_packet_t *rtp_packet = NULL;
	int packet_length = sizeof(rtp_packet_t) + PAYLOAD_BUFFER_LEN;
	
	
	/* Allocate memory for the packet */
	rtp_packet = (rtp_packet_t*)malloc(packet_length);
	if (!rtp_packet) {
		perror("Failed to malloc packet buffer");
		exit(1);
	}
	rtp_packet->packet_buffer_len = sizeof(rtp_hdr_t) + PAYLOAD_BUFFER_LEN;
	rtp_packet->payload_buffer_len = PAYLOAD_BUFFER_LEN;


	// The standard RTP header
	rtp_packet->head.v=RTP_VERSION;
	rtp_packet->head.p=0;
	rtp_packet->head.x=0;
	rtp_packet->head.cc=0;
	rtp_packet->head.m=1;
	rtp_packet->head.pt = config->payload_type;


	// Initalise the sequence number and timestamp
	rtp_packet->head.seq = random32(0) & 65535;
	rtp_packet->head.ts = random32(1);
	
	
	/* Use random ssrc if none specified */
	if (config->ssrc)
		rtp_packet->head.ssrc = htonl(config->ssrc);
	else
		rtp_packet->head.ssrc = random32(2);


	/* This packet doesn't currently contain valid data */
	rtp_packet->valid = 0;

	return rtp_packet;
}

void rtp_packet_set_frame_size( rtp_packet_t* rtp_packet, int frame_size )
{
	rtp_packet->frame_size = frame_size;

}

void rtp_packet_send( mcast_socket_t* rtp_socket, rtp_packet_t* rtp_packet, int payload_len )
{
	int packet_len = payload_len + sizeof(rtp_hdr_t);
	int seq_num = rtp_packet->head.seq;
	int time_stamp = rtp_packet->head.ts;
	
	
	/* Encode to network byte order */
	rtp_packet->head.ts = htonl(time_stamp);
	rtp_packet->head.seq = htons(seq_num);
	
	if (mcast_socket_send( rtp_socket, &rtp_packet->head, packet_len )<=0) {
		perror("sendto failed");
	}

	
	// Update RTP header fields
	rtp_packet->head.ts = time_stamp + (payload_len/rtp_packet->frame_size);
	rtp_packet->head.seq = seq_num + 1;
		
	//rtp_packet_count++;
	//rtp_octet_count += payload_len;
	
	/* Now that packet has been sent, invalidate it */
	rtp_packet->valid = 0;
}



int rtp_packet_recv( mcast_socket_t* rtp_socket, rtp_packet_t* packet )
{
	char from[NI_MAXHOST];
	
	int len=mcast_socket_recv( rtp_socket, 
			&packet->head, packet->packet_buffer_len,
			from, NI_MAXHOST );
	
	/* Failure ? */	 
	if (len <= 0) return len;

	
	/* Decode fields in network byte order */
	packet->head.ssrc = ntohl( packet->head.ssrc );
	packet->head.ts = ntohl( packet->head.ts );
	packet->head.seq = ntohs( packet->head.seq );
	
	/* Convert the IP6 address to a string */
	strncpy( packet->src_ip, from, NI_MAXHOST );
	
	/* Calculate the size of the payload */
	packet->payload_size = len - sizeof( packet->head );
	

	/*
	printf("len=%u from=? v=%d p=%u x=%u cc=%u m=%u pt=%u seq=%u tx=%u ssrc=0x%x\n",
		len, packet->head.v, packet->head.p, packet->head.x, packet->head.cc,
		packet->head.m, packet->head.pt, packet->head.seq, packet->head.ts, 
		packet->head.ssrc);
	*/
	
	/* Packet now contains valid data */
	packet->valid = 1;

	return len;
}




void rtp_packet_delete( rtp_packet_t* rtp_packet )
{

	free( rtp_packet );

}
