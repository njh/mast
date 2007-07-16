/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id$
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include <sndfile.h>
#include <ortp/ortp.h>

#include "codecs.h"
#include "mpa_header.h"
#include "mast.h"


#define MAST_TOOL_NAME	"mast_info"



static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s <address>[/<port>]\n", MAST_TOOL_NAME);
//	printf( "    -s <ssrc>     Source identifier (otherwise use first recieved)\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, RtpSession* session)
{
	char* local_address = NULL;
	int local_port = DEFAULT_RTP_PORT;
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "h?")) != -1)
	switch (ch) {
		// case 'T': timeout?
		case '?':
		case 'h':
		default:
			usage();
	}


	// Parse the ip address and port
	if (argc > optind) {
		local_address = argv[optind];
		optind++;
		
		// Look for port in the address
		char* portstr = strchr(local_address, '/');
		if (portstr && strlen(portstr)>1) {
			*portstr = 0;
			portstr++;
			local_port = atoi(portstr);
		}
	
	} else {
		MAST_ERROR("missing address/port to receive from");
		usage();
	}
	
	// Make sure the port number is even
	if (local_port%2 == 1) local_port--;
	
	// Set the remote address/port
	if (rtp_session_set_local_addr( session, local_address, local_port )) {
		MAST_FATAL("Failed to set receive address/port (%s/%u)", local_address, local_port);
	} else {
		printf( "Receive address: %s/%u\n", local_address,  local_port );
	}
	
}





int main(int argc, char **argv)
{
	RtpSession* session = NULL;
	mblk_t* packet = NULL;
	mblk_t* body = NULL;
	RtpProfile* profile = &av_profile;
	PayloadType* pt = NULL;
	int payload_size = 0;

	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_RECVONLY, TRUE );


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );

	

	// Setup signal handlers
	mast_setup_signals();


	// Recieve a single packet
	packet = mast_wait_for_rtp_packet( session, DEFAULT_TIMEOUT );
	if (packet == NULL) MAST_FATAL("Failed to receive a packet");
	body = packet->b_cont;
	payload_size = (body->b_wptr - body->b_rptr);

	
	// Display information about the packet received
	printf("\n");
	printf("RTP Header\n");
	printf("==========\n");
	printf("Payload type    : %u\n", rtp_get_payload_type( packet ) );
	printf("Payload size    : %u bytes\n", payload_size );
	printf("Sequence Number : %u\n", rtp_get_seqnumber( packet ) );
	printf("Timestamp       : %u\n", rtp_get_timestamp( packet ) );
	printf("SSRC Identifier : %x\n", rtp_get_ssrc( packet ) );
	printf("Marker Bit      : %s\n", rtp_get_markbit( packet ) ? "Set" : "Not Set");
	printf("\n");
	

	// Lookup the payload type
	pt = rtp_profile_get_payload( profile, rtp_get_payload_type( packet ) );
	if (pt == NULL) {
		MAST_WARNING( "Payload type %u isn't registered with oRTP", rtp_get_payload_type( packet ) );
	} else {
		const char* mime_major = "?";

		printf("Payload Details\n");
		printf("===============\n");

		if (pt->type==PAYLOAD_AUDIO_CONTINUOUS) mime_major = "audio";
		else if (pt->type==PAYLOAD_AUDIO_PACKETIZED) mime_major = "audio";
		else if (pt->type==PAYLOAD_VIDEO) mime_major = "video";
		printf("Mime Type       : %s/%s\n", mime_major, pt->mime_type);
		
		if (pt->clock_rate)			printf("Cock Rate       : %u Hz\n", pt->clock_rate);
		if (pt->channels)			printf("Channels        : %u\n", pt->channels);
		if (pt->bits_per_sample)	printf("Bits per Sample : %u\n", pt->bits_per_sample);
		if (pt->normal_bitrate) {
			printf("Normal Bitrate  : %u kbps\n", (pt->normal_bitrate/1000));
			printf("Packet duration : %u ms\n", (payload_size*1000)/(pt->normal_bitrate/8) );
		}
		if (pt->recv_fmtp)			printf("Recieve FMTP    : %s\n", pt->recv_fmtp);
		if (pt->send_fmtp)			printf("Send FMTP       : %s\n", pt->send_fmtp);
		printf("\n");
		
	
	}


	// Parse the MPEG Audio header
	if (rtp_get_payload_type( packet ) == RTP_MPEG_AUDIO_PT) {
		/* FIXME: check fragment offset header (see rfc2250) */
		unsigned char* mpa_ptr = body->b_rptr + 4;
		mpa_header_t mh;
	
		printf("MPEG Audio Header\n");
		printf("=================\n");
		
		if (!mpa_header_parse( mpa_ptr, &mh)) {
			MAST_WARNING("Failed to parse MPEG Audio header");
		} else {
			mpa_header_debug( stdout, &mh );
		}
	}
	

	// Close RTP session
	mast_deinit_ortp( session );
	
	 
	// Success !
	return 0;
}


