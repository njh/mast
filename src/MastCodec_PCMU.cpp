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

#include "MastCodec_PCMU.h"


#define	PCMU_DEFAULT_CHANNELS		(1)
#define	PCMU_DEFAULT_SAMPLERATE		(8000)


static mast_sample_t pcmu_decode_table[128] =
{
        -0.980347, -0.949097, -0.917847, -0.886597, -0.855347, -0.824097, -0.792847, -0.761597, 
        -0.730347, -0.699097, -0.667847, -0.636597, -0.605347, -0.574097, -0.542847, -0.511597, 
        -0.488159, -0.472534, -0.456909, -0.441284, -0.425659, -0.410034, -0.394409, -0.378784, 
        -0.363159, -0.347534, -0.331909, -0.316284, -0.300659, -0.285034, -0.269409, -0.253784, 
        -0.242065, -0.234253, -0.226440, -0.218628, -0.210815, -0.203003, -0.195190, -0.187378, 
        -0.179565, -0.171753, -0.163940, -0.156128, -0.148315, -0.140503, -0.132690, -0.124878, 
        -0.119019, -0.115112, -0.111206, -0.107300, -0.103394, -0.099487, -0.095581, -0.091675, 
        -0.087769, -0.083862, -0.079956, -0.076050, -0.072144, -0.068237, -0.064331, -0.060425, 
        -0.057495, -0.055542, -0.053589, -0.051636, -0.049683, -0.047729, -0.045776, -0.043823, 
        -0.041870, -0.039917, -0.037964, -0.036011, -0.034058, -0.032104, -0.030151, -0.028198, 
        -0.026733, -0.025757, -0.024780, -0.023804, -0.022827, -0.021851, -0.020874, -0.019897, 
        -0.018921, -0.017944, -0.016968, -0.015991, -0.015015, -0.014038, -0.013062, -0.012085, 
        -0.011353, -0.010864, -0.010376, -0.009888, -0.009399, -0.008911, -0.008423, -0.007935, 
        -0.007446, -0.006958, -0.006470, -0.005981, -0.005493, -0.005005, -0.004517, -0.004028, 
        -0.003662, -0.003418, -0.003174, -0.002930, -0.002686, -0.002441, -0.002197, -0.001953, 
        -0.001709, -0.001465, -0.001221, -0.000977, -0.000732, -0.000488, -0.000244, 0.000000
};


#define uBIAS 0x84
#define uCLIP 32635

/*
static inline mast_sample_t ulaw_to_float( char ulawbyte )
{
	int sign, exponent, mantissa;
	mast_sample_t sample;

	ulawbyte = ~ulawbyte;
	sign = (ulawbyte & 0x80);
	exponent = (ulawbyte >> 4) & 0x07;
	mantissa = ulawbyte & 0x0F;
	sample = exp_lut[exponent] + (mantissa << (exponent + 3));
	if(sign != 0) sample = -sample;
	
	return sample;
}
*/


static inline mast_sample_t ulaw_to_float( int ulawbyte )
{
	mast_sample_t sample = 0.0;

	if (ulawbyte & 0x80) {
		sample = -1.0f * pcmu_decode_table[ulawbyte & 0x7F];
	} else {
		sample = pcmu_decode_table[ulawbyte & 0x7F];
	}
		
	return sample;
}

static inline char float_to_ulaw( mast_sample_t sample_f32 )
{
	int sign, exponent, mantissa;
	int sample_s16 = (int)(sample_f32 * 0x8000f);
	unsigned char ulawbyte;
	int exp_lut[256] =
		{0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
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

// Get the sample into sign-magnitude. 
	sign = (sample_s16 >> 8) & 0x80;		// set aside the sign 
	if(sign != 0) sample_s16 = -sample_s16;		// get magnitude 
	if(sample_s16 > uCLIP) sample_s16 = uCLIP;		// clip the magnitude 

// Convert from 16 bit linear to ulaw. 
	sample_s16 = sample_s16 + uBIAS;
	exponent = exp_lut[(sample_s16 >> 7) & 0xFF];
	mantissa = (sample_s16 >> (exponent + 3)) & 0x0F;
	ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
	if (ulawbyte == 0) ulawbyte = 0x02;	    // optional CCITT trap 
#endif

	return ulawbyte;
}


// Calculate the number of samples per packet
size_t MastCodec_PCMU::frames_per_packet_internal( size_t max_bytes )
{
	int bytes_per_unit = SAMPLES_PER_UNIT * this->channels;
	MAST_DEBUG("PCMU bytes per unit = %d", bytes_per_unit);
	return (max_bytes / bytes_per_unit) * SAMPLES_PER_UNIT;
}



// Encode a packet's payload
size_t MastCodec_PCMU::encode_packet_internal(
		size_t inputsize, 	/* input size in frames */
		mast_sample_t *input,
		size_t outputsize,	/* output size in bytes */
		u_int8_t *output)
{
	register size_t i;
	
	if (outputsize < inputsize) {
		MAST_ERROR("encode_ulaw: output buffer isn't big enough");
		return 0;
	}

	for(i=0;i<inputsize;i++) {
		output[i] = float_to_ulaw( input[i] );
	}	
	
	return inputsize;
}



// Decode a packet's payload
size_t MastCodec_PCMU::decode_packet_internal(
		size_t inputsize,	/* input size in bytes */
		u_int8_t  *input,
		size_t outputsize, 	/* output size in frames */
		mast_sample_t  *output)
{
	size_t i;
	
	if (outputsize < inputsize) {
		MAST_ERROR("decode_ulaw: output buffer isn't big enough");
		return 0;
	}
	
	for(i=0;i<inputsize;i++) {
		output[i] = ulaw_to_float( input[i] );
	}		
	
	return inputsize;
}

	
// Initialise the PCMU codec
MastCodec_PCMU::MastCodec_PCMU( MastMimeType* type )
	: MastCodec(type)
{

	// Set default values
	this->samplerate = PCMU_DEFAULT_SAMPLERATE;
	this->channels = PCMU_DEFAULT_CHANNELS;
	
	// Apply MIME type parameters to the codec
	this->apply_mime_type_params( type );

}




