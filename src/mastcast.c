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
#include "mast.h"
#include "rtp.h"
#include "audio.h"
#include "udp_socket.h"


#define PROGRAM_NAME "mastcast"


/* Global Variables */
config_t config;
audio_t *audio = NULL;


static void
init_config( config_t* config )
{
	
	config->ttl = 5;
	config->port = 0;
	config->ip = NULL;
	
	config->timeout = 10;
	config->ssrc = 0;
	config->payload_type = -1;
	config->payload_size = 0;
	
	config->use_stdio = 0;
	config->use_sdl = 0;
	config->loop_file = 0;
	config->devname = NULL;
	config->filename = NULL;
	
	config->audio_init_callback = NULL;

}



/* Return the difference between two times as microseconds */
long
diff_timeval(struct timeval * ep, struct timeval * sp)
{
        if (sp->tv_usec > ep->tv_usec) {
                ep->tv_usec += 1000000;
                --ep->tv_sec;
        }
        return ((ep->tv_sec - sp->tv_sec) * 1000000L) +
            (ep->tv_usec - sp->tv_usec);
} 


/* Add microseconds to a timeval */
void add_usec2timeval( struct timeval * tv, long add )
{
	tv->tv_usec += add;

	/* Don't let usec overflow */
	while (tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
}


static void
main_loop( udp_socket_t* rtp_socket, rtp_packet_t* rtp_packet )
{
	struct timeval due, now;
	int payload_frames = config.payload_size / audio->encoded_frame_size;
	int payload_duration = payload_frames * ((float)1/audio->samplerate * 1000000);
	int delay=0, endcoded_bytes = 0;
	


	// Get the time now...
	gettimeofday(&due, NULL);
 
 
	while(1) {
		
		/* Calculate when the packet is due to be sent*/
		add_usec2timeval( &due, payload_duration );

		/* Read and encode the audio */
		endcoded_bytes = audio_encode( audio, &rtp_packet->payload, config.payload_size );
		
		/* Assume end of file ? */
		if (endcoded_bytes < 1) {
			if (config.loop_file) {
				// Go back to start
				audio_file_rewind( audio );
				continue;
			} else {
				break;
			}
		}


		/* Did we get enough bytes ? */
		if (endcoded_bytes != config.payload_size) {
			fprintf(stderr, "Warning: audio_encode() didn't return enough encoded audio\n");
			/* sending out a smaller packet crashes rat :( */
		}
		
		
		
		/* How long do we wait ?  (if at all) */
        gettimeofday(&now, NULL);
		delay = diff_timeval(&due, &now);
		if (delay > 0) usleep( delay/2 );
		
		
		/* Time to send the packet */
		rtp_packet_send( rtp_socket, rtp_packet, endcoded_bytes );
		

		
		
#if DEBUG
	{
		float fnow, fdue;
		
        gettimeofday(&now, NULL);
		fnow = now.tv_sec + ((float)now.tv_usec / 1000000);
		fdue = due.tv_sec + ((float)due.tv_usec / 1000000);
		
		/*fprintf(stderr, "Sent packet: ts=%u, seq=%u, len=%u\n",
		rtp_timestamp, rtp_seq_num, config.payload_size);

		fprintf( stderr, "samplerate = %d, payload_frames = %d, duration = %d, delay = %ld\n",
				audio->samplerate, payload_frames, (int)( (float)1/audio->samplerate * 1000000 ), delay );
		*/
		fprintf( stderr, "now=[%f] due=[%f] diff=%f duration = %d, delay = %d \n",
				fnow, fdue, fdue-fnow, payload_duration, delay );
	}
			
#endif	
	
		
	}



}


void print_config() {


#if DEBUG
	fprintf(stderr,"config.ttl=%d\n", config.ttl);
	fprintf(stderr,"config.port=%d\n", config.port);
	fprintf(stderr,"config.ip=%s\n", config.ip);
	fprintf(stderr,"config.ssrc=0x%x\n", config.ssrc);
	fprintf(stderr,"config.payload=%d\n", config.payload_type);
	fprintf(stderr,"config.payload_size=%d\n", config.payload_size);
	fprintf(stderr,"config.use_stdio=%d\n", config.use_stdio);
#ifdef USE_OSS
	fprintf(stderr,"config.devname=%s\n", config.devname);
#endif
	fprintf(stderr,"config.filename=%s\n", config.filename);
#endif

	fprintf(stderr,"Audio Rate: %d Hz\n", audio->samplerate);
	fprintf(stderr,"Audio Sample Size: 16 bit\n");
	fprintf(stderr,"Audio Channels: %d\n", audio->channels);
	fprintf(stderr,"RTP Payload Len: %d\n", config.payload_size );
	fprintf(stderr,"RTP Payload Type: %d\n\n", config.payload_type );

}


int usage() {
	
	fprintf(stderr, "%s [options] <addr>/<port>\n", PROGRAM_NAME);
	fprintf(stderr, " -s <ssrc> (if unspecified it is random)\n");
	fprintf(stderr, " -t <ttl> Time to live (default 5)\n");
	fprintf(stderr, " -p <payload type> (default = 10) [0,8,10,11]\n");
	fprintf(stderr, " -z <payload size> (default = (10) = 1280)\n");
	fprintf(stderr, " -f <filename> File to stream\n");
	fprintf(stderr, " -l loop file\n");
	fprintf(stderr, " -i read audio from stdin\n");
#ifdef USE_OSS
	fprintf(stderr, " -d <dev> read audio device\n");
#endif
	fprintf(stderr, "%s package version %s.\n", PACKAGE, VERSION);
	
	exit(1);
	
}


static void
parse_cmd_line(int argc, char **argv, config_t* conf)
{
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "hs:t:if:lp:y:d:z:")) != -1)
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
		case 'i':
			conf->use_stdio = 1;
		break;
		case 'l':
			conf->loop_file = 1;
		break;
		case 't':
			conf->ttl = atoi(optarg);
		break;
		case 'p':
			conf->payload_type = atoi(optarg);
		break;
		case 'z':
			conf->payload_size = atoi(optarg);
		break;
		case '?':
		case 'h':
		default:
			usage();
	}

