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


#ifndef	_MAST_CONFIG_H_
#define	_MAST_CONFIG_H_


typedef struct config_s {
	int ttl;
	int port;
	char* ip;
	int timeout;
	
	unsigned int ssrc;
	int payload_type;
	int payload_size;
	
	int use_stdio;
	int loop_file;
	char* devname;
	char* filename;
		
	int verbose;
	
	void(*audio_init_callback)( void* audio );
	
} config_t;


#define DEFAULT_PAYLOAD		10
#define MAX_STR_LEN			255


extern config_t* mast_config_init( );
extern void mast_config_print( config_t* config );
extern void mast_config_delete( config_t** config );


#endif
