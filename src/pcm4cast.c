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


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include <audiofile.h>

#include "config.h"
#include "random32.h"
#include "pcm6cast.h"


#define DEBUG	1
#define PROGRAM_NAME "pcm6cast"
#define SAMPLE_RATE (44100)


/* Global Variables */

char* audio_file_name = "style.aiff";
int audio_channels = 2;		// Default to stereo
int payload_size = 1280;	// 512-1466 (and devisable by 4)
int port = 5432;
int ttl = 128;
char * ip = "226.35.32.217";


int rtp_packet_len=0;
rtp_packet_t *rtp_packet=NULL;
u_int32_t rtp_seq_num=0;
u_int32_t rtp_timestamp=0;
u_int32_t rtp_frames_per_packet=0;
u_int32_t rtp_ssrc=0;
u_int32_t rtp_packet_count=0;
u_int32_t rtp_octet_count=0;

struct {
	rtcp_common_t sr_common;
	struct {
		u_int32_t ssrc;     /* sender generating this report */
		u_int32_t ntp_sec;  /* NTP timestamp */
		u_int32_t ntp_frac;
		u_int32_t rtp_ts;   /* RTP timestamp */
		u_int32_t psent;    /* packets sent */
		u_int32_t osent;    /* octets sent */ 
		//rtcp_rr_t rr[1];  /* variable-length list */
	} sr;
	
	
	rtcp_common_t sdes_common;
	u_int32_t sdes_ssrc;      /* first SSRC/CSRC */
	rtcp_sdes_item_t sdes_cname;	
	rtcp_sdes_item_t sdes_name;	
	
} rtcp_packet;


void init_rtp_packet( )
{
	// Allocate memory for the packet
	rtp_packet_len = sizeof(rtp_hdr_t) + payload_size;
	rtp_packet = malloc(rtp_packet_len);



	// Initalise the sequence number and timestamp
	rtp_seq_num= random32(0) & 65535;
	rtp_timestamp = random32(1);
	rtp_ssrc = random32(2);

	
	// The standard RTP header
	rtp_packet->head.v=RTP_VERSION;
	rtp_packet->head.p=0;
	rtp_packet->head.x=0;
	rtp_packet->head.cc=0;
	rtp_packet->head.m=1;
	rtp_packet->head.ssrc=rtp_ssrc;
	
	
	/* Set the payload type */
	if (audio_channels==1)		rtp_packet->head.pt = 11;
	else if (audio_channels==2)	rtp_packet->head.pt = 10;
	else {
		fprintf(stderr, "%s: Invalid number of channels: %d\n", audio_channels);
		exit(1);
	}
	
	/* Calculate the number of frames per packet */
	rtp_frames_per_packet = (payload_size/2)/audio_channels;
}


// create a sender socket.
static int
make_rtp_socket(char *szAddr, unsigned short port, int TTL, struct sockaddr_in *sSockAddr)
{
  int          iRet, iLoop = 1;
  struct       sockaddr_in sin;
  char         cTtl = (char)TTL;
  char         cLoop=0;
  unsigned int tempaddr;

  int iSocket = socket( AF_INET, SOCK_DGRAM, 0 );
  if (iSocket < 0) {
    fprintf(stderr,"socket() failed.\n");
    exit(1);
  }

  tempaddr=inet_addr(szAddr);
  sSockAddr->sin_family = sin.sin_family = AF_INET;
  sSockAddr->sin_port = sin.sin_port = htons(port);
  sSockAddr->sin_addr.s_addr = tempaddr;

  iRet = setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, &iLoop, sizeof(int));
  if (iRet < 0) {
    fprintf(stderr,"setsockopt SO_REUSEADDR failed\n");
    exit(1);
  }

  if ((ntohl(tempaddr) >> 28) == 0xe) {
  	fprintf(stderr, "Setting multicast flags...\n");
  
    // only set multicast parameters for multicast destination IPs
    iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_TTL, &cTtl, sizeof(char));
    if (iRet < 0) {
      fprintf(stderr,"setsockopt IP_MULTICAST_TTL failed.  multicast in kernel?\n");
      exit(1);
    }

    cLoop = 1;	// !?
    iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_LOOP,
		      &cLoop, sizeof(char));
    if (iRet < 0) {
      fprintf(stderr,"setsockopt IP_MULTICAST_LOOP failed.  multicast in kernel?\n");
      exit(1);
    }
  }

  return iSocket;
}


