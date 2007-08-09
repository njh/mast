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

#include <samplerate.h>
#include <netinet/in.h>

#include "MastCodec_L16.h"
#include "mast.h"

#define	L16_DEFAULT_SAMPLERATE		(44100)
#define	L16_DEFAULT_CHANNELS		(2)


// Calculate the number of samples per packet
size_t MastCodec_L16::frames_per_packet_internal( size_t max_bytes )
{
	int bytes_per_frame = sizeof(int16_t) * this->channels;
	int bytes_per_unit = SAMPLES_PER_UNIT * bytes_per_frame;
	MAST_DEBUG("L16 bytes per frame = %d", bytes_per_frame);
	MAST_DEBUG("L16 bytes per unit = %d", bytes_per_unit);
	return (max_bytes / bytes_per_unit) * SAMPLES_PER_UNIT;
}


// Encode a packet's payload
size_t MastCodec_L16::encode_packet_internal(
		size_t inputsize, 	/* input size in frames */
		mast_sample_t *input,
		size_t outputsize,	/* output size in bytes */
		u_int8_t *output)
{
	size_t num_samples = inputsize * this->channels;
	size_t output_bytes = (num_samples * sizeof(int16_t));
	int16_t *output16 = (int16_t*)output;
	size_t i;

	if (outputsize < output_bytes) {
		MAST_ERROR("encode_l16: output buffer isn't big enough");
		return 0;
	}

	// Convert samples from float32 to 16-bit integers
	src_float_to_short_array( input, output16, num_samples);
	
	// Then convert from machine to network endian
	for(i=0; i< num_samples; i++) {
		int16_t sample = htons( output16[i] );
		output16[i] = sample;
	}

	return output_bytes;
}


// Decode a packet's payload
size_t MastCodec_L16::decode_packet_internal(
		size_t inputsize,	/* input size in bytes */
		u_int8_t  *input,
		size_t outputsize, 	/* output size in frames */
		mast_sample_t  *output)
{
	register int num_samples = (inputsize/sizeof(int16_t));
	int16_t *input16 = (int16_t*)input;
	register int i;

	if (outputsize < inputsize) {
		MAST_ERROR("decode_l16: output buffer isn't big enough");
		return 0;
	}

	// Convert to machine byte order from network byte order
	for(i=0; i< num_samples; i++) {
		int16_t sample = ntohs(input16[i]);
		input16[i] = sample;
	}

	// Then convert from 16-bit integrer to float32
	src_short_to_float_array( input16, output, num_samples);


	return num_samples/this->channels;
}


// Initialise the L16 codec
MastCodec_L16::MastCodec_L16( MastMimeType *type)
	: MastCodec(type)
{

	// Set default values
	this->samplerate = L16_DEFAULT_SAMPLERATE;
	this->channels = L16_DEFAULT_CHANNELS;

	// Apply MIME type parameters to the codec
	this->apply_mime_type_params( type );

}

