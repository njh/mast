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

#include "codecs.h"
#include "mast.h"


// ==================================== private for ulaw 

#define uBIAS 0x84
#define uCLIP 32635

typedef struct
{
	int16_t *ulawtoint16_table;
	int16_t *ulawtoint16_ptr;
	unsigned char *int16toulaw_table;
	unsigned char *int16toulaw_ptr;
} mast_pcmu_t;



static int ulaw_init_ulawtoint16( mast_pcmu_t* p )
{
	int i;

	if(!p->ulawtoint16_table)
	{
    	static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
    	int sign, exponent, mantissa, sample;
		unsigned char ulawbyte;

		p->ulawtoint16_table = malloc(sizeof(int16_t) * 256);
		if (!p->ulawtoint16_table) return 1;
		
		p->ulawtoint16_ptr = p->ulawtoint16_table;
		for(i = 0; i < 256; i++)
		{
			ulawbyte = (unsigned char)i;
    		ulawbyte = ~ulawbyte;
    		sign = (ulawbyte & 0x80);
    		exponent = (ulawbyte >> 4) & 0x07;
    		mantissa = ulawbyte & 0x0F;
    		sample = exp_lut[exponent] + (mantissa << (exponent + 3));
    		if(sign != 0) sample = -sample;
			p->ulawtoint16_ptr[i] = sample;
		}
	}
	
	return 0;
}

static int ulaw_init_int16toulaw( mast_pcmu_t* p )
{

	if(!p->int16toulaw_table)
	{
    	int sign, exponent, mantissa;
    	unsigned char ulawbyte;
		int sample;
		int i;
    	int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

 		p->int16toulaw_table = malloc(65536);
		if (!p->int16toulaw_table) return 1;

		p->int16toulaw_ptr = p->int16toulaw_table + 32768;

		for(i = -32768; i < 32768; i++)
		{
			sample = i;
// Get the sample into sign-magnitude. 
    		sign = (sample >> 8) & 0x80;		// set aside the sign 
    		if(sign != 0) sample = -sample;		// get magnitude 
    		if(sample > uCLIP) sample = uCLIP;		// clip the magnitude 

// Convert from 16 bit linear to ulaw. 
    		sample = sample + uBIAS;
		    exponent = exp_lut[(sample >> 7) & 0xFF];
		    mantissa = (sample >> (exponent + 3)) & 0x0F;
		    ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
		    if (ulawbyte == 0) ulawbyte = 0x02;	    // optional CCITT trap 
#endif

		    p->int16toulaw_ptr[i] = ulawbyte;
		}
	}
	return 0;
}

static int16_t ulaw_bytetoint16(mast_pcmu_t* p, u_int8_t input)
{
	return p->ulawtoint16_ptr[input];
}

static u_int8_t ulaw_int16tobyte(mast_pcmu_t* p, int16_t input)
{
	return p->int16toulaw_ptr[input];
}


// Calculate the number of samples per packet
static int mast_samples_per_packet_pcmu( mast_codec_t *codec, int max_bytes)
{
	int bytes_per_unit = SAMPLES_PER_UNIT * codec->channels;
	MAST_DEBUG("PCMU bytes per unit = %d", bytes_per_unit);
	return (max_bytes / bytes_per_unit) * SAMPLES_PER_UNIT;
}



// Encode a packet's payload
static u_int32_t mast_encode_pcmu(
		mast_codec_t* codec,
		u_int32_t inputsize, 	/* input size in samples */
		int16_t *input,
		u_int32_t outputsize,	/* output size in bytes */
		u_int8_t *output)
{
	mast_pcmu_t* p = (mast_pcmu_t*)codec->ptr;
	register int i;
	
	if (outputsize < inputsize) {
		MAST_ERROR("encode_ulaw: output buffer isn't big enough");
		return 0;
	}

	for(i=0;i<inputsize;i++) {
		output[i] = ulaw_int16tobyte(p, input[i]);
	}	
	
	return inputsize;
}



// Decode a packet's payload
static u_int32_t mast_decode_pcmu(
		mast_codec_t* codec,
		u_int32_t inputsize,	/* input size in bytes */
		u_int8_t  *input,
		u_int32_t outputsize, 	/* output size in samples */
		int16_t  *output)
{
	mast_pcmu_t* p = (mast_pcmu_t*)codec->ptr;
	int i;
	
	if (outputsize < inputsize) {
		MAST_ERROR("decode_ulaw: output buffer isn't big enough");
		return 0;
	}
	
	for(i=0;i<inputsize;i++) {
		output[i] = ulaw_bytetoint16(p, input[i]);
	}		
	
	// outputsize == inputsize * 2
	return inputsize;
}





static int mast_deinit_pcmu( mast_codec_t* codec )
{
	mast_pcmu_t* p = (mast_pcmu_t*)codec->ptr;
	
	if (p) {
		if(p->ulawtoint16_table)	free(p->ulawtoint16_table); 
		if(p->int16toulaw_table)	free(p->int16toulaw_table); 
		free(p);
		codec->ptr = NULL;
	}

	free( codec );
	
	// Success
	return 0;
}
	


// Initialise the PCMU codec
int mast_init_pcmu( mast_codec_t* codec ) {
	mast_pcmu_t* pcmu = NULL;

	if (codec->channels!=1) {
		MAST_ERROR("The PCMU codec is mono only");
		return -1;
	}

	// Set the callbacks
	codec->samples_per_packet = mast_samples_per_packet_pcmu;
	codec->encode_packet = mast_encode_pcmu;
	codec->decode_packet = mast_decode_pcmu;
	codec->deinit = mast_deinit_pcmu;


	// Allocate memory for codec's private data
	pcmu = malloc( sizeof(mast_pcmu_t) );
	if (pcmu==NULL) {
		MAST_ERROR( "Failed to allocate memory for mast_pcmu_t data structure" );
		return -1;
	}
	codec->ptr = pcmu;
	memset( pcmu, 0, sizeof(mast_pcmu_t) );

	// Initialise the tables
	if (ulaw_init_ulawtoint16( pcmu )) {
		MAST_ERROR( "Failed to initialise ulaw decode table" );
		return -1;
	}
	if (ulaw_init_int16toulaw( pcmu )) {
		MAST_ERROR( "Failed to initialise ulaw encode table" );
		return -1;
	}
	
	// Success
	return 0;
}


