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

#include <samplerate.h>
#include "../libgsm/gsm.h"



// Calculate the number of samples per packet
static int mast_samples_per_packet_gsm( mast_codec_t *codec, int max_bytes)
{
	int frames_per_packet = max_bytes/GSM_FRAME_BYTES;
	MAST_DEBUG("GSM frames per packet = %d",frames_per_packet);
	return frames_per_packet * GSM_FRAME_SAMPLES;
}


// Encode a packet's payload
static u_int32_t mast_encode_gsm(
		mast_codec_t* codec,
		u_int32_t inputsize, 	// input size in samples
		float *input,
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
		gsm_signal signal_l16[GSM_FRAME_SAMPLES];
		float *signal_float = &input[GSM_FRAME_SAMPLES*f];
		gsm_byte* byte = output+(GSM_FRAME_BYTES*f);
		
		// Convert from float to 16-bit integer
		src_float_to_short_array( signal_float, signal_l16, GSM_FRAME_SAMPLES);
		
		// Encode to GSM
		gsm_encode(gsm_handle, signal_l16, byte);
	}

	return frames*GSM_FRAME_BYTES;
}


// Decode a packet's payload
static u_int32_t mast_decode_gsm(
		mast_codec_t* codec,
		u_int32_t inputsize,		// input size in bytes
		u_int8_t  *input,
		u_int32_t outputsize, 	// output size in samples
		float  *output)
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
		gsm_signal signal_l16[GSM_FRAME_SAMPLES];
		gsm_byte* byte = input+(GSM_FRAME_BYTES*f);
		float *signal_float = &output[GSM_FRAME_SAMPLES*f];
		
		// Decode from GSM
		gsm_decode(gsm_handle, byte, signal_l16);
		
		// Convert from 16-bit integer to float
		src_short_to_float_array( signal_l16, signal_float, GSM_FRAME_SAMPLES);
	}

	return frames*GSM_FRAME_SAMPLES;
}


static int mast_deinit_gsm( mast_codec_t* codec )
{
	gsm gsm_handle = codec->ptr;

	// Free the GSM handle
	if (gsm_handle) {
		gsm_destroy( gsm_handle );
		gsm_handle = NULL;
	}

	// Success
	return 0;
}
	


// Initialise the GSM codec
int mast_init_gsm(mast_codec_t* codec)
{
	gsm gsm_handle = NULL;
	
	if (codec->channels!=1) {
		MAST_ERROR("The GSM codec is mono only");
		return -1;
	}
	
	// Set the callbacks
	codec->samples_per_packet = mast_samples_per_packet_gsm;
	codec->encode_packet = mast_encode_gsm;
	codec->decode_packet = mast_decode_gsm;
	codec->deinit = mast_deinit_gsm;
	
	// Create the GSM handle
	gsm_handle = gsm_create();
	codec->ptr = gsm_handle;
	if (gsm_handle==NULL) {
		MAST_ERROR( "Failed to initialise GSM library" );
		mast_deinit_gsm( codec );
		return -1;
	}

	// Success
	return 0;
}
