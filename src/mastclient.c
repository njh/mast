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
#include "mast_config.h"
#include "mcast_socket.h"
#include "audio.h"
#include "rtp.h"



#define PROGRAM_NAME "mastclient"
#define DEFAULT_DEVICE "/dev/dsp"


/* Global Variables */
config_t *config=NULL;



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
	
	
#ifndef HAVE_OSS
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
		fprintf(stderr, "Error: OSS isn't available - please choose file or stdout.\n\n");
		usage();
	}
	
#ifndef HAVE_SNDFILE
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
	
	
}

#define SOURCE_SWAP_TIMEOUT		(4)
int still_running = 1;




static void
client_termination_handler (int signum)
{
	still_running = 0;
	switch(signum) {
		case SIGTERM:	fprintf(stderr, "Got termination signal.\n"); break;
		case SIGINT:	fprintf(stderr, "Got interupt signal.\n"); break;
	}
	signal(signum, client_termination_handler);
}


void
client_setup_signals()
{
	/* Setup special handling of signals */
	//if (signal(SIGTERM, termination_handler) == SIG_IGN) signal(SIGTERM, SIG_IGN);
	//if (signal(SIGINT, termination_handler) == SIG_IGN)  signal(SIGINT, SIG_IGN);
	//signal(SIGPIPE, SIG_IGN);	

	signal(SIGTERM, client_termination_handler);
	signal(SIGINT, client_termination_handler);
	signal(SIGHUP, client_termination_handler);
}



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

		/* Wait for an RTP packet */
		if (rtp_packet_recv( rtp_socket, packet )<=0) break;
		
		
		
		if (ssrc && ssrc != packet->head.ssrc) {
			if (config->verbose)
				fprintf(stderr, "ignoring packet from another source: 0x%x\n", packet->head.ssrc);
			continue;
		}
		
		/* wait for specific SSRC ? */
		if (config->ssrc && packet->head.ssrc != config->ssrc) {
			if (config->verbose)
				fprintf(stderr, "ignoring packet from non-chosen source: 0x%x\n", packet->head.ssrc);
			continue;
		}

		
		// Time to initalise ?
		if (!ssrc) {
		
			/* Is it the payload we requested ? */
			if (config->payload_type != -1 && 
			    config->payload_type != packet->head.pt)
			{
				if (config->verbose)
					fprintf(stderr, "ignoring packet which isn't of chosen payload type: %d\n", packet->head.pt);
			    continue;
			}
		
			/* Store the payload type and payload size */
			config->payload_type = packet->head.pt;
			config->payload_size = packet->payload_size;
			ssrc = packet->head.ssrc;
		
			audio = audio_open_decoder( config );
			packet->frame_size = audio->encoded_frame_size;

			/* Display info about the stream */
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

			/* Store the time of this successful packet */
			gettimeofday(&last_valid, NULL);


#if 0
			{	
				// Display info
				static unsigned long total_duration=0;
				static unsigned long packet_count=1;
				static int last_usec=0;
	
				fprintf(stderr, "%i.%i: payload_size=%d ts=%u seq=%u diff=%d avrg=%d\n", 
					(int)last_valid.tv_sec, (int)last_valid.tv_usec, 
					packet->payload_size, packet->head.ts, packet->head.seq,
					(int)last_valid.tv_usec-last_usec, (int)(total_duration/packet_count));
					
				if ((last_valid.tv_usec-last_usec) > 0) {
					total_duration += (last_valid.tv_usec-last_usec);
					packet_count++;
				}
				last_usec = last_valid.tv_usec;
			}
#endif
			
			
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




int
main(int argc, char **argv)
{
	mcast_socket_t *mcast_socket = NULL;

	// Set sensible defaults
	config = mast_config_init( );
	

	// Parse the command line
	parse_cmd_line( argc, argv, config );
	
	
	// Create UDP Socket
	mcast_socket = mcast_socket_create( config->ip, config->port, config->ttl, 1 );
	if (!mcast_socket) return 1;
	
	// Configure timeout on socket
	mcast_socket_set_timeout( mcast_socket, config->timeout );
	
	// Display some information
	mast_config_print( config );

	// Catch Signals
	client_setup_signals();
	

	// Run the main process loop
	client_main_loop( &config, mcast_socket );


	// Close UDP socket
	mcast_socket_close( mcast_socket );
	
	// Clean up configuration structure
	mast_config_delete( &config );
	
	
	// Success
	return 0;
}

