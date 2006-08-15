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

#include <sys/types.h>
#include <sys/time.h>


#include "config.h"
#include "mast_config.h"
#include "mcast_socket.h"
#include "rtp.h"
#include "mpa_header.h"


#define PROGRAM_NAME "mpacast"


/* Global Variables */
config_t *config=NULL;






int usage() {
	
	fprintf(stderr, "%s [options] <addr>/<port> [file1 file2...]\n", PROGRAM_NAME);
	fprintf(stderr, "  -s <ssrc> (if unspecified it is random)\n");
	fprintf(stderr, "  -t <ttl> Time to live (default 5)\n");
	fprintf(stderr, "  -z <payload size> Maximum payload size (1450)\n");
	fprintf(stderr, "  -l loop file\n");
	fprintf(stderr, "%s package version %s.\n", PACKAGE, VERSION);
	
	exit(1);
	
}


static void
parse_cmd_line(int argc, char **argv, config_t* conf)
{
	int ch;

	// Parse the options/switches
	while ((ch = getopt(argc, argv, "hs:t:if:lz:")) != -1)
	switch (ch) {
		case 's': {
			// ssrc
			char * ssrc_str = optarg;
			
			// Remove 0x from the start of the string
			if (optarg[0] == '0' && (optarg[1] == 'x' || optarg[1] == 'X')) {
				ssrc_str += 2;
			}
			
			if (sscanf(ssrc_str, "%x", &conf->ssrc)<=0) {
				fprintf(stderr, "Error: SSRC should be a hexadeicmal number.\n\n");
				usage();
			}
			break;
		}
		case 'f':
			conf->filename = optarg;
		break;
		case 'i':
			conf->use_stdio = 1;
		break;
		case 'l':
			conf->loop_file = 1;
		break;
		case 't':
			conf->ttl = atoi(optarg);
		break;
		case 'z':
			conf->payload_size = atoi(optarg);
		break;
		case '?':
		case 'h':
		default:
			usage();
	}


	if (conf->loop_file && !conf->filename) {
		fprintf(stderr, "Error: Can't loop unless streaming a file.\n");
		usage();
	}
	
	if (!conf->filename && !conf->use_stdio) {
		fprintf(stderr, "Error: no input source selected.\n");
		usage();
	}
	
	
	// Parse the ip address and port
	if (argc > optind) {
		conf->ip = argv[optind];
		optind++;
		
		if (argc > optind) {
			conf->port = atoi(argv[optind]);
		} else {
			// Look for port in 
			char* port = strchr(conf->ip, '/');
			if (port && strlen(port)>1) {
				*port = 0;
				port++;
				conf->port = atoi(port);
			}
		}
		
		if (!conf->port) {
			fprintf(stderr, "Error: missing port parameter.\n\n");
			usage();
		}
	
	} else {
		fprintf(stderr, "Error: missing IP/port.\n\n");
		usage();
	}
	
	// Make sure the port number is even
	if (conf->port%2 == 1) conf->port--;
	
}



static FILE*
open_source( config_t* conf )
{
	FILE* src = NULL;
	
	if (conf->use_stdio) {
		src = stdin;
	} else if (conf->filename) {
		src = fopen( conf->filename, "r" );
		if (!src) perror( "Failed to open file for reading" );
	} else {
		fprintf(stderr, "open_source() failed: not STDIN or filename.\n");
		exit(-1);
	}

	return src;
}

static void
main_loop( mcast_socket_t* rtp_socket, rtp_packet_t* rtp_packet, FILE* src )
{
	mpa_header_t mh;
	unsigned char buf[8192];
	int synced=0;
	
	while(!feof(src)) {
		unsigned int byte = fgetc(src);
		
		if (byte == 0xFF) {
			
			// Read in the following 3 bytes of header
			buf[0] = byte;
			fread( &buf[1], 1, 3, src);


			// Get information about the frame
			if (mpa_header_parse( buf, &mh )) {
			
				// Once we see two valid MPEG Audio frames in a row
				// then we consider ourselves synced
				if (!synced) {
					// ignore the rest of the first frame
					fread( &buf[4], 1, mh.framesize-4, src);

					// read in next header
					fread( buf, 1, 4, src);			
					if (mpa_header_parse( buf, &mh )) {
						#ifdef DEBUG
							mpa_header_print( &mh );
						#endif
						synced = 1;
					}
				}

				
				// If synced then do something with the frame of MPEG audio
				if (synced) {
				
					// Read in the rest of the frame
					fread( &buf[4], 1, mh.framesize-4, src);
					
					// Copy into RTP packet
					memcpy( &rtp_packet->payload, buf, mh.framesize );
					
					// Send the packet
					rtp_packet_send( rtp_socket, rtp_packet, mh.framesize );
					
					fprintf(stderr,".");
				}
				
			} else {
				if (synced) fprintf(stderr, "Lost sync.\n");
				synced=0;
			}
			
		} else {
			if (synced) fprintf(stderr, "Lost sync.\n");
			synced=0;
		}

	}

}


int main(int argc, char **argv)
{
	mcast_socket_t *rtp_socket;
 	rtp_packet_t *rtp_packet;
 	FILE *src = NULL;
	

	// Set sensible defaults
	config = mast_config_init( );
	config->payload_type = 14;	// MPEG Audio
	
	// Parse the command line
	parse_cmd_line( argc, argv, config );

	// Open the input source
	src = open_source( config );

	/* Create and set default values for RTP packet */
	rtp_packet = rtp_packet_init( config );
	//rtp_packet_set_frame_size( rtp_packet, encoded_frame_size );
	 
	// Create multicast sockets
	rtp_socket = mcast_socket_create( config->ip, config->port, config->ttl, 0 );
	if (!rtp_socket) {
		fprintf(stderr, "mcast_socket_create returned NULL\n");
		return -1;
	}



	// Display some information
	mast_config_print( config );
	
	
	
	// Do 'stuff'
	main_loop( rtp_socket, rtp_packet, src );
	
	
	// Clean up
	mcast_socket_close(rtp_socket);
	rtp_packet_delete( rtp_packet );
	mast_config_delete( &config );
	fclose( src );
	
	 
	// Success !
	return 0;
}


