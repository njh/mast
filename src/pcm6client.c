/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003 University of Southampton
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
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include "config.h"
#include "udp_socket.h"
#include "pcm6cast.h"
#include "rtp.h"



#define PROGRAM_NAME "pcm6client"
#define DEFAULT_DEVICE "/dev/dsp"


/* Global Variables */
config_t config;



static void
usage()
{

	fprintf(stderr, "%s [options] <addr>/<port>\n", PROGRAM_NAME);
	fprintf(stderr, "   -s <ssrc> specify ssrc - otherwise use first recieved\n");
	fprintf(stderr, "   -p <payload type> only accept specific payload\n");
//	fprintf(stderr, "   -t <ttl> (for RTCP reports - default 127)\n");
	fprintf(stderr, "   -m <timeout> Timeout in seconds for waiting for packets (0 to disable)\n");
	fprintf(stderr, "   -o send audio to stdout\n");
	fprintf(stderr, "   -f <filename> save stream to file\n");
	fprintf(stderr, "\nVersion: %s\n", VERSION);

	exit(1);
}


static void
parse_cmd_line(int argc, char **argv, config_t* conf)
{
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "hs:t:of:m:p:qd:")) != -1)
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
		case 'd':
			conf->devname = optarg;
		break;
		case 'o':
			conf->use_stdio = 1;
		break;
		case 't':
			conf->ttl = atoi(optarg);
		break;
		case 'm':
			conf->timeout = atoi(optarg);
		break;
		case 'p':
			conf->payload_type = atoi(optarg);
		break;
		case 'q':
			// Be quiet
			conf->verbose = 0;
		break;
		case '?':
		case 'h':
		default:
			usage();
	}
	
	
#ifndef USE_OSS
	if (conf->devname) {
		fprintf(stderr, "Error: Soundcard/OSS support isn't available.\n\n");
		usage();
	}
#else
	if (!conf->use_stdio && !conf->filename) {
		fprintf(stderr, "Using default OSS device: %s\n", DEFAULT_DEVICE);
		conf->devname = DEFAULT_DEVICE;
	}
#endif

	if (!conf->devname && !conf->use_stdio && !conf->filename) {
#ifdef USE_SDL	
		conf->use_sdl = 1;
#else
		fprintf(stderr, "Error: Neither OSS or SDL is available - please choose file or stdout.\n\n");
		usage();
#endif
	}
	
#ifndef USE_SNDFILE
	if(conf->filename) {
		fprintf(stderr, "Error: libsndfile isn't available.\n\n");
		usage();
	}
#endif

	/*** Should check for conflicting command line arguments *****/

	
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
	
	/* Make sure the port number is even */
	if (conf->port%2 == 1) conf->port--;
	
	
#if DEBUG
	fprintf(stderr,"config.ttl=%d\n", conf->ttl);
	fprintf(stderr,"config.port=%d\n", conf->port);
	fprintf(stderr,"config.ip=%s\n", conf->ip);
	fprintf(stderr,"config.ssrc=0x%x\n", conf->ssrc);
	fprintf(stderr,"config.use_stdio=%d\n", conf->use_stdio);
	fprintf(stderr,"config.use_sdl=%d\n", conf->use_sdl);
	fprintf(stderr,"config.devname=%s\n", conf->devname);
	fprintf(stderr,"config.filename=%s\n", conf->filename);
	fprintf(stderr,"config.timeout=%d\n", conf->timeout);
#endif
}




int main(int argc, char **argv)
{
	udp_socket_t *udp_socket = NULL;

	// Set sensible defaults
	client_init_config( &config );


	// Parse the command line
	parse_cmd_line( argc, argv, &config );
	
	
	// Create UDP Socket
	udp_socket = udp_socket_create( config.ip, config.port, config.ttl, 1 );
	udp_socket_set_timeout( udp_socket, config.timeout );
	

	// Catch Signals
	client_setup_signals();
	

	// Run the main process loop
	client_main_loop( &config, udp_socket );


	// Close UDP socket
	udp_socket_close( udp_socket );
	
	
	// Success
	return 0;
}

