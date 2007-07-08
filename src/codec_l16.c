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
#include "codecs.h"
#include "mast.h"




static u_int32_t encode_l16(
		mast_codec_t* codec,
		u_int32_t inputsize, 	// input size in samples
		int16_t *input,
		u_int32_t outputsize,	// output size in bytes
		u_int8_t *output)
{
	register int i;
	register int input_bytes = (inputsize*2);
	int16_t *output16 = (int16_t*)output;

	if (outputsize < input_bytes) {
		MAST_ERROR("encode_l16: output buffer isn't big enough");
		return 0;
	}

	for(i=0; i< inputsize; i++) {
		output16[i] = htons(input[i]);
	}

	return input_bytes;
}


static u_int32_t decode_l16(
		mast_codec_t* codec,
		u_int32_t inputsize,		// input size in bytes
		u_int8_t  *input,
		u_int32_t outputsize, 	// output size in samples
		int16_t  *output)
{
	register int i;
	register int num_samples = (inputsize/2);
	int16_t *input16 = (int16_t*)input;

	if (outputsize < inputsize) {
		MAST_ERROR("decode_l16: output buffer isn't big enough");
		return 0;
	}

	// Convert for network byte order, to machine byte order
	for(i=0; i< num_samples; i++) {
		output[i] = ntohs(input16[i]);
	}

	return num_samples;
}


static int deinit_l16( mast_codec_t* codec )
{
	// Don't need to do anything else here
	free( codec );
	
	// Success
	return 0;
}
	


// Initialise the L16 codec
mast_codec_t* mast_init_l16() {
	mast_codec_t* codec = malloc( sizeof(mast_codec_t) );
	if (codec==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_codec_t data structure" );
		return NULL;
	}
	
	// Set the callbacks
	memset( codec, 0, sizeof(mast_codec_t) );
	codec->encode = encode_l16;
	codec->decode = decode_l16;
	codec->deinit = deinit_l16;
	codec->ptr = NULL;
	
	return codec;
}



