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

#include <sndfile.h>

#include "MastSendTool.h"
#include "mast.h"


#define MAST_TOOL_NAME	"mast_filecast"


/* Global Variables */
SNDFILE *g_input_file = NULL;
int g_loop_file = FALSE;
char* g_filename = NULL;




/* 
  format_duration_string() 
  Create human readable duration string from libsndfile info
*/
static char* format_duration_string( SF_INFO *sfinfo )
{
	float seconds;
	int minutes;
	char *string = (char*)malloc( STR_BUF_SIZE );
	
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
  mast_fill_input_buffer()
  Make sure input buffer if full of audio  
*/
size_t mast_fill_input_buffer( MastAudioBuffer* buffer )
{
	size_t total_read = 0;
	int frames_wanted = buffer->get_write_space();
	int frames_read = 0;

	if (frames_wanted==0) {
		// No audio wanted
		MAST_DEBUG( "Tried to fill buffer when it is full" );
		return 0;
	}
	
	// Loop until buffer is full
	while( frames_wanted > 0 ) {
		frames_wanted = buffer->get_write_space();
		frames_read = sf_readf_float( g_input_file, buffer->get_write_ptr(), frames_wanted );
		
		// Add on the frames read
		if (frames_read > 0) {
			buffer->add_frames( frames_read );
			total_read += frames_read;
		}
		
		// Reached end of file?
		if (frames_read < frames_wanted) {
			MAST_DEBUG("Reached end of file (frames_wanted=%d, frames_read=%d)", frames_wanted, frames_read);
			if (g_loop_file) {
				// Seek back to the beginning
				if (sf_seek( g_input_file, 0, SEEK_SET )) {
					MAST_ERROR("Failed to seek to start of file: %s", sf_strerror( g_input_file ) );
					return 0;
				}
			} else {
				// Reached end of file (and don't want to loop)
				break;
			}
		}
	}
		
	if (total_read < 0) {
		MAST_ERROR("Failed to read from file: %s", sf_strerror( g_input_file ) );
	}

	return total_read;
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
	printf( "%s [options] <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
	printf( "    -s <ssrc>       Source identifier in hex (default is random)\n");
	printf( "    -t <ttl>        Time to live\n");
	printf( "    -p <payload>    The payload mime type to send\n");
	printf( "    -o <name=value> Set codec parameter / option\n");
	printf( "    -z <size>       Set the per-packet payload size\n");
	printf( "    -d <dscp>       DSCP Quality of Service value\n");
	printf( "    -l              Loop the audio file\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, MastSendTool* tool)
{
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "s:t:p:o:z:d:lh?")) != -1)
	switch (ch) {
		case 's': tool->set_session_ssrc( optarg ); break;
		case 't': tool->set_multicast_ttl( optarg ); break;
		case 'p': tool->set_payload_mimetype( optarg ); break;
		case 'o': tool->set_payload_mimetype_param( optarg ); break;
		case 'z': tool->set_payload_size_limit( optarg ); break;
		case 'd': tool->set_session_dscp( optarg ); break;

		case 'l': g_loop_file = TRUE; break;
		
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
	

	// Get the input file
	if (argc > optind) {
		g_filename = argv[optind++];
	} else {
		MAST_ERROR("missing audio input filename");
		usage();
	}

}



int main(int argc, char **argv)
{
	MastSendTool *tool = NULL;
	SF_INFO sfinfo;


	// Create the send tool object
	tool = new MastSendTool( MAST_TOOL_NAME );
	tool->enable_scheduling();

	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, tool );

	// Open the input file by filename
	memset( &sfinfo, 0, sizeof(sfinfo) );
	g_input_file = sf_open(g_filename, SFM_READ, &sfinfo);
	if (g_input_file == NULL) MAST_FATAL("Failed to open input file:\n%s", sf_strerror(NULL));
	tool->set_input_channels( sfinfo.channels );
	tool->set_input_samplerate( sfinfo.samplerate );
	
	// Display some information about the input file
	print_file_info( g_input_file, &sfinfo );
	
	// Setup signal handlers
	mast_setup_signals();

	// Run the main loop
	tool->run();
	
	// Clean up
	delete tool;
	

	// Close input file
	if (sf_close( g_input_file )) {
		MAST_ERROR("Failed to close input file:\n%s", sf_strerror(g_input_file));
	}

	
	// Success !
	return 0;
}


