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

#ifndef	_RTP_H_
#define	_RTP_H_


#include "udp_socket.h"




#define RTP_VERSION    2



/*
 * RTP data header
 */
typedef struct {
#ifdef WORDS_BIGENDIAN
	unsigned int v:2;			/* protocol version */
	unsigned int p:1;			/* padding flag */
	unsigned int x:1;			/* header extension flag */
	unsigned int cc:4;			/* CSRC count */
	unsigned int m:1;			/* marker bit */
	unsigned int pt:7;			/* payload type */
#else
	unsigned int cc:4;			/* CSRC count */
	unsigned int x:1;			/* header extension flag */
	unsigned int p:1;			/* padding flag */
	unsigned int v:2;			/* protocol version */
	unsigned int pt:7;			/* payload type */
	unsigned int m:1;			/* marker bit */
#endif
    unsigned int seq:16;		/* sequence number */
    unsigned int ts;			/* timestamp */
    unsigned int ssrc;			/* synchronization source */
   // unsigned int csrc[1];		/* optional CSRC list */
} rtp_hdr_t;


typedef struct {

	int packet_buffer_len;		// Length from head to end
	int payload_buffer_len;		// Length available for payload
	int payload_size;			// Ammount used by payload
	int frame_size;				// Size (in bytes) of a frame of audio
	
	// IP address the packet came from
	char src_ip[INET6_ADDRSTRLEN];
	
	// Is this a valid/available packet
	int valid;

	rtp_hdr_t head;
	char payload;

} rtp_packet_t;




#endif