#ifndef USE_OSS
	if (conf->devname) {
		fprintf(stderr, "Error: OSS soundcard support isn't available.\n");
		usage();
	}
#endif

#ifndef USE_SNDFILE
	if(conf->filename) {
		fprintf(stderr, "Error: libsndfile isn't available.\n\n");
		usage();
	}
#endif
	
	if (conf->loop_file && !conf->filename) {
		fprintf(stderr, "Error: Can't loop unless streaming a file.\n");
		usage();
	}
	
	if (!conf->filename && !conf->devname && !conf->use_stdio) {
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
	
	/* Make sure the port number is even */
	if (conf->port%2 == 1) conf->port--;
	
}



int main(int argc, char **argv)
{
	udp_socket_t *rtp_socket;
 	rtp_packet_t *rtp_packet;
	
	
	init_config( &config );
	
	// Parse the command line
	parse_cmd_line( argc, argv, &config );
	

	/* Open audio file */
	audio = audio_open_encoder( &config );
	

	/* Create and set default values for RTP packet */
	rtp_packet = rtp_packet_init( &config );
	rtp_packet_set_frame_size( rtp_packet, audio->encoded_frame_size );
	 
	// Create multicast sockets
	rtp_socket = udp_socket_create( config.ip, config.port, config.ttl, 0 );
	//rtcp_socket = udp_socket_create( config.ip, config.port+1, config.ttl, 0 );


	// Display some information
	print_config();
	 
	 
	main_loop( rtp_socket, rtp_packet );
	 
	 
	/* Close audio */
	audio_close( audio );
	
	
	udp_socket_close(rtp_socket);
	//udp_socket_close(rtcp_socket);
	
	rtp_packet_delete( rtp_packet );
	 
	// Success !
	return 0;
}


