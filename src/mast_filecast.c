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


#define PROGRAM_NAME "mast_filecast"


/* Global Variables */
int g_loop_file = FALSE;
int g_payload_size_limit = DEFAULT_PAYLOAD_LIMIT;
char* g_payload_type = DEFAULT_PAYLOAD_TYPE;
char* g_filename = NULL;




/* 
  format_duration_string() 
  Create human readable duration string from libsndfile info
*/
static char* format_duration_string( SF_INFO *sfinfo )
{
	float seconds;
	int minutes;
	char * string = malloc( STR_BUF_SIZE );
	
	if (sfinfo->frames==0 || sfinfo->samplerate==0) {
		snprintf( string, STR_BUF_SIZE, "Unknown" );
		return string;
	}
	
	// Calculate the number of minutes and seconds
	seconds = sfinfo->frames / sfinfo->samplerate;
	minutes = (seconds / 60 );
	seconds -= (minutes * 60);

	// Create a string out of it
	snprintf( string, STR_BUF_SIZE, "%imin %1.1fsec", minutes, seconds);

	return string;
}


/* 
  print_file_info() 
  Display information about input and output files
*/
static void print_file_info( SNDFILE *inputfile, SF_INFO *sfinfo )
{
	SF_FORMAT_INFO format_info;
	SF_FORMAT_INFO subformat_info;
	char sndlibver[128];
	char *duration = NULL;
	
	// Get the format
	format_info.format = sfinfo->format & SF_FORMAT_TYPEMASK;
	sf_command (inputfile, SFC_GET_FORMAT_INFO, &format_info, sizeof(format_info)) ;

	// Get the sub-format info
	subformat_info.format = sfinfo->format & SF_FORMAT_SUBMASK;
	sf_command (inputfile, SFC_GET_FORMAT_INFO, &subformat_info, sizeof(subformat_info)) ;

	// Get the version of libsndfile
	sf_command (NULL, SFC_GET_LIB_VERSION, sndlibver, sizeof(sndlibver));

	// Get human readable duration of the input file
	duration = format_duration_string( sfinfo );

	printf( "---------------------------------------------------------\n");
	printf( "%s (http://www.mega-nerd.com/libsndfile/)\n", sndlibver);
	printf( "Input File: %s\n", g_filename );
	printf( "Input Format: %s, %s\n", format_info.name, subformat_info.name );
	printf( "Input Sample Rate: %d Hz\n", sfinfo->samplerate );
	if (sfinfo->channels == 1) printf( "Input Channels: Mono\n" );
	else if (sfinfo->channels == 2) printf( "Input Channels: Stereo\n" );
	else printf( "Input Channels: %d\n", sfinfo->channels );
	printf( "Input Duration: %s\n", duration );
	printf( "---------------------------------------------------------\n");
	
	free( duration );
}


