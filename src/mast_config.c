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


#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "mast_config.h"


config_t*
mast_config_init( )
{
	config_t* config = (char*)malloc( sizeof(config_t) );
	if (!config) {
		fprintf(stderr,"malloc failed to allocate memory for config structure.\n");
		exit(-1);
	}
	
	config->ttl = 5;
	config->port = 0;
	config->ip = NULL;
	
	config->timeout = 10;
	config->ssrc = 0;
	config->payload_type = -1;
	config->payload_size = 0;
	
	config->use_stdio = 0;
	config->loop_file = 0;
	config->devname = NULL;
	config->filename = NULL;
	
	// Default is to be verbose
	config->verbose = 1;

	config->audio_init_callback = NULL;

	return config;
}



void
mast_config_print( config_t* config )
{

#if DEBUG
	fprintf(stderr,"config->ttl=%d\n", config->ttl);
	fprintf(stderr,"config->port=%d\n", config->port);
	fprintf(stderr,"config->ip=%s\n", config->ip);
	fprintf(stderr,"config->ssrc=0x%x\n", config->ssrc);
	fprintf(stderr,"config->payload=%d\n", config->payload_type);
	fprintf(stderr,"config->payload_size=%d\n", config->payload_size);
	fprintf(stderr,"config->use_stdio=%d\n", config->use_stdio);
#ifdef HAVE_OSS
	fprintf(stderr,"config->devname=%s\n", config->devname);
#endif
	fprintf(stderr,"config->filename=%s\n", config->filename);
#endif


	fprintf(stderr,"RTP Payload Len: %d\n", config->payload_size );
	fprintf(stderr,"RTP Payload Type: %d\n\n", config->payload_type );

}


void
mast_config_delete( config_t** config )
{
	
	if (config && *config) {
		free( (char*)*config );
		*config = NULL;
	}

}

