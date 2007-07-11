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



static int mast_set_param_mpa( mast_codec_t* codec, const char* name, const char* value )
{
	// We don't support any parameters
	return -1;
}

static const char* mast_get_param_mpa( mast_codec_t* codec, const char* name )
{
	// We don't support any parameters
	return NULL;
}


static void prepare_encoder( mast_mpegaudio_t* p )
{
	int err;
	
	twolame_print_config( p->twolame );
	
	
	err = twolame_init_params( p->twolame );
	if (err) {
	
		MAST_WARNING( "Failed to prepare twolame encoder" );
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
	
	
	if (!p->prepared) {
		prepare_encoder( p );
		p->prepared=1;
	}
	
/*
	register int input_bytes = (inputsize*2);

	if (outputsize < input_bytes) {
		MAST_ERROR("encode_mpa: output buffer isn't big enough");
		return 0;
	}
*/

	return 0;
}


static u_int32_t mast_decode_mpa(
		mast_codec_t* codec,
		u_int32_t inputsize,	// input size in bytes
		u_int8_t  *input,
		u_int32_t outputsize, 	// output size in samples
		int16_t  *output)
{
	MAST_ERROR("MAST doesn't currently support decoding of MPEG Audio.");
	
	// Error
	return 0;
}


static int mast_deinit_mpa( mast_codec_t* codec )
{
	mast_mpegaudio_t* p = codec->ptr;
	
	if (p) {
		// De-initialise twolame
		if (p->twolame) twolame_close( &p->twolame );

		free( p );
	}
	
	free( codec );
	
	// Success
	return 0;
}
	


// Initialise the codec
mast_codec_t* mast_init_mpa() {
	mast_mpegaudio_t *mpa = NULL;

	mast_codec_t* codec = malloc( sizeof(mast_codec_t) );
	if (codec==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_codec_t data structure" );
		return NULL;
	}
	

	// Set the callbacks
	memset( codec, 0, sizeof(mast_codec_t) );
	codec->set_param = mast_set_param_mpa;
	codec->get_param = mast_get_param_mpa;
	codec->encode = mast_encode_mpa;
	codec->decode = mast_decode_mpa;
	codec->deinit = mast_deinit_mpa;
	codec->ptr = NULL;


	// Allocate memory for codec's private data
	mpa = malloc( sizeof(mast_mpegaudio_t) );
	if (mpa==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_mpa_t data structure" );
		free( codec );
		return NULL;
	}
	codec->ptr = mpa;
	memset( mpa, 0, sizeof(mast_mpegaudio_t) );


	// Initialise twolame
	mpa->twolame = twolame_init();
	if (mpa->twolame==NULL) {
		MAST_ERROR( "Failed to initialise TwoLame" );
		free( codec );
		return NULL;
	}

	
	return codec;
}



