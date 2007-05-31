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

#include <ortp/ortp.h>


#include "config.h"
#include "mast.h"
#include "mpa_header.h"


#define MAST_TOOL_NAME		"mast_mpacast"
#define READ_BUFFER_SIZE	(8196)


/* Global Variables */
int g_loop_file = FALSE;
int g_payload_size_limit = DEFAULT_PAYLOAD_LIMIT;
char* g_chosen_payload_type = "MPA";
char* g_filename = NULL;



// RTP Payload Type for MPEG Audio
PayloadType	payload_type_mpeg_audio={
	PAYLOAD_AUDIO_PACKETIZED, // type
	90000,	// clock rate
	0,		// bytes per sample N/A
	NULL,	// zero pattern N/A
	0,		// pattern_length N/A
	0,		// normal_bitrate
	"mpa",	// MIME Type
	0		// flags
};




static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
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
			g_chosen_payload_type = optarg;
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
		MAST_ERROR("missing input filename");
		usage();
	}
	

}



static FILE* open_source( char* filename )
{
	FILE* src = NULL;
	
	if (filename==NULL) {
		MAST_FATAL("filename is NULL");
	} else if (strcmp( filename, "-" ) == 0) {
		src = stdin;
	} else {
		src = fopen( filename, "rb" );
	}

	return src;
}


static void main_loop_gsm( RtpSession *session, FILE* input, u_int8_t* buffer )
{
	int synced = 0;
	int timestamp = 0;
	
	/* rfc3551: Blocks of 160 audio samples are compressed into 33 octets, for an
	   effective data rate of 13,200 b/s.
	   Every such buffer begins with a 4 bit signature (0xD)
	*/

	while( mast_still_running() ) {
		int bytes_read = 0;
		int byte = -1;
		
		// Check for EOF
		if (feof( input )) {
			MAST_WARNING( "Reached EOF" );
			if (g_loop_file) {
				if (fseek(input, 0, SEEK_SET)) {
					MAST_ERROR("Failed to seek to start of file: %s", strerror(errno) );
					break;
				}
			
				// Clear the EOF error
				clearerr( input );

			} else {
				break;
			}
		}

		// Check for other error
		if (ferror(input)) {
			MAST_ERROR( "File read error: %s", strerror(errno) );
			break;
		}
	
		// Read a single byte from the input file
		byte = fgetc( input );
		if (byte < 0) continue;
		

		// Check that first nibble is the signature
		if ((byte & 0xF0) == GSM_FRAME_SIGNATURE) {
		
			// Now read in the rest of the frame
			buffer[0] = byte;
			bytes_read = fread( &buffer[1], 1, GSM_FRAME_BYTES-1, input);
			if (bytes_read<0) continue;
			
			// If not synced, then check the byte at the start of the next frame
			if (!synced) {
				byte = fgetc( input );
				if (byte < 0) continue;

				// Again, checked the signature
				if ((byte & 0xF0) == GSM_FRAME_SIGNATURE) {
					synced=1;
					MAST_DEBUG( "Gained GSM Audio sync");
					ungetc(byte, input);
				} else {
					synced=0;
					MAST_DEBUG( "Failed to gain GSM Audio sync");
				}
			}
			
			// FIXME: support multiple GSM frames per packet
			
			// Send out a packet
			if (synced) {
			
				// Send audio payload
				rtp_session_send_with_ts(session, buffer, GSM_FRAME_BYTES, timestamp);
				
				// Frame of GSM audio is 160 samples long
				timestamp += GSM_FRAME_SAMPLES;   //  * frames_per_packet
			}
			
		} else {
			// Lost sync
			if (synced) MAST_WARNING( "Lost GSM Audio sync");
			synced = 0;
		}
				
	}

}

