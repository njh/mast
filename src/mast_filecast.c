/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2005 University of Southampton
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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sndfile.h>
#include <ortp/ortp.h>

#include "config.h"
#include "mast.h"


#define PROGRAM_NAME "mast_filecast"


/* Global Variables */
int running = TRUE;
int loop_file = FALSE;
char* filename = NULL;




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
	printf( "Input File: %s\n", filename );
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
	printf( "    -l            Loop the audio file\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, RtpSession* session)
{
	char *address = NULL;
	int port = 0;
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "s:t:p:lh?")) != -1)
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
		
		case 'p': {
			int pt = mast_parse_payloadtype( optarg );
			if (pt==-1 || rtp_session_set_send_payload_type( session, pt )) {
				MAST_FATAL("Failed to set payload type");
			}
			break;
		}
		
		case 'l':
			loop_file = TRUE;
		break;
		
		case '?':
		case 'h':
		default:
			usage();
	}


	// Parse the ip address and port
	if (argc > optind) {
		address = argv[optind];
		optind++;
		
		if (argc > optind) {
			port = atoi(argv[optind]);
		} else {
			// Look for port in 
			char* portstr = strchr(address, '/');
			if (portstr && strlen(portstr)>1) {
				*portstr = 0;
				portstr++;
				port = atoi(portstr);
			}
		}
		
		// Use default port if none given
		if (!port) port = DEFAULT_RTP_PORT;
	
	} else {
		MAST_ERROR("missing address/port to send to");
		usage();
	}
	
	// Make sure the port number is even
	if (port%2 == 1) port--;
	

	// Get the input file
	if (argc > optind) {
		filename = argv[optind];
		optind++;
	} else {
		MAST_ERROR("missing audio input filename");
		usage();
	}
	
	// Set the remote address/port
	if (rtp_session_set_remote_addr( session, address, port )) {
		MAST_FATAL("Failed to set remote address/port");
	}
	
}



int main(int argc, char **argv)
{
	RtpSession* session = NULL;
	SNDFILE* file = NULL;
	SF_INFO sfinfo;

	
	// Initialise the oRTP library
	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);

	// Allocate memory for the audio read buffer
	doit();


	// Create RTP session
	session = rtp_session_new(RTP_SESSION_SENDONLY);
	if (session==NULL) {
		MAST_FATAL( "Failed to create oRTP session.\n" );	
	}

	// Parse the command line
	parse_cmd_line( argc, argv, session );

	
	// Open the input file by filename
	memset( &sfinfo, 0, sizeof(sfinfo) );
	file = sf_open(filename, SFM_READ, &sfinfo);
	if (file == NULL) {
		MAST_FATAL("Failed to open input file:\n%s", sf_strerror(NULL));
	}
	

	// Display some information
	//printf( "Local address"
	//printf( "Remote address"
	print_file_info( file, &sfinfo );


	// Configure the RTP session
	rtp_session_set_scheduling_mode(session, TRUE);
	rtp_session_set_blocking_mode(session, TRUE);


	// Allocate memory for the RTP packet structure

	// Setup signal handlers
	mast_setup_signals();


	// The main loop
	while( ((i=fread(buffer,1,160,infile))>0) && mast_still_running() )
	{
		encode_audio();
	
		rtp_session_send_with_ts(session, buffer, i, user_ts);
		user_ts+=160;
	}
	 
	 
	// Close Audio file
	if (sf_close( file )) {
		MAST_ERROR("Failed to close input file:\n%s", sf_strerror(file));
	}
	
	// Close RTP session
	rtp_session_destroy(session);
	ortp_exit();
	
	 
	// Success !
	return 0;
}