int send_rtp_packet( int rtp_socket, struct sockaddr *rtp_si, int audio_frames) {
	unsigned int payload_len = (audio_frames * 2 * audio_channels);
	unsigned int packet_len = payload_len + sizeof(rtp_hdr_t);
	
	
	// Update RTP header fields
	rtp_packet->head.ts=htonl(rtp_timestamp);
	rtp_packet->head.seq=htons(rtp_seq_num);
	
	printf("Sending packet: ts=%u, seq=%u, len=%u (head=%u)\n",
	rtp_timestamp, rtp_seq_num, packet_len, sizeof(rtp_hdr_t));
	
	/* Increment the timestamp and sequence */
	rtp_timestamp+=audio_frames;
	rtp_seq_num++;
	rtp_packet_count++;
	rtp_octet_count += payload_len;
	
	return sendto(rtp_socket, rtp_packet, packet_len, 0, rtp_si, sizeof(*rtp_si));
}

int send_rtcp_packet( int rtcp_socket, struct sockaddr *rtcp_si) {
	int packet_len = sizeof(rtcp_packet.sr_common) + sizeof(rtcp_packet.sr) +
	                 sizeof(rtcp_packet.sdes_common) +
	                 4 + sizeof(rtcp_packet.sdes_cname) + sizeof(rtcp_packet.sdes_name);
	int sr_plen = sizeof(rtcp_packet.sr)/4;
	int sdes_plen = (4+sizeof(rtcp_packet.sdes_cname)+sizeof(rtcp_packet.sdes_name))/4;
	struct timeval now;
	
	fprintf(stderr, "  Sending RTCP packet: len=%d srlen=%d sdes_plen=%d\n", 
				packet_len, sizeof(rtcp_packet.sr), 4+sizeof(rtcp_packet.sdes_cname));

	/* The SR */
	rtcp_packet.sr_common.version = RTP_VERSION;
	rtcp_packet.sr_common.p = 0;
	rtcp_packet.sr_common.count = 0;
	rtcp_packet.sr_common.pt = RTCP_SR;
	rtcp_packet.sr_common.length = sr_plen;
	rtcp_packet.sr.ssrc = htonl(rtp_ssrc);
	
	/* set the NTP fields */
	gettimeofday(&now, NULL);
	rtcp_packet.sr.ntp_sec = htonl(now.tv_sec + NTP_EPOCH_OFFSET);
	rtcp_packet.sr.ntp_frac = htonl((u_int32_t)(((double)(now.tv_usec)*(u_int32_t)(~0))/1000000.0));

	rtcp_packet.sr.rtp_ts = htonl(rtp_timestamp);	/* Supposed to be caculated from NTP ? */
	rtcp_packet.sr.psent = htonl(rtp_packet_count);
	rtcp_packet.sr.osent = htonl(rtp_octet_count);
	
	
	/* The SDES */
	rtcp_packet.sdes_common.version = RTP_VERSION;
	rtcp_packet.sdes_common.p = 0;
	rtcp_packet.sdes_common.count = 2;
	rtcp_packet.sdes_common.pt = RTCP_SDES;
	rtcp_packet.sdes_common.length = sdes_plen;
	rtcp_packet.sdes_ssrc = htonl(rtp_ssrc);
	
	rtcp_packet.sdes_cname.type = RTCP_SDES_CNAME;
	rtcp_packet.sdes_cname.length = 30;
	memcpy(rtcp_packet.sdes_cname.data, "njh@hwickabab.ecs.soton.ac.uk.", 30);
	
	rtcp_packet.sdes_name.type = RTCP_SDES_NAME;
	rtcp_packet.sdes_name.length = 30;
	memcpy(rtcp_packet.sdes_name.data,  "Mr   Nicholas   James  Humfrey", 30);
	
	
	
	return sendto(rtcp_socket, &rtcp_packet, packet_len, 0, rtcp_si, sizeof(*rtcp_si));
}


