/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2007 University of Southampton
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
 *  $Id:$
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <ortp/ortp.h>

#include "config.h"
#include "mast.h"


#define MAST_TOOL_NAME "mast_cast"


/* Global Variables */
int g_loop_file = FALSE;
int g_payload_size_limit = DEFAULT_PAYLOAD_LIMIT;
char* g_payload_type = DEFAULT_PAYLOAD_TYPE;
char* g_filename = NULL;






static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>]\n", MAST_TOOL_NAME);
	printf( "    -s <ssrc>     Source identifier (if unspecified it is random)\n");
	printf( "    -t <ttl>      Time to live\n");
	printf( "    -p <payload>  The payload type to send\n");
	printf( "    -z <size>     Set the per-packet payload size\n");
	printf( "    -d <dcsp>     DCSP Quality of Service value\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, RtpSession* session)
{
	char* remote_address = NULL;
	int remote_port = DEFAULT_RTP_PORT;
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "s:t:p:z:d:lh?")) != -1)
	switch (ch) {
		case 's': {
			// ssrc
			char * ssrc_str = optarg;
			int ssrc;
			
			// Remove 0x from the start of the string
			if (optarg[0] == '0' && (optarg[1] == 'x' || optarg[1] == 'X')) {
				ssrc_str += 2;
			}
			
			// Parse the hexadecimal number into an integer
			if (sscanf(ssrc_str, "%x", &ssrc)<=0) {
				MAST_ERROR("SSRC should be a hexadeicmal number");
				usage();
			}
			
			// Set it in the session
			rtp_session_set_ssrc( session, ssrc );
			break;
		}
		
		case 't':
			if (rtp_session_set_multicast_ttl( session, atoi(optarg) )) {
				MAST_FATAL("Failed to set multicast TTL");
			}
		break;
		
		case 'p':
			g_payload_type = optarg;
		break;

		case 'z':
			g_payload_size_limit = atoi(optarg);
		break;
		
		case 'd':
			if (rtp_session_set_dscp( session, mast_parse_dscp(optarg) )) {
				MAST_FATAL("Failed to set DSCP value");
			}
		break;
		
		case 'l':
			g_loop_file = TRUE;
		break;
		
		case '?':
		case 'h':
		default:
			usage();
	}


	// Parse the ip address and port
	if (argc > optind) {
		remote_address = argv[optind];
		optind++;
		
		// Look for port in the address
		char* portstr = strchr(remote_address, '/');
		if (portstr && strlen(portstr)>1) {
			*portstr = 0;
			portstr++;
			remote_port = atoi(portstr);
		}
	
	} else {
		MAST_ERROR("missing address/port to send to");
		usage();
	}
	
	// Make sure the port number is even
	if (remote_port%2 == 1) remote_port--;
	
	// Set the remote address/port
	if (rtp_session_set_remote_addr( session, remote_address, remote_port )) {
		MAST_FATAL("Failed to set remote address/port (%s/%d)", remote_address, remote_port);
	} else {
		printf( "Remote address: %s/%d\n", remote_address,  remote_port );
	}
	

	// Get the input file
	if (argc > optind) {
		g_filename = argv[optind];
		optind++;
	} else {
		MAST_ERROR("missing audio input filename");
		usage();
	}

}



int main(int argc, char **argv)
{
	RtpSession* session = NULL;
	RtpProfile* profile = &av_profile;
	PayloadType* pt = NULL;
	int bytes_per_frame = 0;
	short *audio_buffer = NULL;
	int frames_per_packet = 0;
	int pt_idx = -1;

	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_SENDONLY );


	// Parse the command line
	parse_cmd_line( argc, argv, session );

	

	// And load a codec for the payload
	


	// Display some information about output stream
	//printf( "Remote address: %s/%d\n", remote_address,  remote_port );
	//printf( "Payload type: %s (pt=%d)\n", mime_type, pt_idx );
	//printf( "Bytes per packet: %d\n", g_payload_size_limit );
	//printf( "Bytes per frame: %d\n", bytes_per_frame );
	//printf( "Frames per packet: %d\n", (int)frames_per_packet );
	printf( "---------------------------------------------------------\n");
	
	// Get the PayloadType structure
	pt = rtp_profile_get_payload( profile, pt_idx );
	if (pt == NULL) MAST_FATAL("Failed to get payload type information from oRTP");

	// Allocate memory for audio and packet buffers
	audio_buffer = (short*)malloc( bytes_per_frame * frames_per_packet );
	if (audio_buffer==NULL) MAST_FATAL("Failed to allocate memory for audio buffer");
	



	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( mast_still_running() )
	{
		//read_audio();

		//encode_audio();
	
		// Send out an RTP packet
		//rtp_session_send_with_ts(session, (uint8_t*)audio_buffer, frames_read*bytes_per_frame, user_ts);
		//user_ts+=frames_read;


			
	}


	 
	// Close RTP session
	mast_deinit_ortp( session );
	
	 
	// Success !
	return 0;
}


