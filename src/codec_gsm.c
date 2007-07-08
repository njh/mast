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

#include "config.h"
#include "codecs.h"
#include "mast.h"

#include "../libgsm/gsm.h"



static u_int32_t encode_gsm(
		mast_codec_t* codec,
		u_int32_t inputsize, 	// input size in samples
		int16_t *input,
		u_int32_t outputsize,	// output size in bytes
		u_int8_t *output)
{
	gsm gsm_handle = codec->ptr;
	int frames = (inputsize/GSM_FRAME_SAMPLES);
	int f = 0;
	
	if (inputsize % GSM_FRAME_SAMPLES) {
		MAST_DEBUG("encode_gsm: number of input samples (%d) isn't a multiple of %d", inputsize, GSM_FRAME_SAMPLES);
	}

	if (outputsize < (frames*GSM_FRAME_BYTES)) {
		MAST_ERROR("encode_gsm: output buffer isn't big enough");
		return -1;
	}

	// Encode frame by frame
	for(f=0; f<frames; f++) {
		gsm_signal* signal = &input[GSM_FRAME_SAMPLES*f];
		gsm_byte* byte = output+(GSM_FRAME_BYTES*f);
		gsm_encode(gsm_handle, signal, byte);
	}

	return frames*GSM_FRAME_BYTES;
}


static u_int32_t decode_gsm(
		mast_codec_t* codec,
		u_int32_t inputsize,		// input size in bytes
		u_int8_t  *input,
		u_int32_t outputsize, 	// output size in samples
		int16_t  *output)
{
	gsm gsm_handle = codec->ptr;
	int frames = (inputsize/GSM_FRAME_BYTES);
	int f = 0;
	
	if (outputsize < (frames*GSM_FRAME_SAMPLES)) {
		MAST_ERROR("decode_gsm: output buffer isn't big enough");
		return -1;
	}

	// Decode frame by frame
	for(f=0; f<frames; f++) {
		gsm_byte* byte = input+(GSM_FRAME_BYTES*f);
		gsm_signal* signal = &output[GSM_FRAME_SAMPLES*f];
		gsm_decode(gsm_handle, byte, signal);
	}

	return frames*GSM_FRAME_SAMPLES;
}


static int deinit_gsm( mast_codec_t* codec )
{
	gsm gsm_handle = codec->ptr;

	// Free the GSM handle
	if (gsm_handle) {
		gsm_destroy( gsm_handle );
		gsm_handle = NULL;
	}

	// Free the codec structure
	free( codec );
	
	// Success
	return 0;
}
	


// Initialise the GSM codec
mast_codec_t* mast_init_gsm()
{
	mast_codec_t* codec = NULL;
	gsm gsm_handle = NULL;

	// Allocate memory for generic codec structure
	codec = malloc( sizeof(mast_codec_t) );
	if (codec==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_codec_t data structure" );
		return NULL;
	}
	
	// Set the callbacks
	memset( codec, 0, sizeof(mast_codec_t) );
	codec->encode = encode_gsm;
	codec->decode = decode_gsm;
	codec->deinit = deinit_gsm;
	
	// Create the GSM handle
	gsm_handle = gsm_create();
	codec->ptr = gsm_handle;
	if (gsm_handle==NULL) {
		MAST_ERROR( "Failed to initialise GSM library" );
		deinit_gsm( codec );
		return NULL;
	}
	
	return codec;
}
