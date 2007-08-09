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


#include "MastCodec_GSM.h"
#include "mast.h"

#include <samplerate.h>
#include "../libgsm/gsm.h"

#define	GSM_DEFAULT_CHANNELS		(1)
#define	GSM_DEFAULT_SAMPLERATE		(8000)



// Calculate the number of samples per packet
size_t MastCodec_GSM::frames_per_packet_internal( size_t max_bytes )
{
	int frames_per_packet = max_bytes/GSM_FRAME_BYTES;
	MAST_DEBUG("GSM frames per packet = %d", frames_per_packet);
	return frames_per_packet * GSM_FRAME_SAMPLES;
}


// Encode a packet's payload
size_t MastCodec_GSM::encode_packet_internal(
		size_t inputsize, 	/* input size in frames */
		mast_sample_t *input,
		size_t outputsize,	/* output size in bytes */
		u_int8_t *output)
{
	size_t frames = (inputsize/GSM_FRAME_SAMPLES);
	size_t f = 0;

	if (inputsize % GSM_FRAME_SAMPLES) {
		MAST_DEBUG("decode_gsm: number of input samples (%d) isn't a multiple of %d", inputsize, GSM_FRAME_SAMPLES);
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

	// Return the length of the encoded audio
	return frames*GSM_FRAME_BYTES;
}


// Decode a packet's payload
size_t MastCodec_GSM::decode_packet_internal(
		size_t inputsize,	/* input size in bytes */
		u_int8_t  *input,
		size_t outputsize, 	/* output size in frames */
		mast_sample_t  *output)
{
	size_t frames = (inputsize/GSM_FRAME_BYTES);
	size_t f = 0;
	
	if (inputsize % GSM_FRAME_BYTES) {
		MAST_DEBUG("decode_gsm: number of input bytes (%d) isn't a multiple of %d", inputsize, GSM_FRAME_BYTES);
	}

	if (outputsize < (frames*GSM_FRAME_SAMPLES)) {
		MAST_ERROR("decode_gsm: output buffer isn't big enough");
		return -1;
	}

	// Decode frame by frame
	for(f=0; f<frames; f++) {
		gsm_signal signal_l16[GSM_FRAME_SAMPLES];
		gsm_byte* byte = input+(GSM_FRAME_BYTES*f);
		mast_sample_t *signal_float = output+(GSM_FRAME_SAMPLES*f);
		
		// Decode from GSM
		gsm_decode(gsm_handle, byte, signal_l16);
		
		// Convert from 16-bit integer to float
		src_short_to_float_array( signal_l16, signal_float, GSM_FRAME_SAMPLES);

	}

	return frames*GSM_FRAME_SAMPLES;
}


MastCodec_GSM::~MastCodec_GSM()
{

	// Free the GSM handle
	if (gsm_handle) {
		gsm_destroy( gsm_handle );
		gsm_handle = NULL;
	}

}
	


// Initialise the GSM codec
MastCodec_GSM::MastCodec_GSM( MastMimeType *type)
	: MastCodec(type)
{
	
	// Set default values
	this->samplerate = GSM_DEFAULT_SAMPLERATE;
	this->channels = GSM_DEFAULT_CHANNELS;

	
	// Create the GSM handle
	gsm_handle = gsm_create();
	if (gsm_handle==NULL) {
		MAST_FATAL( "Failed to initialise GSM library" );
	}
	

	// Apply MIME type parameters to the codec
	this->apply_mime_type_params( type );

}