static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>] <filename>\n", PROGRAM_NAME);
	printf( "    -s <ssrc>     Source identifier (if unspecified it is random)\n");
	printf( "    -t <ttl>      Time to live\n");
	printf( "    -p <payload>  The payload type to send\n");
	printf( "    -z <size>     Set the per-packet payload size\n");
	printf( "    -d <dcsp>     DCSP Quality of Service value\n");
	printf( "    -l            Loop the audio file\n");
	
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
	SNDFILE* input = NULL;
	SF_INFO sfinfo;
	mast_codec_t *codec = NULL;
	PayloadType* pt = NULL;
	int frames_per_packet = 0;
	int16_t *audio_buffer = NULL;
	u_int8_t *payload_buffer = NULL;
	int user_ts = 0;
	int pt_idx = -1;

	
	// Create an RTP session
	session = mast_init_ortp( RTP_SESSION_SENDONLY );


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );

	
	// Open the input file by filename
	memset( &sfinfo, 0, sizeof(sfinfo) );
	input = sf_open(g_filename, SFM_READ, &sfinfo);
	if (input == NULL) MAST_FATAL("Failed to open input file:\n%s", sf_strerror(NULL));

	
	// Display some information about the input file
	print_file_info( input, &sfinfo );
	
	// Display some information about the chosen payload type
	printf( "Sending SSRC: %x\n", session->snd.ssrc );
	printf( "Payload type: %s/%d/%d\n", g_payload_type, sfinfo.samplerate, sfinfo.channels );


	// Work out the payload type index to use
	pt_idx = rtp_profile_find_payload_number( profile, g_payload_type, sfinfo.samplerate, sfinfo.channels );
	if ( pt_idx<0 ) MAST_FATAL("Failed to find static payload type index");
	printf( "Payload type index: %d\n", pt_idx );


	// Get the PayloadType structure
	pt = rtp_profile_get_payload( profile, pt_idx );
	if (pt == NULL) MAST_FATAL("Failed to get payload type information from oRTP");


	// Set the payload type in the session
	if (rtp_session_set_send_payload_type( session, pt_idx )) {
		MAST_FATAL("Failed to set session payload type index");
	}
	
	// Load the codec
	codec = mast_init_codec( g_payload_type );
	if (codec == NULL) MAST_FATAL("Failed to get initialise codec" );


	// Calculate how much audio goes in each packet
	if (pt->type == PAYLOAD_AUDIO_CONTINUOUS) {
		int bytes_per_frame, bytes_per_unit;
		bytes_per_frame = pt->bits_per_sample / 8;
		printf( "Bytes per sample: %d\n", bytes_per_frame );
		if (bytes_per_frame<=0) MAST_FATAL( "Invalid number of bytes per frame" );
		bytes_per_unit = bytes_per_frame * FRAMES_PER_UNIT;
		frames_per_packet = (g_payload_size_limit / bytes_per_unit) * FRAMES_PER_UNIT;
		printf( "Packet size: %d bytes\n", (frames_per_packet * bytes_per_frame) );
	
	} else if (strcmp(pt->mime_type, "GSM")==0) {
	
		// FIXME: clean this code up a bit
		frames_per_packet = (g_payload_size_limit / GSM_FRAME_BYTES) * GSM_FRAME_SAMPLES;
		printf( "Packet size: %d bytes\n", (frames_per_packet/GSM_FRAME_SAMPLES)*GSM_FRAME_BYTES );

	} else {
		MAST_FATAL("Only PAYLOAD_AUDIO_CONTINUOUS is currently supported");
	}


	printf( "Audio samples per packet: %d\n", frames_per_packet );
	printf( "Packet duration: %d ms\n", (frames_per_packet*1000 / pt->clock_rate));
	printf( "---------------------------------------------------------\n");
	if (frames_per_packet<=0) MAST_FATAL( "Invalid number of samples per packet" );


	// Allocate memory for audio buffer
	audio_buffer = (int16_t*)malloc( frames_per_packet * sizeof(int16_t) * pt->channels );
	if (audio_buffer == NULL) MAST_FATAL("Failed to allocate memory for audio buffer");

	// Allocate memory for the packet buffer
	payload_buffer = (u_int8_t*)malloc( g_payload_size_limit );
	if (payload_buffer == NULL) MAST_FATAL("Failed to allocate memory for payload buffer");
	


	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( mast_still_running() )
	{
		int frames_read = sf_readf_short( input, audio_buffer, frames_per_packet );
		int payload_bytes = 0;
		
		// Was there an error?
		if (frames_read < 0) {
			MAST_ERROR("Failed to read from file: %s", sf_strerror( input ) );
			break;
		}
		
		// Encode audio
		payload_bytes = codec->encode(codec,
					frames_read*pt->channels, audio_buffer, 
					g_payload_size_limit, payload_buffer );
		if (payload_bytes == -1)
		{
			MAST_ERROR("Codec encode failed" );
			break;
		}
		
	
		if (payload_bytes) {
			// Send out an RTP packet
			rtp_session_send_with_ts(session, payload_buffer, payload_bytes, user_ts);
			user_ts+=frames_read;
		}
		

		// Reached end of file?
		if (frames_read < frames_per_packet) {
			MAST_DEBUG("Reached end of file (frames_read=%d)", frames_read);
			if (g_loop_file) {
				// Seek back to the beginning
				if (sf_seek( input, 0, SEEK_SET )) {
					MAST_ERROR("Failed to seek to start of file: %s", sf_strerror( input ) );
					break;
				}
			} else {
				// End of file, exit main loop
				break;
			}
		}
		
	}

	// Free up the buffers audio/read buffers
	if (payload_buffer) {
		free(payload_buffer);
		payload_buffer=NULL;
	}
	if (audio_buffer) {
		free(audio_buffer);
		audio_buffer=NULL;
	}
	 
	// Close input file
	if (sf_close( input )) {
		MAST_ERROR("Failed to close input file:\n%s", sf_strerror(input));
	}
	
	// Close RTP session
	mast_deinit_ortp( session );
	
	 
	// Success !
	return 0;
}


