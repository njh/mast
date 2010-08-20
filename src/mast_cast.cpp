/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
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

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include "MastSendTool.h"
#include "mast.h"


#define MAST_TOOL_NAME	"mast_cast"


/* Global Variables */
jack_options_t g_client_opt = JackNullOption;
int g_do_autoconnect = FALSE;
int g_rb_duration = DEFAULT_RINGBUFFER_DURATION;
jack_port_t *g_jackport[2] = {NULL, NULL};
jack_ringbuffer_t *g_ringbuffer = NULL;
jack_default_audio_sample_t *g_interleavebuf = NULL;

/* For signaling when there's room in the ringbuffer. */
pthread_mutex_t g_ringbuffer_cond_mutex;
pthread_cond_t g_ringbuffer_cond;



/* Callback called by JACK when audio is available
   Use as little CPU time as possible, just copy accross the audio
   into the ring buffer
*/
static int process_callback(jack_nframes_t nframes, void *arg)
{
	MastSendTool* tool = (MastSendTool*)arg;
	unsigned int channels = tool->get_input_channels();
    size_t to_write = 0, written = 0;
	unsigned int c,n;
	
	// Process channel by channel
	for (c=0; c < channels; c++)
	{	
        jack_default_audio_sample_t *buf = (jack_default_audio_sample_t*)
        	jack_port_get_buffer(g_jackport[c], nframes);
 
		// Interleave the left and right channels
		for(n=0; n<nframes; n++) {
			g_interleavebuf[(n*channels)+c] = buf[n];
		}
	}

	// Now write the interleaved audio to the ring buffer
	to_write = sizeof(float) * nframes * channels;
	written = jack_ringbuffer_write(g_ringbuffer, (char*)g_interleavebuf, to_write);
	if (to_write > written) {
		// If this goes wrong, then the buffer goes out of sync and we get static
		MAST_FATAL("Failed to write to ring ruffer, try increading the ring-buffer size");
		return 1;
	}
	
	// Signal the other thread that audio is available
	pthread_cond_signal(&g_ringbuffer_cond);

	// Success
	return 0;
}
					

// Callback called by JACK when buffersize changes
static int buffersize_callback(jack_nframes_t nframes, void *arg)
{
	MastSendTool* tool = (MastSendTool*)arg;
	int channels = tool->get_input_channels();
	MAST_DEBUG("JACK buffer size is %d samples long", nframes);

	// (re-)allocate conversion buffer
	g_interleavebuf = (jack_default_audio_sample_t*)
		realloc( g_interleavebuf, nframes * sizeof(float) * channels );
	if (g_interleavebuf == NULL) {
		MAST_FATAL("Failed to (re-)allocate the convertion buffer");
	}

	// Success
	return 0;
}



/*
  mast_fill_input_buffer()
  Make sure input buffer if full of audio  
*/
size_t mast_fill_input_buffer( MastAudioBuffer* buffer )
{
	int frames_wanted = buffer->get_write_space();
	size_t bytes_wanted = frames_wanted * buffer->get_channels() * sizeof( mast_sample_t );
	size_t bytes_read = 0, frames_read = 0;



	// Keep checking that there is enough audio available
	while (jack_ringbuffer_read_space(g_ringbuffer) < bytes_wanted) {
		MAST_WARNING( "Not enough audio available in ringbuffer; waiting" );
		//MAST_DEBUG("Ring buffer is %u%% full", (jack_ringbuffer_read_space(g_ringbuffer)*100) / g_ringbuffer->size);

		// Wait for some more audio to become available
		pthread_mutex_lock(&g_ringbuffer_cond_mutex);
		pthread_cond_wait(&g_ringbuffer_cond, &g_ringbuffer_cond_mutex);
		pthread_mutex_unlock(&g_ringbuffer_cond_mutex);
	}

	// Copy frames from ring buffer to temporary buffer
	bytes_read = jack_ringbuffer_read(g_ringbuffer, (char*)buffer->get_write_ptr(), bytes_wanted);
	if (bytes_read<=0) MAST_FATAL( "Failed to read from ringbuffer" );
	if (bytes_read!=bytes_wanted) MAST_WARNING("Failed to read enough audio for a full packet");
	
	// Mark the space in the buffer as used
	frames_read = bytes_read / (buffer->get_channels() * sizeof( mast_sample_t ));
	buffer->add_frames( frames_read );

	// Return the number 
	return frames_read;
}

// Callback called by JACK when jackd is shutting down
static void shutdown_callback(void *arg)
{
	MAST_WARNING("MAST quitting because jackd is shutting down" );
	
	// Signal the main thead to stop
	mast_stop_running();
}



// Crude way of automatically connecting up jack ports
static void autoconnect_jack_ports( jack_client_t* client, int port_count )
{
	const char **all_ports;
	int ch=0, err = 0;
	int i;

	// Get a list of all the jack ports
	all_ports = jack_get_ports(client, NULL, NULL, JackPortIsOutput);
	if (!all_ports) {
		MAST_FATAL("autoconnect_jack_ports(): jack_get_ports() returned NULL");
	}
	
	// Step through each port name
	for (i = 0; all_ports[i]; ++i) {
		const char* local_port = jack_port_name( g_jackport[ch] );
		
		// Connect the port
		MAST_INFO("Connecting '%s' => '%s'", all_ports[i], local_port);
		err = jack_connect(client, all_ports[i], local_port);
		if (err != 0) MAST_FATAL("connect_jack_port(): failed to jack_connect() ports: %d", err);
		
		// Found enough ports ?
		if (++ch >= port_count) break;
	}
	
	free( all_ports );
}


