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
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <twolame.h>

#include "codecs.h"
#include "mast.h"


typedef struct
{
	twolame_options *twolame;
	int prepared;
} mast_mpegaudio_t;


// Calculate the number of samples per packet for MPEG Audio
static int mast_spp_mpa( mast_codec_t *codec, int max_bytes)
{
	mast_mpegaudio_t* p = codec->ptr;
	int frames_per_packet = max_bytes/twolame_get_framelength( p->twolame );
	MAST_DEBUG("MPEG Audio Frames per Packet: %d", frames_per_packet );
	
	return frames_per_packet*TWOLAME_SAMPLES_PER_FRAME;
}


// Prepare to start encoding/decoding
static int mast_prepare_mpa( mast_codec_t *codec )
{
	mast_mpegaudio_t* p = codec->ptr;

	return twolame_init_params( p->twolame );
}


static int mast_set_param_mpa( mast_codec_t* codec, const char* name, const char* value )
{
	mast_mpegaudio_t* p = codec->ptr;

	/*	Parameters listed in RFC3555 for audio/MPA:
	
		layer: 1,2,3
		samplerate: 
		mode: "stereo", "joint_stereo", "single_channel", "dual_channel"
		bitrate: the data rate for the audio bit stream
		ptime: duration of each packet in milliseconds
		maxptime: maximum duration of each packet in milliseconds
		
		Example:
		audio/MPA;layer=2;mode=stereo;bitrate=160;
	*/
	
	if (strcmp(name, "layer")==0) {
		int layer = atoi(value);
		if (layer!=2) return -2;
	} else if (strcmp(name, "mode")==0) {
		int mode;
		if (strcmp(value, "stereo")==0) mode = TWOLAME_STEREO;
		else if (strcmp(value, "joint_stereo")==0) mode = TWOLAME_JOINT_STEREO;
		else if (strcmp(value, "single_channel")==0) mode = TWOLAME_MONO;
		else if (strcmp(value, "dual_channel")==0) mode = TWOLAME_DUAL_CHANNEL;
		else return -2;
		
		if (twolame_set_mode( p->twolame, mode )) {
			MAST_WARNING("Failed to set mode");
		}
	} else if (strcmp(name, "bitrate")==0) {
		int bitrate = atoi(value);
		if (twolame_set_bitrate( p->twolame, bitrate )) {
			MAST_WARNING("Failed to set bitrate");
		}
	} else {
		// Unsupported parameter
		return -1;
	}
	
	// Success
	return 0;
}

static const char* mast_get_param_mpa( mast_codec_t* codec, const char* name )
{

	if (strcmp(name, "layer")==0) {
		// We only support layer 2
		return "2";
	} else {
		// Unsupported
		return NULL;
	}

}


static u_int32_t mast_encode_mpa(
		mast_codec_t* codec,
		u_int32_t inputsize, 	// input size in samples
		int16_t *input,
		u_int32_t outputsize,	// output size in bytes
		u_int8_t *output)
{
	mast_mpegaudio_t* p = codec->ptr;
	int bytes_encoded = 0;

	// MPEG Audio header at the start of the packet
	output[0] = 0x00;
	output[1] = 0x00;
	output[2] = 0x00;
	output[3] = 0x00;

	// Encode the audio using twolame
	bytes_encoded = twolame_encode_buffer_interleaved( p->twolame, input, inputsize, output+4, outputsize-4 );
	MAST_DEBUG( "mast_encode_mpa: twolame encoded %d samples to %d bytes (max %d bytes)", inputsize, bytes_encoded, outputsize );
	
	return bytes_encoded+4;
}




static int mast_deinit_mpa( mast_codec_t* codec )
{
	mast_mpegaudio_t* p = codec->ptr;
	
	if (p) {
		// De-initialise twolame
		if (p->twolame) twolame_close( &p->twolame );

		free( p );
	}
	
	// Success
	return 0;
}
	


// Initialise the codec
int mast_init_mpa( mast_codec_t* codec ) {
	mast_mpegaudio_t *mpa = NULL;

	// Set the callbacks
	codec->samples_per_packet = mast_spp_mpa;
	codec->prepare = mast_prepare_mpa;
	codec->set_param = mast_set_param_mpa;
	codec->get_param = mast_get_param_mpa;
	codec->encode = mast_encode_mpa;
	codec->deinit = mast_deinit_mpa;


	// Allocate memory for codec's private data
	mpa = malloc( sizeof(mast_mpegaudio_t) );
	if (mpa==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_mpa_t data structure" );
		return -1;
	}
	codec->ptr = mpa;
	memset( mpa, 0, sizeof(mast_mpegaudio_t) );


	// Initialise twolame
	mpa->twolame = twolame_init();
	if (mpa->twolame==NULL) {
		MAST_ERROR( "Failed to initialise TwoLame" );
		return -1;
	}

	// Configure twolame
	if (twolame_set_num_channels( mpa->twolame, codec->channels )) {
		MAST_ERROR( "Failed to set number of input channels" );
		return -1;
	}
	if (twolame_set_in_samplerate( mpa->twolame, codec->samplerate )) {
		MAST_ERROR( "Failed to set number of input samplerate" );
		return -1;
	}
	
	// Success
	return 0;
}



