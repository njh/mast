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





/*
Ê* Intel/DVI ADPCM coder/decoder.
Ê*
Ê* The algorithm for this coder was taken from the IMA Compatability Project
Ê* proceedings, Vol 2, Number 2; May 1992.
Ê*
 */

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

typedef struct adpcm_state {
	short valprev;		/* Previous output value */
	unsigned char index;	/* Index into stepsize table */
	unsigned char pad;
} adpcm_state_t;

    


void adpcm_encode(
    const int16_t *inp,
    u_int8_t *outp,
    int len,
    struct adpcm_state *state)
{
	int val;			/* Current input sample value */
	int sign;			/* Current adpcm sign bit */
	int delta;			/* Current adpcm output value */
	int step;			/* Stepsize */
	int valprev;		/* virtual previous output value */
	int vpdiff;			/* Current change to valprev */
	int index;			/* Current step change index */
	int outputbuffer = 0;	/* place to keep previous 4-bit value */
	int bufferstep;		/* toggle between outputbuffer/output */
	
	valprev = state->valprev;
	index = state->index;
	step = stepsizeTable[index];
	
	bufferstep = 1;
	
	while (--len >= 0) {
		val = *inp++;
		
		/* Step 1 - compute difference with previous value */
		delta = val - valprev;
		sign = (delta < 0) ? 8 : 0;
		if ( sign ) delta = (-delta);
		
		/* Step 2 - Divide and clamp */
		delta = (delta<<2) / step;
		if ( delta > 7 ) delta = 7;
		vpdiff = ((delta*step) >> 2) + (step >> 3);
		
		/* Step 3 - Update previous value */
		if ( sign )
			valprev -= vpdiff;
		else
			valprev += vpdiff;
		
		/* Step 4 - Clamp previous value to 16 bits */
		if ( valprev > 32767 )
			valprev = 32767;
		else if ( valprev < -32768 )
			valprev = -32768;
		
		/* Step 5 - Assemble value, update index and step values */
		delta |= sign;
		index += indexTable[delta];
		if ( index < 0 ) index = 0;
		if ( index > 88 ) index = 88;
		step = stepsizeTable[index];
		
		/* Step 6 - Output value */
		if ( bufferstep ) {
			outputbuffer = (delta << 4) & 0xf0;
		} else {
			*outp++ = (delta & 0x0f) | outputbuffer;
		}
		bufferstep = !bufferstep;
	}
	
	/* Output last step, if needed */
	if ( !bufferstep )
		*outp++ = outputbuffer;
	
	state->valprev = valprev;
	state->index = index;
}


void adpcm_decode(
    const u_int8_t *inp,
    int16_t *outp,
    int len,
    struct adpcm_state *state)
{
	int sign;			/* Current adpcm sign bit */
	int delta;			/* Current adpcm output value */
	int step;			/* Stepsize */
	int valprev;		/* virtual previous output value */
	int vpdiff;			/* Current change to valprev */
	int index;			/* Current step change index */
	int inputbuffer = 0;	/* place to keep next 4-bit value */
	int bufferstep;		/* toggle between inputbuffer/input */
	
	valprev = state->valprev;
	index = state->index;
	if ( index < 0 ) index = 0;
	else if ( index > 88 ) index = 88;
	step = stepsizeTable[index];
	
	bufferstep = 0;
	
	for ( ; len > 0 ; len-- ) {
	
		/* Step 1 - get the delta value and compute next index */
		if ( bufferstep ) {
			delta = inputbuffer & 0xf;
		} else {
			inputbuffer = *inp++;
			delta = (inputbuffer >> 4) & 0xf;
		}
		bufferstep = !bufferstep;
		
		/* Step 2 - Find new index value (for later) */
		index += indexTable[delta];
		if ( index < 0 ) index = 0;
		else if ( index > 88 ) index = 88;
		
		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;
		
		/* Step 4 - update output value */
		vpdiff = ((delta*step) >> 2) + (step >> 3);
		if ( sign )
			valprev -= vpdiff;
		else
			valprev += vpdiff;
		
		/* Step 5 - clamp output value */
		if ( valprev > 32767 )
			valprev = 32767;
		else if ( valprev < -32768 )
			valprev = -32768;
		
		/* Step 6 - Update step value */
		step = stepsizeTable[index];
		
		/* Step 7 - Output value */
		*outp++ = valprev;
	}
	
	state->valprev = valprev;
	state->index = index;
}




static u_int32_t mast_encode_dvi4(
		mast_codec_t* codec,
		u_int32_t inputsize, 	// input size in samples
		int16_t *input,
		u_int32_t outputsize,	// output size in bytes
		u_int8_t *output)
{
	adpcm_state_t* hdr = (adpcm_state_t*)output;
	adpcm_state_t* state = codec->ptr;
	int bytes = inputsize/2;
	
	if (outputsize < bytes) {
		MAST_ERROR("encode_dvi4: output buffer isn't big enough");
		return -1;
	}

	// Decode the audio samples in the packet
	adpcm_encode(input, output+sizeof(adpcm_state_t), inputsize, state);

	// Copy the state to the packet header
	hdr->valprev = htons( state->valprev );
	hdr->index = state->index;

	return sizeof(adpcm_state_t) + bytes;
}


static u_int32_t mast_decode_dvi4(
		mast_codec_t* codec,
		u_int32_t inputsize,		// input size in bytes
		u_int8_t  *input,
		u_int32_t outputsize, 	// output size in samples
		int16_t  *output)
{
	adpcm_state_t* hdr = (adpcm_state_t*)input;
	adpcm_state_t* state = codec->ptr;
	int bytes = inputsize-sizeof(adpcm_state_t);

	if (outputsize < (bytes*2)) {
		MAST_ERROR("decode_dvi4: output buffer isn't big enough");
		return -1;
	}

	// Copy the state from the packet header
	state->valprev = ntohs( hdr->valprev );
	state->index = hdr->index;

//	MAST_DEBUG("valprev=%d index=%d", state->valprev, state->index );

	// Decode the audio samples in the packet
	adpcm_decode(input+sizeof(adpcm_state_t), output, bytes, state);

	return bytes*2;
}


static int mast_deinit_dvi4( mast_codec_t* codec )
{
	adpcm_state_t* state = codec->ptr;

	// Free the ADPCM state
	if (state) free( state );

	// Free the codec structure
	free( codec );
	
	// Success
	return 0;
}
	


// Initialise the GSM codec
mast_codec_t* mast_init_dvi4()
{
	mast_codec_t* codec = NULL;

	// Allocate memory for generic codec structure
	codec = malloc( sizeof(mast_codec_t) );
	if (codec==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_codec_t data structure" );
		return NULL;
	}
	
	// Set the callbacks
	memset( codec, 0, sizeof(mast_codec_t) );
	codec->encode = mast_encode_dvi4;
	codec->decode = mast_decode_dvi4;
	codec->deinit = mast_deinit_dvi4;
	
	// Create the state record
	codec->ptr = malloc( sizeof(adpcm_state_t) );
	if (codec->ptr==NULL) {
		MAST_ERROR( "Failed to allocate memory for ADPCM state" );
		mast_deinit_dvi4( codec );
		return NULL;
	} else {
		// Zero the memory
		memset( codec->ptr, 0, sizeof(adpcm_state_t) );
	}

	return codec;
}