AFfilehandle open_audio_file(char* file_name)
{
	AFfilehandle input_file;
	int sampleFormat;
	int audio_bits;

	/* No file name passed in ? */
	if (!file_name) return AF_NULL_FILEHANDLE;


	/* Open audio file */
	input_file = afOpenFile(file_name, "r", AF_NULL_FILESETUP);
	
	if (input_file == AF_NULL_FILEHANDLE) {
		fprintf(stderr, "%s: Cannot open audio file: %s.\n",
		PROGRAM_NAME, file_name);
		exit(1);
	}
	
	/* Check the sample rate */
	if (afGetRate(input_file, AF_DEFAULT_TRACK) != SAMPLE_RATE) {
		fprintf(stderr, "%s: The audio must have a sample rate of 44.1 kHz.\n", PROGRAM_NAME);
		exit(1);
	}
		
	
	/* Make sure it is a sample format we support */
	afGetSampleFormat(input_file, AF_DEFAULT_TRACK, &sampleFormat, &audio_bits);
	if (sampleFormat != AF_SAMPFMT_TWOSCOMP) {
		fprintf(stderr, "%s: Sorry the only supported sample format is twos complement.\n", PROGRAM_NAME);
		exit(1);
	}
	
	/* Check the sample size */
	if (audio_bits != 16) {
		fprintf(stderr, "%s: The audio must have a sample size of 16bits.\n", PROGRAM_NAME);
		exit(1);
	}

	audio_channels = afGetChannels(input_file, AF_DEFAULT_TRACK);
	

	return input_file;
}




void print_config() {

	printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
	printf("Sample Size: 16 bit\n");
	printf("Channels: %d\n\n", audio_channels);
	printf("RTP Packet Len: %d\n", rtp_packet_len );
	printf("Payload Type: %d\n", rtp_packet->head.pt );
	printf("Frames per packet: %d\n", rtp_frames_per_packet );

}



/* Return the difference between two times as microseconds */
long
diff_timeval(struct timeval * ep, struct timeval * sp)
{
        if (sp->tv_usec > ep->tv_usec) {
                ep->tv_usec += 1000000;
                --ep->tv_sec;
        }
        return ((ep->tv_sec - sp->tv_sec) * 1000000L) +
            (ep->tv_usec - sp->tv_usec);
} 


/* Add microseconds to a timeval */
void add_usec2timeval( struct timeval * tv, long add )
{
	tv->tv_usec += add;

	/* Don't let usec overflow */
	if (tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
}



int main() {
	int rtp_socket = 0, rtcp_socket;
	struct sockaddr_in rtp_si;
	struct sockaddr_in rtcp_si;
 	AFfilehandle input_file;
	struct timeval due, now;
	int frame_count;

	
	
	/* Make sure the port number is even */
	if (port%2 == 1) port--;
	
	input_file = open_audio_file( audio_file_name );
	 
	 
	init_rtp_packet( );
	
	 
	// Create an RTP multicast socket
	rtp_socket = make_rtp_socket(ip, port, ttl, &rtp_si);
	if (!rtp_socket) {
		fprintf(stderr, "Failed to create a RTP socket.\n");
		exit(1);
	}
	 
	// Create an RTCP multicast socket
	rtcp_socket = make_rtp_socket(ip, port+1, ttl, &rtcp_si);
	if (!rtcp_socket) {
		fprintf(stderr, "Failed to create a RTCP socket.\n");
		exit(1);
	}


	if (DEBUG) print_config();
	 
	
	// Get the time now...
	gettimeofday(&due, NULL);


	while(1) {
		int frames_read = afReadFrames(input_file, AF_DEFAULT_TRACK, &rtp_packet->payload, rtp_frames_per_packet);
		long delay;


		/* Calculate when the packet is due to be sent*/
		add_usec2timeval( &due, frames_read * (float)1/44100 * 1000000 );
		
		/* How long do we wait ?  (if at all) */
        gettimeofday(&now, NULL);
		delay = diff_timeval(&due, &now);
		if (delay > 0) usleep( delay );
		
		
		/* Zero any memory after the read size */
		if (frames_read != rtp_frames_per_packet) {
			memset( &rtp_packet->payload+frames_read, 0, rtp_frames_per_packet-frames_read);
			
			/* sending out a smaller packet crashes rat :( */
		}
		
		
		/* Time to send the packet */
		send_rtp_packet( rtp_socket, &rtp_si, rtp_frames_per_packet /*frames_read*/ );
		
		if (frames_read != rtp_frames_per_packet) {
			// Assume end of file.
			break;
		}
		
		/* Sent an RTCP packet every 5 seconds or so */
		frame_count += frames_read;
		if (frame_count > 220500) {
			send_rtcp_packet( rtcp_socket, &rtcp_si );
			frame_count = 0;
		}

	}
	 
	 
	/* Close the input file if it is open */
	if (input_file) {
		afCloseFile( input_file );
		input_file = AF_NULL_FILEHANDLE;
	}
	 
}