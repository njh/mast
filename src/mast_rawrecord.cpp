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
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include "MPA_Header.h"
#include "MastTool.h"
#include "mast.h"



#define MAST_TOOL_NAME		"mast_rawrecord"


/* Global Variables */
char *g_filename = NULL;			// filename of output file





static int usage()
{
	
	fprintf(stderr, "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	fprintf(stderr, "%s <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
//	printf( "    -s <ssrc>     Source identifier (otherwise use first recieved)\n");
//	printf( "    -t <ttl>      Time to live\n");
//	printf( "    -p <payload>  The payload type to send\n");
	
	exit(1);
	
}



static void parse_cmd_line(int argc, char **argv, MastTool* tool)
{
	int ch;

	// Parse the options/switches
	while ((ch = getopt(argc, argv, "h?")) != -1)
	switch (ch) {

/* may still be useful for RTCP		
		case 't': tool->set_multicast_ttl( optarg ); break;
*/

		case '?':
		case 'h':
		default:
			usage();
	}


	// Parse the ip address and port
	if (argc > optind) {
		tool->set_session_address( argv[optind++] );
	} else {
		MAST_ERROR("missing address/port to send to");
		usage();
	}	

	// Get the output file
	if (argc > optind) {
		g_filename = argv[optind++];
	} else {
		MAST_ERROR("missing audio output filename");
		usage();
	}

}


static FILE* open_output_file( char* filename )
{
	FILE* output = NULL;


	// Open the output file
	if (strcmp(filename, "-")==0) {
		fprintf(stderr, "Output file: STDOUT\n");
		output = stdout;
	} else { 
		fprintf(stderr, "Output file: %s\n", filename);
		output = fopen( filename, "wb" );
	}

	// Check pointer isn't NULL
	if (output==NULL) {
		MAST_FATAL( "failed to open output file: %s", strerror(errno) );
	}
	
	return output;
}



int main(int argc, char **argv)
{
	MastTool* tool = NULL;
	RtpProfile* profile = &av_profile;
	PayloadType* pt = NULL;
	FILE* output = NULL;
	mblk_t* packet = NULL;
	int ts_diff = 0;
	int ts = 0;

	
	// Create an RTP session
	tool = new MastTool( MAST_TOOL_NAME, RTP_SESSION_RECVONLY );
	tool->enable_scheduling();


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, tool );
	

	
	
	// Recieve an initial packet
	packet = tool->wait_for_rtp_packet();
	if (packet == NULL) MAST_FATAL("Failed to receive an initial packet");
	
	// Lookup the payload type
	pt = rtp_profile_get_payload( profile, rtp_get_payload_type( packet ) );
	if (pt == NULL) MAST_FATAL( "Payload type %d isn't registered with oRTP", rtp_get_payload_type( packet ) );
	fprintf(stderr, "Payload type: %s\n", payload_type_get_rtpmap( pt ));
	
	// Work out the duration of the packet
	ts_diff = mast_rtp_packet_duration( packet );
	MAST_DEBUG("ts_diff = %d", ts_diff);


	// Open the output file
	output = open_output_file( g_filename );
	if (output==NULL) MAST_FATAL( "failed to open output file" );
	
	// We can free the packet now
	freemsg( packet );
	


	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( mast_still_running() )
	{

		// Read in a packet
		packet = rtp_session_recvm_with_ts( tool->get_session(), ts );
		if (packet==NULL) {

			MAST_DEBUG( "packet is NULL" );

		} else {
			int data_len = mast_rtp_packet_size( packet );
			if (data_len==0) {
				MAST_WARNING("Failed to get size of packet's payload");
			} else {
				unsigned char* data_ptr = packet->b_cont->b_rptr;
				int bytes_written = 0;
				
				// Skip the extra header for MPA payload
				if (rtp_get_payload_type( packet ) == RTP_MPEG_AUDIO_PT) {
					data_ptr += 4;
					data_len -= 4;
				}
				
				// Update the timestamp difference
				ts_diff = mast_rtp_packet_duration( packet );
				MAST_DEBUG("ts_diff = %d", ts_diff);
				MAST_DEBUG("data_len = %d", data_len);

				// Write to disk
				bytes_written = fwrite( data_ptr, 1, data_len, output );
				if (bytes_written != data_len) {
					MAST_ERROR("Failed to write data to disk: %s", strerror(errno) );
					break;
				}
			}
		}
		
		// Increment the timestamp for the next packet
		ts += ts_diff;
	}


	// Close output file
	if (fclose( output )) {
		MAST_ERROR("Failed to close output file:\n%s", strerror(errno));
	}
	
	// Close RTP session
	delete tool;
	
	
	// Success
	return 0;
}


