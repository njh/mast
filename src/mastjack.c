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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>

#include "config.h"
#include "mastjack.h"
#include "mast.h"



// ------- Globals -------
int g_channels = 2;
int g_do_autoconnect = FALSE;
int g_rb_duration = DEFAULT_RINGBUFFER_DURATION;
jack_port_t *g_jackport[2] = {NULL, NULL};
jack_ringbuffer_t *g_ringbuffer = NULL;
int16_t *g_convbuffer = NULL;





/* Callback called by JACK when audio is available
   Use as little CPU time as possible, just copy accross the audio
   into the ring buffer
*/
static int process_callback(jack_nframes_t nframes, void *arg)
{
    size_t to_write = 0, written = 0;
	unsigned int c,n;
	
	// Process channel by channel
	for (c=0; c < g_channels; c++)
	{	
        jack_default_audio_sample_t *buf = jack_port_get_buffer(g_jackport[c], nframes);
 
		// Convert the audio to signed 16-bit
		for(n=0; n<nframes; n++) {
			//int tmp = lrintf(buf[n] * 32768.0f);	FIXME: can't get this to compile on Linux
			int tmp = (buf[n] * 32768.0f);
			if (tmp > SHRT_MAX) {
				g_convbuffer[(n*g_channels)+c] = SHRT_MAX;
			} else if (tmp < SHRT_MIN) {
				g_convbuffer[(n*g_channels)+c] = SHRT_MIN;
			} else {
				g_convbuffer[(n*g_channels)+c] = (int16_t) tmp;
			}
		}
	}

	// Now write the converted audio to the ring buffer
	to_write = sizeof(int16_t) * nframes * g_channels;
	written = jack_ringbuffer_write(g_ringbuffer, (char*)g_convbuffer, to_write);
	if (to_write > written) {
		// If this goes wrong, then the buffer goes out of sync and we get static
		MAST_FATAL("Failed to write to ring ruffer");
		return 1;
	}

	// Success
	return 0;
}
					

// Callback called by JACK when buffersize changes
static int buffersize_callback(jack_nframes_t nframes, void *arg)
{
	MAST_DEBUG("JACK buffer size is %d samples long", nframes);

	// (re-)allocate conversion buffer
	g_convbuffer = realloc( g_convbuffer, nframes * sizeof(int16_t) * g_channels );
	if (g_convbuffer == NULL) {
		MAST_FATAL("Failed to (re-)allocate the convertion buffer");
	}

	// Success
	return 0;
}




// Callback called by JACK when jackd is shutting down
static void shutdown_callback(void *arg)
{
	MAST_WARNING("MAST quitting because jackd is shutting down" );
	
	// Signal the main thead to stop
	mast_stop_running();
}



// Crude way of automatically connecting up jack ports
static void autoconnect_jack_ports( jack_client_t* client )
{
	const char **all_ports;
	unsigned int ch=0;
	int err = 0;
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
		if (++ch >= g_channels) break;
	}
	
	free( all_ports );
}


// Initialise Jack related stuff
jack_client_t* mast_init_jack( const char* client_name, jack_options_t jack_opt ) 
{
	jack_client_t* client = NULL;
	jack_status_t status;
	size_t ringbuffer_size = 0;
	int i = 0;

	// Register with Jack
	if ((client = jack_client_open(client_name, jack_opt, &status)) == 0) {
		MAST_ERROR("Failed to start jack client: 0x%x", status);
		return NULL;
	} else {
		MAST_INFO( "JACK client registered as '%s'", jack_get_client_name( client ) );
	}

	// Create our input port(s)
	if (g_channels==1) {
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
	ringbuffer_size = jack_get_sample_rate( client ) * g_rb_duration * g_channels * sizeof(int16_t) / 1000;
	MAST_INFO("Duration of the ring buffer is %d ms (%d bytes)", g_rb_duration, (int)ringbuffer_size );
	if (!(g_ringbuffer = jack_ringbuffer_create( ringbuffer_size ))) {
		MAST_ERROR("Cannot create ringbuffer %d", i);
		return NULL;
	}

	// Register callbacks
	jack_on_shutdown(client, shutdown_callback, NULL );
	jack_set_buffer_size_callback( client, buffersize_callback, NULL);
	jack_set_process_callback(client, process_callback, NULL);
	
	// Create conversion buffer
	buffersize_callback( jack_get_buffer_size(client), NULL );

	// Activate JACK
	if (jack_activate(client)) MAST_FATAL("Cannot activate JACK client");
	
	/* Auto connect ports ? */
	if (g_do_autoconnect) autoconnect_jack_ports( client );
	
	return client;
}


// Shut down jack related stuff
void mast_deinit_jack( jack_client_t *client )
{
	// Leave the Jack graph
	if (jack_client_close(client)) {
		MAST_WARNING("Failed to close jack client");
	}
	
	// Free the coversion buffer
	if (g_convbuffer) {
		free( g_convbuffer );
		g_convbuffer = NULL;
	}
	
	// Free up the ring buffer
	if (g_ringbuffer) {
		jack_ringbuffer_free( g_ringbuffer );
		g_ringbuffer = NULL;
	}
}