// Initialise Jack related stuff
static jack_client_t* init_jack( MastSendTool* tool ) 
{
	const char* client_name = tool->get_tool_name();
	jack_client_t* client = NULL;
	jack_status_t status;
	size_t ringbuffer_size = 0;
	int port_count = tool->get_input_channels();
	int i = 0;


    pthread_mutex_init(&g_ringbuffer_cond_mutex, NULL);
    pthread_cond_init(&g_ringbuffer_cond, NULL);

	// Register with Jack
	if ((client = jack_client_open(client_name, g_client_opt, &status)) == 0) {
		MAST_ERROR("Failed to start jack client: 0x%x", status);
		return NULL;
	} else {
		MAST_INFO( "JACK client registered as '%s'", jack_get_client_name( client ) );
	}

	// Create our input port(s)
	if (port_count==1) {
		if (!(g_jackport[0] = jack_port_register(client, "mono", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))) {
			MAST_ERROR("Cannot register mono input port");
			return NULL;
		}
	} else {
		if (!(g_jackport[0] = jack_port_register(client, "left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))) {
			MAST_ERROR("Cannot register left input port");
			return NULL;
		}
		
		if (!(g_jackport[1] = jack_port_register(client, "right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))) {
			MAST_ERROR( "Cannot register left input port");
			return NULL;
		}
	}
	
	// Create ring buffer
	ringbuffer_size = jack_get_sample_rate( client ) * g_rb_duration * port_count * sizeof(int16_t) / 1000;
	MAST_INFO("Duration of the ring buffer is %d ms (%d bytes)", g_rb_duration, (int)ringbuffer_size );
	if (!(g_ringbuffer = jack_ringbuffer_create( ringbuffer_size ))) {
		MAST_ERROR("Cannot create ringbuffer %d", i);
		return NULL;
	}

	// Register callbacks
	jack_on_shutdown(client, shutdown_callback, tool );
	jack_set_buffer_size_callback( client, buffersize_callback, tool);
	jack_set_process_callback(client, process_callback, tool);
	
	// Create conversion buffer
	buffersize_callback( jack_get_buffer_size(client), tool );

	// Activate JACK
	if (jack_activate(client)) MAST_FATAL("Cannot activate JACK client");
	
	/* Auto connect ports ? */
	if (g_do_autoconnect) autoconnect_jack_ports( client, tool->get_input_channels() );
	
	return client;
}



// Shut down jack related stuff
static void deinit_jack( jack_client_t *client )
{
	// Leave the Jack graph
	if (jack_client_close(client)) {
		MAST_WARNING("Failed to close jack client");
	}
	
	// Free the coversion buffer
	if (g_interleavebuf) {
		free( g_interleavebuf );
		g_interleavebuf = NULL;
	}
	
	// Free up the ring buffer
	if (g_ringbuffer) {
		jack_ringbuffer_free( g_ringbuffer );
		g_ringbuffer = NULL;
	}

    pthread_cond_destroy(&g_ringbuffer_cond);
    pthread_mutex_destroy(&g_ringbuffer_cond_mutex);

}





static int usage() {
	
	printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
	printf( "%s [options] <address>[/<port>]\n", MAST_TOOL_NAME);
	printf( "    -s <ssrc>       Source identifier in hex (default is random)\n");
	printf( "    -t <ttl>        Time to live\n");
	printf( "    -p <payload>    The payload mime type to send\n");
	printf( "    -o <name=value> Set codec parameter / option\n");
	printf( "    -z <size>       Set the per-packet payload size\n");
	printf( "    -d <dscp>       DSCP Quality of Service value\n");
	printf( "    -r <msec>       Ring-buffer duration in milliseconds\n");
	printf( "    -c <channels>   Number of audio channels\n");
	printf( "    -n <name>       Name for this JACK client\n");
	printf( "    -a              Auto-connect jack input ports\n");
	
	exit(1);
	
}


static void parse_cmd_line(int argc, char **argv, MastSendTool* tool)
{
	int ch;


	// Parse the options/switches
	while ((ch = getopt(argc, argv, "as:t:p:o:z:d:c:r:h?")) != -1)
	switch (ch) {
		case 's': tool->set_session_ssrc( optarg ); break;
		case 't': tool->set_multicast_ttl( optarg ); break;
		case 'p': tool->set_payload_mimetype( optarg ); break;
		case 'o': tool->set_payload_mimetype_param( optarg ); break;
		case 'z': tool->set_payload_size_limit( optarg ); break;
		case 'd': tool->set_session_dscp( optarg ); break;

		case 'c': tool->set_input_channels( atoi(optarg) ); break;
		case 'a': g_do_autoconnect = TRUE; break;
		case 'r': g_rb_duration = atoi(optarg); break;
		
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
	
}




int main(int argc, char **argv)
{
	MastSendTool *tool = NULL;
	jack_client_t* client = NULL;


	// Create the send tool object
	tool = new MastSendTool( MAST_TOOL_NAME );


	// Parse the command line arguments 
	// and configure the session
	parse_cmd_line( argc, argv, tool );


	// Initialise Jack
	client = init_jack( tool );
	if (client==NULL) MAST_FATAL( "Failed to initialise JACK client" );
	
	// Get the samplerate of the JACK Router
	tool->set_input_samplerate( jack_get_sample_rate( client ) );
	
	// Setup signal handlers
	mast_setup_signals();

	// Run the main loop
	tool->run();
	
	// Clean up
	delete tool;
	

	// Shut down JACK
	deinit_jack( client );

	
	// Success !
	return 0;
}