static void main_loop_mpa( RtpSession *session, FILE* input, u_int8_t* buffer )
{
	mpa_header_t mh;
	u_int8_t* mpabuf = buffer+4;
	int timestamp = 0;
	int synced=0;
	
	// Four null bytes at start of buffer
	// which make up the MPEG Audio RTP header
	// (see rfc2250)
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	
	
	while( mast_still_running() ) {
		int bytes_read = 0;
		int byte = -1;
		
		// Check for EOF
		if (feof( input )) {
			MAST_WARNING( "Reached EOF" );
			if (g_loop_file) {
				if (fseek(input, 0, SEEK_SET)) {
					MAST_ERROR("Failed to seek to start of file: %s", strerror(errno) );
					break;
				}
			
				// Clear the EOF error
				clearerr( input );

			} else {
				break;
			}
		}

		// Check for other error
		if (ferror(input)) {
			MAST_ERROR( "File read error: %s", strerror(errno) );
			break;
		}
	
		// Read a single byte from the input file
		byte = fgetc( input );
		if (byte < 0) continue;


		// Check for signature byte
		if (byte == 0xFF) {
			
			// Read in the following 3 bytes of header
			mpabuf[0] = byte;
			bytes_read = fread( &mpabuf[1], 1, 3, input);
			if (bytes_read!=3) continue;


			// Get information about the frame
			if (mpa_header_parse( mpabuf, &mh )) {
			
				// Once we see two valid MPEG Audio frames in a row
				// then we consider ourselves synced
				if (!synced) {
					// ignore the rest of the first frame
					bytes_read = fread( &mpabuf[4], 1, mh.framesize-4, input);
					if (bytes_read!=mh.framesize-4) continue;
					
					// read in next header
					bytes_read = fread( mpabuf, 1, 4, input);
					if (bytes_read!=4) continue;
					
					// Check that the next header is valid too
					if (mpa_header_parse( mpabuf, &mh )) {
						MAST_WARNING( "Gained sync on MPEG audio stream" );
						mpa_header_print( &mh );
						synced = 1;
						
						// Work out how big payload will be
						if (g_payload_size_limit < mh.framesize) {
							MAST_FATAL("audio frame size is bigger than packet size limit");
						}
						
						// FIXME: support multiple frames per packet
						printf("MPEG Audio Frame size: %d bytes\n", mh.framesize );
					}

					
					// FIXME: reimplement so we don't loose first frame
					
					
				}

				
				// If synced then do something with the frame of MPEG audio
				if (synced) {
				
					// Read in the rest of the frame
					fread( &mpabuf[4], 1, mh.framesize-4, input);

					// Send audio payload (plus 4 null bytes at the start)
					rtp_session_send_with_ts(session, buffer, mh.framesize+4, timestamp);
					
					// Timestamp for MPEG Audio is based on fixed 90kHz clock rate
					timestamp += ((mh.samples * 90000) / mh.samplerate);   //  * frames_per_packet

				}
				
			} else {
				if (synced) MAST_WARNING( "Lost MPEG Audio sync");
				synced=0;
			}
			
		} else {
			if (synced) MAST_WARNING( "Lost MPEG Audio sync");
			synced=0;
		}

	}

}



int main(int argc, char **argv)
{
	RtpSession *session = NULL;
	RtpProfile* profile = &av_profile;
	FILE* input = NULL;
	u_int8_t *buffer = NULL;
	
	
	// Create an RTP session
	session = mast_init_ortp( MAST_TOOL_NAME, RTP_SESSION_SENDONLY );


	// Set the MPEG Audio payload type to 14 in the AV profile
	rtp_profile_set_payload(profile, RTP_MPEG_AUDIO_PT, &payload_type_mpeg_audio);

	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, session );
	

	// Open the input source
	input = open_source( g_filename );
	if (!input) {
		MAST_FATAL( "Failed to open input file: %s", strerror(errno) );
	}
	
	// Allocate memory for the read buffer
	buffer = malloc( READ_BUFFER_SIZE );
	if (!buffer) {
		MAST_FATAL( "Failed to allocate memory for the read buffer: %s", strerror(errno) );
	}
	
	// Setup signal handlers
	mast_setup_signals();

	// The main loop
	if (strcasecmp( g_chosen_payload_type, "MPA") == 0) {
	
		if (rtp_session_set_send_payload_type( session, 14 )) {
			MAST_FATAL("Failed to set session payload type index");
		}
	
		main_loop_mpa( session, input, buffer );
		
	} else if (strcasecmp( g_chosen_payload_type, "GSM") == 0) {

		if (rtp_session_set_send_payload_type( session, 3 )) {
			MAST_FATAL("Failed to set session payload type index");
		}
	
		main_loop_gsm( session, input, buffer );
	
	} else {
		MAST_ERROR("Unsuppored raw payload type: %s", g_chosen_payload_type );
	}
	
	// Free the read buffer
	if (buffer) {
		free(buffer);
		buffer=NULL;
	}

	// Close input file
	if (fclose( input )) {
		MAST_ERROR("Failed to close input file:\n%s", strerror(errno) );
	}
	
	// Close RTP session
	mast_deinit_ortp( session );
	
	// Success !
	return 0;
}


