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
char* g_filename = NULL;



static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
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



/*
static void
client_main_loop(config_t* config, mcast_socket_t* rtp_socket)
{
	unsigned int last_seq=0, ssrc = 0;
	rtp_packet_t *packet = NULL;
	struct timeval last_valid;			// Time last valid packet was recieved
	audio_t * audio = NULL;

	// Allocate memory for packet buffers
	packet = rtp_packet_init( config );
	
	
	fprintf(stderr, "Waiting for first packet...\n");

	// Loop foreverish

	while(still_running) {

		// Wait for an RTP packet 
		if (rtp_packet_recv( rtp_socket, packet )<=0) break;
		
		
		
		if (ssrc && ssrc != packet->head.ssrc) {
			if (config->verbose)
				fprintf(stderr, "ignoring packet from another source: 0x%x\n", packet->head.ssrc);
			continue;
		}
		
		// wait for specific SSRC ? 
		if (config->ssrc && packet->head.ssrc != config->ssrc) {
			if (config->verbose)
				fprintf(stderr, "ignoring packet from non-chosen source: 0x%x\n", packet->head.ssrc);
			continue;
		}

		
		// Time to initalise ?
		if (!ssrc) {
		
			// Is it the payload we requested ? 
			if (config->payload_type != -1 && 
			    config->payload_type != packet->head.pt)
			{
				if (config->verbose)
					fprintf(stderr, "ignoring packet which isn't of chosen payload type: %d\n", packet->head.pt);
			    continue;
			}
		
			// Store the payload type and payload size 
			config->payload_type = packet->head.pt;
			config->payload_size = packet->payload_size;
			ssrc = packet->head.ssrc;
		
			audio = audio_open_decoder( config );
			packet->frame_size = audio->encoded_frame_size;

			// Display info about the stream 
			fprintf(stderr, "Src IP: %s\n", packet->src_ip);
			fprintf(stderr, "SSRC: 0x%8.8x\n", ssrc);
			fprintf(stderr, "Payload: %d\n", config->payload_type);
			fprintf(stderr, "Sample Rate: %d Hz\n", audio->samplerate);
			fprintf(stderr, "Channels: %d\n", audio->channels);
			fprintf(stderr, "Size of first payload: %d bytes\n", packet->payload_size);
		}
		
		
		if (packet->head.pt != config->payload_type) {
			if (config->verbose)
				fprintf(stderr, "Error payload changed part way through stream: %d\n", packet->head.pt);
			break;
		}
		
		
		
		if (last_seq && last_seq != packet->head.seq-1 && last_seq!=65535) {
			int diff = abs(packet->head.seq-last_seq);
			int samples = diff * (config->payload_size / audio->encoded_frame_size);
			
			if (config->verbose)
				fprintf(stderr, "packet missing/out of sequence. this=%u last=%u diff=%u lost=%fs\n",
						packet->head.seq, last_seq, diff, samples * ((float)1/audio->samplerate) );
			
			// Decode and output audio
			audio_decode( audio, &packet->payload, packet->payload_size );

		} else {

			// Store the time of this successful packet 
			gettimeofday(&last_valid, NULL);

		}
		
		// Decode and output audio
		audio_decode( audio, &packet->payload, packet->payload_size );


		//if (last_seq < packet->head.seq || last_seq==65535)
				last_seq = packet->head.seq;

	}	// while



	// Free the packet buffers
	rtp_packet_delete( packet );


	audio_close(audio);
}
*/



int
main(int argc, char **argv)
{
	RtpSession* session = NULL;
//	RtpProfile* profile = &av_profile;
	SNDFILE* output = NULL;
	mblk_t* packet = NULL;
	SF_INFO sfinfo;
//	mast_codec_t *codec = NULL;
	int16_t *audio_buffer = NULL;
	char* suffix = NULL;
      int      i,       k, count ;
	int user_ts = 9999;

	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_SENDONLY );


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );
	
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
	


	// Get the first packet, so that we know the format
	for(i=0;i<10000;i++) {
		packet = rtp_session_recvm_with_ts( session, user_ts );
		if (packet == NULL) {
			MAST_WARNING( "failed to read packet" );
		} else {
			mblk_t *body = packet->b_cont;
			int payload_len = (body->b_wptr - body->b_rptr);
			printf("pt=%d, ", rtp_session_get_recv_payload_type( session ) );
			printf("ts=%d, ", rtp_session_get_current_recv_ts( session ) );
			printf("payload_len=%d\n", payload_len );
			user_ts+=160;
		}
		
	}	


	// Open the output file
	//output = sf_open( g_filename, SFM_WRITE, &sfinfo );
	//if (output==NULL) MAST_FATAL( "failed to open output file: %s", sf_strerror(NULL) );


	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( mast_still_running() )
	{

		// Read in a packet
		
		// Decode the audio
		
		// Write to disk

		break;
	}
	

	// Free up the buffers audio/read buffers
	if (audio_buffer) {
		free(audio_buffer);
		audio_buffer=NULL;
	}
	 
	// Close input file
	if (sf_close( output )) {
		MAST_ERROR("Failed to close output file:\n%s", sf_strerror(output));
	}
	
	// Close RTP session
	mast_deinit_ortp( session );
	
	
	// Success
	return 0;
}

