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

#include <ortp/ortp.h>

#include "config.h"
#include "codecs.h"
#include "mast_cast-jack.h"
#include "mast.h"


#define MAST_TOOL_NAME	"mast_cast"


/* Global Variables */
char* g_client_name = MAST_TOOL_NAME;
jack_options_t g_client_opt = JackNullOption;
int g_payload_size_limit = DEFAULT_PAYLOAD_LIMIT;
char* g_payload_type = DEFAULT_PAYLOAD_TYPE;






static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>]\n", MAST_TOOL_NAME);
	printf( "    -s <ssrc>     Source identifier in hex (default is random)\n");
	printf( "    -t <ttl>      Time to live\n");
	printf( "    -p <payload>  The payload type to send\n");
	printf( "    -z <size>     Set the per-packet payload size\n");
	printf( "    -d <dscp>     DSCP Quality of Service value\n");
	printf( "    -r <msec>     Ring-buffer duration in milliseconds\n");
	printf( "    -c <channels> Number of audio channels\n");
	printf( "    -n <name>     Name for this JACK client\n");
	printf( "    -a            Auto-connect jack input ports\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, RtpSession* session)
{
	char* remote_address = NULL;
	int remote_port = DEFAULT_RTP_PORT;
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "as:t:p:z:d:c:r:h?")) != -1)
	switch (ch) {
		case 'a':
			g_do_autoconnect = TRUE;
		break;
	
		case 's':
			mast_set_session_ssrc( session, optarg );
		break;
		
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

		case 'c':
			g_channels = atoi(optarg);
		break;
		
		case 'r':
			g_rb_duration = atoi(optarg);
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
		MAST_INFO( "Remote address: %s/%d", remote_address,  remote_port );
	}
	
}



int main(int argc, char **argv)
{
	RtpSession* session = NULL;
	PayloadType* pt = NULL;
	jack_client_t* client = NULL;
	mast_codec_t *codec = NULL;
	int16_t *audio_buffer = NULL;
	u_int8_t *payload_buffer = NULL;
	int samples_per_packet = 0;
	int audio_buffer_size = 0;
	int samplerate = 0;
	int ts = 0;

	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_SENDONLY, FALSE );
	
	// For this session we don't want blocking or scheduling
	rtp_session_set_scheduling_mode(session, FALSE);
	rtp_session_set_blocking_mode(session, FALSE);


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );

	

	// Initialise Jack
	client = mast_init_jack( g_client_name, g_client_opt );
	if (client == NULL) MAST_FATAL( "Failed to initialise JACK client" );
	
	// Get the samplerate of the JACK Router
	samplerate = jack_get_sample_rate( client );
	MAST_INFO( "Samplerate of JACK engine: %d Hz", samplerate );


	// Display some information about the chosen payload type
	MAST_INFO( "Sending SSRC: 0x%x", session->snd.ssrc );
	MAST_INFO( "Payload type: %s/%d/%d", g_payload_type, samplerate, g_channels );


	// Work out the payload type index to use
	pt = mast_choose_payloadtype( session, g_payload_type, samplerate, g_channels );
	if (pt == NULL) MAST_FATAL("Failed to get payload type information from oRTP");
	
	// Load the codec
	codec = mast_init_codec( g_payload_type );
	if (codec == NULL) MAST_FATAL("Failed to get initialise codec" );

	// Calculate the packet size
	samples_per_packet = mast_calc_samples_per_packet( pt, g_payload_size_limit );
	if (samples_per_packet<=0) MAST_FATAL( "Invalid number of samples per packet" );

	// Allocate memory for audio buffer
	audio_buffer_size = samples_per_packet * sizeof(int16_t) * pt->channels;
	audio_buffer = (int16_t*)malloc( audio_buffer_size );
	if (audio_buffer == NULL) MAST_FATAL("Failed to allocate memory for audio buffer");

	// Allocate memory for the packet buffer
	payload_buffer = (u_int8_t*)malloc( g_payload_size_limit );
	if (payload_buffer == NULL) MAST_FATAL("Failed to allocate memory for payload buffer");


	// Setup signal handlers
	mast_setup_signals();



	// The main loop
	while( mast_still_running() )
	{
		size_t payload_bytes = 0;
		size_t bytes_read = 0;

		// Wait for some audio
		if (jack_ringbuffer_read_space(g_ringbuffer) < audio_buffer_size) {
			//MAST_WARNING( "Not enough audio available in ringbuffer; waiting" );

			// Wait for some more audio to become available
			pthread_mutex_lock(&g_ringbuffer_cond_mutex);
			pthread_cond_wait(&g_ringbuffer_cond, &g_ringbuffer_cond_mutex);
			pthread_mutex_unlock(&g_ringbuffer_cond_mutex);
			continue;
		}

		// Copy frames from ring buffer to temporary buffer
		bytes_read = jack_ringbuffer_read(g_ringbuffer, (char*)audio_buffer, audio_buffer_size);
		if (bytes_read<=0) MAST_FATAL( "Failed to read from ringbuffer" );
		if (bytes_read!=audio_buffer_size) MAST_WARNING("Failed to read enough audio for a full packet");
		

		//MAST_DEBUG("Ring buffer is now %u%% full", (jack_ringbuffer_read_space(g_ringbuffer)*100) / g_ringbuffer->size);

		// Encode audio
		payload_bytes = codec->encode(codec,
					(bytes_read / sizeof(int16_t)), audio_buffer, 
					g_payload_size_limit, payload_buffer );
		if (payload_bytes<0)
		{
			MAST_ERROR("Codec encode failed" );
			break;
		}
		
		if (payload_bytes) {
			// Send out an RTP packet
			rtp_session_send_with_ts(session, payload_buffer, payload_bytes, ts);
			ts+= (bytes_read / (sizeof(int16_t)*g_channels));
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
	
	// De-initialise the codec
	if (codec) {
		codec->deinit( codec );
		codec=NULL;
	}

	// Close JACK client
	mast_deinit_jack( client );
	 
	// Close RTP session
	mast_deinit_ortp( session );
	
	
	// Success !
	return 0;
}


