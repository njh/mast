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
#include "audio.h"
#include "pcm6cast.h"
#include "rtp.h"

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


void
client_init_config( config_t* config )
{
	
	config->ttl = 127;
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
	
	// Default is to be verbose
	config->verbose = 1;
	
	// Default is to buffer 5 packets
	config->packet_buf_count = 5;
	
	config->audio_init_callback = NULL;

}

static rtp_packet_t * compare_seq( rtp_packet_t *a, rtp_packet_t *b )
{
	if (a->head.seq < b->head.seq) {
		//fprintf(stderr, "Packet a [%d] is less than packet b [%d]\n", a->head.seq, b->head.seq);
		return a;
	} else {
		//fprintf(stderr, "Packet b [%d] is less than packet a [%d]\n", b->head.seq, a->head.seq);
		return b;
	}
}

void
client_main_loop(config_t* config, udp_socket_t* rtp_socket)
{
	unsigned int last_seq=0, ssrc = 0;
	rtp_packet_t *packet = NULL;
	rtp_packet_t **packet_array = NULL;
	struct timeval last_valid;			// Time last valid packet was recieved
	audio_t * audio = NULL;
	int i;

	// Allocate memory for packet buffers
	packet_array = malloc( config->packet_buf_count * sizeof(packet) );
	for( i=0; i< config->packet_buf_count; i++ ) {
		fprintf(stderr, "Creating packet buffer %i...\n", i);
		packet_array[i] = rtp_packet_init( config );
	}
	
	
	
	fprintf(stderr, "Waiting for first packet...\n");

	// Loop foreverish

	while(still_running) {
	
		/* Look for an invalid packet - to wipe over */
		for( i=0, packet=NULL; i< config->packet_buf_count; i++ ) {
			if (packet_array[i]->valid == 0) {
				packet = packet_array[i];
				fprintf(stderr, "Putting new packet into slot %i.\n",i);
				break;
			}
		}
		
		/* Check we have somewhere to put the packet */
		if (!packet) {
			fprintf(stderr, "Couldn't find space in packet array to recieve new packet.\n");
			break;
		}
		
		/* Wait for an RTP packet */
		if (rtp_packet_recv( rtp_socket, packet )<=0) break;
		fprintf(stderr, "\tNew packet has seq=%i\n", packet->head.seq);
		
		
		
		
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
		
		
		
		
		
		/* ---------------------------------------------------- */
		/* Don't play packets until all slots are full */
		{
			int all_full=1;
			for( i=0; i< config->packet_buf_count; i++ ) {
				if (packet_array[i]->valid==0) {
					all_full=0; break;
				}
			}
		
			if (!all_full) {
				fprintf(stderr, "Not all the slots in the array are full yet.\n");
				continue;
			}
		}
			
		
		
		/* Look for the oldest packet valid packet - to play */
		for( i=0; i< config->packet_buf_count; i++ ) {
			if (packet_array[i]->valid) {
				packet = compare_seq( packet, packet_array[i]);
			}
		}

		
		fprintf(stderr, "\tPlaying packet with seq=%i\n", packet->head.seq);
		
		if (last_seq && last_seq != packet->head.seq-1 && last_seq!=65535) {
		
			if (config->verbose)
				fprintf(stderr, "packet missing/out of sequence. this=%u last=%u diff=%u\n",
						packet->head.seq, last_seq, abs(packet->head.seq-last_seq));
			
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
			
			// Decode and output audio
			audio_decode( audio, &packet->payload, packet->payload_size );
			
		}
		
		
		/* Invalidate the packet we just played */
		packet->valid = 0;
		
		if (last_seq < packet->head.seq || last_seq==65535)	last_seq = packet->head.seq;

	}	// while



	// Free the packet buffers
	for( i=0; i< config->packet_buf_count; i++ ) {
		fprintf(stderr, "Deleting packet buffer %i...\n", i);
		rtp_packet_delete( packet_array[i] );
	}
	


	audio_close(audio);
}

