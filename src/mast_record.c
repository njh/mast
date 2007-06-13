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
#include <getopt.h>

#include <sndfile.h>
#include <ortp/ortp.h>

#include "config.h"
#include "codecs.h"
#include "mast.h"



#define MAST_TOOL_NAME		"mast_record"


/* Global Variables */
char *g_filename = NULL;			// filename of output file



static int usage()
{
	
	fprintf(stderr, "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	fprintf(stderr, "%s [options] <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
//	printf( "    -s <ssrc>     Source identifier (otherwise use first recieved)\n");
//	printf( "    -t <ttl>      Time to live\n");
//	printf( "    -p <payload>  The payload type to send\n");
	
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

/* may still be useful for RTCP		
		case 't':
			if (rtp_session_set_multicast_ttl( session, atoi(optarg) )) {
				MAST_FATAL("Failed to set multicast TTL");
			}
		break;
*/

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
		MAST_ERROR("missing address/port to send to");
		usage();
	}
	
	// Make sure the port number is even
	if (local_port%2 == 1) local_port--;
	
	// Set the remote address/port
	if (rtp_session_set_local_addr( session, local_address, local_port )) {
		MAST_FATAL("Failed to set receive address/port (%s/%d)", local_address, local_port);
	} else {
		printf( "Receive address: %s/%d\n", local_address,  local_port );
	}
	

	// Get the output file
	if (argc > optind) {
		g_filename = argv[optind];
		optind++;
	} else {
		MAST_ERROR("missing audio output filename");
		usage();
	}

}


static SNDFILE* open_output_file( char* filename, PayloadType* pt )
{
	SNDFILE* output = NULL;
	SF_INFO sfinfo;
	char* suffix = NULL;
	int k=0, count=0;

	// Zero the SF_INFO structure
	bzero( &sfinfo, sizeof( sfinfo ) );
	
	// Get the filename suffix
	suffix = strrchr(g_filename, '.') + 1;
	if (suffix==NULL || strlen(suffix)==0) {
		MAST_FATAL("output filename doesn't have a file type extention");
	}

	// Get the number of entries in the simple file format table
	if (sf_command (NULL, SFC_GET_SIMPLE_FORMAT_COUNT, &count, sizeof (int))) {
		MAST_FATAL("failed to get simple output format count");
	}

	// Look at each simple file format in the table
	for (k = 0 ; k < count ; k++)
	{
		SF_FORMAT_INFO	format_info;
		format_info.format = k;
		if (sf_command (NULL, SFC_GET_SIMPLE_FORMAT, &format_info, sizeof (format_info))) {
			MAST_FATAL("failed to get information about simple format %d", k);
		}
		if (strcasecmp( format_info.extension, suffix )==0) {
			// Found match :)
			printf("Output file format: %s\n", format_info.name );
			sfinfo.format = format_info.format;
			break;
		}
	}

	// Check it found something
	if (sfinfo.format == 0x00) {
		MAST_FATAL( "No simple libsndfile format flags defined for file extention '%s'", suffix );
	}
	
	
	// Set the samplerate and number of channels
	sfinfo.channels = pt->channels;
	sfinfo.samplerate = pt->clock_rate;
	

	// Open the output file
	output = sf_open( filename, SFM_WRITE, &sfinfo );
	if (output==NULL) MAST_FATAL( "failed to open output file: %s", sf_strerror(NULL) );

	return output;
}



int main(int argc, char **argv)
{
	RtpSession* session = NULL;
	RtpProfile* profile = &av_profile;
	PayloadType* pt = NULL;
	SNDFILE* output = NULL;
	mblk_t* packet = NULL;
	mblk_t* body = NULL;
	mast_codec_t *codec = NULL;
	int16_t *audio_buffer = NULL;
	int audio_buffer_len = 0;
	int ts_diff = 0;
	int ts = 0;

	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_RECVONLY );


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );
	


	// Recieve an initial packet
	packet = mast_wait_for_rtp_packet( session, DEFAULT_TIMEOUT );
	if (packet == NULL) MAST_FATAL("Failed to receive an initial packet");
	body = packet->b_cont;
	
	// Lookup the payload type
	pt = rtp_profile_get_payload( profile, rtp_get_payload_type( packet ) );
	if (pt == NULL) MAST_FATAL( "Payload type %d isn't registered with oRTP", rtp_get_payload_type( packet ) );
	fprintf(stderr, "Payload type: %s\n", payload_type_get_rtpmap( pt ));
	
	// Work out the duration of the packet
	ts_diff = ((body->b_wptr - body->b_rptr) * pt->clock_rate) / (pt->normal_bitrate/8) ;
	MAST_DEBUG("ts_diff = %d", ts_diff);
	
	// Open the output file
	output = open_output_file( g_filename, pt );
	if (output==NULL) MAST_FATAL( "failed to open output file" );
	
	// We can free the packet now
	freemsg( packet );


	// Load the codec
	codec = mast_init_codec( pt->mime_type );
	if (codec == NULL) MAST_FATAL("Failed to get initialise codec" );

	// Allocate memory for audio buffer
	audio_buffer_len = ts_diff * sizeof(int16_t) * pt->channels;
	audio_buffer = (int16_t*)malloc( audio_buffer_len );
	if (audio_buffer == NULL) MAST_FATAL("Failed to allocate memory for audio buffer");
	

	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( mast_still_running() )
	{

		// Read in a packet
		packet = rtp_session_recvm_with_ts( session, ts );
		if (packet==NULL) {

			MAST_DEBUG( "packet is NULL" );

		} else {
			int samples_decoded = 0;
			int samples_written = 0;
			body = packet->b_cont;
		
			// Decode the audio
			samples_decoded = codec->decode(codec,
						(body->b_wptr - body->b_rptr), body->b_rptr, 
						audio_buffer_len, audio_buffer );
			if (samples_decoded<0)
			{
				MAST_ERROR("Codec decode failed" );
				break;
			}
			
			// Write to disk
			samples_written = sf_write_short( output, audio_buffer, samples_decoded );
			if (samples_written<0) {
				MAST_ERROR("Failed to write audio samples to disk: %s", sf_strerror( output ));
				break;
			}
		}
		

		// Increment the timestamp for the next packet
		ts += ts_diff;
	}
	

	// Free up the buffers audio/read buffers
	if (audio_buffer) {
		free(audio_buffer);
		audio_buffer=NULL;
	}

	// De-initialise the codec
	if (codec) {
		codec->deinit( codec );
		codec=NULL;
	}
	 
	// Close output file
	if (sf_close( output )) {
		MAST_ERROR("Failed to close output file:\n%s", sf_strerror(output));
	}
	
	// Close RTP session
	mast_deinit_ortp( session );
	
	
	// Success
	return 0;
}

