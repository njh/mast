/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
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

#include "MastCodec_PCMA.h"


#define PCMA_DEFAULT_CHANNELS        (1)
#define PCMA_DEFAULT_SAMPLERATE      (8000)


static float pcma_decode[128] =
{
    -0.167969, -0.160156, -0.183594, -0.175781, -0.136719, -0.128906, -0.152344, -0.144531,
    -0.230469, -0.222656, -0.246094, -0.238281, -0.199219, -0.191406, -0.214844, -0.207031,
    -0.083984, -0.080078, -0.091797, -0.087891, -0.068359, -0.064453, -0.076172, -0.072266,
    -0.115234, -0.111328, -0.123047, -0.119141, -0.099609, -0.095703, -0.107422, -0.103516,
    -0.671875, -0.640625, -0.734375, -0.703125, -0.546875, -0.515625, -0.609375, -0.578125,
    -0.921875, -0.890625, -0.984375, -0.953125, -0.796875, -0.765625, -0.859375, -0.828125,
    -0.335938, -0.320312, -0.367188, -0.351562, -0.273438, -0.257812, -0.304688, -0.289062,
    -0.460938, -0.445312, -0.492188, -0.476562, -0.398438, -0.382812, -0.429688, -0.414062,
    -0.010498, -0.010010, -0.011475, -0.010986, -0.008545, -0.008057, -0.009521, -0.009033,
    -0.014404, -0.013916, -0.015381, -0.014893, -0.012451, -0.011963, -0.013428, -0.012939,
    -0.002686, -0.002197, -0.003662, -0.003174, -0.000732, -0.000244, -0.001709, -0.001221,
    -0.006592, -0.006104, -0.007568, -0.007080, -0.004639, -0.004150, -0.005615, -0.005127,
    -0.041992, -0.040039, -0.045898, -0.043945, -0.034180, -0.032227, -0.038086, -0.036133,
    -0.057617, -0.055664, -0.061523, -0.059570, -0.049805, -0.047852, -0.053711, -0.051758,
    -0.020996, -0.020020, -0.022949, -0.021973, -0.017090, -0.016113, -0.019043, -0.018066,
    -0.028809, -0.027832, -0.030762, -0.029785, -0.024902, -0.023926, -0.026855, -0.025879
};



static float alaw_to_float( int alawbyte )
{
    float sample = 0.0;

    if (alawbyte & 0x80) {
        sample = -1.0f * pcma_decode[alawbyte & 0x7F];
    } else {
        sample = pcma_decode[alawbyte & 0x7F];
    }

    return sample;
}


static inline int val_seg(int val)
{
    int r = 1;

    val >>= 8;
    if (val & 0xf0) {
        val >>= 4;
        r += 4;
    }
    if (val & 0x0c) {
        val >>= 2;
        r += 2;
    }
    if (val & 0x02)
        r += 1;
    return r;
}


static inline int float_to_alaw( float sample_f32 )
{
    int sample_s16 = (int)(sample_f32 * (float)0x8000);
    unsigned char mask;
    unsigned char aval;
    int seg;

    if (sample_s16 >= 0) {
        mask = 0xD5;
    } else {
        mask = 0x55;
        sample_s16 = -sample_s16;
        if (sample_s16 > 0x7fff) sample_s16 = 0x7fff;
    }

    if (sample_s16 < 256) {
        aval = sample_s16 >> 4;
    } else {
        /* Convert the scaled magnitude to segment number. */
        seg = val_seg (sample_s16);
        aval = (seg << 4) | ((sample_s16 >> (seg + 3)) & 0x0f);
    }
    return aval ^ mask;
}



// Calculate the number of samples per packet
size_t MastCodec_PCMA::frames_per_packet_internal( size_t max_bytes )
{
    int bytes_per_unit = SAMPLES_PER_UNIT * this->channels;
    return (max_bytes / bytes_per_unit) * SAMPLES_PER_UNIT;
}


// Encode a packet's payload
size_t MastCodec_PCMA::encode_packet_internal(
    size_t inputsize,     /* input size in frames */
    mast_sample_t *input,
    size_t outputsize,    /* output size in bytes */
    u_int8_t *output)
{
    size_t i=0;

    if (outputsize < inputsize) {
        MAST_ERROR("encode_pcma: output buffer isn't big enough");
        return 0;
    }

    for (i=0; i < inputsize; i++)
    {
        output[i] = float_to_alaw( input[i] );
    };

    return inputsize;
}


// Decode a packet's payload
size_t MastCodec_PCMA::decode_packet_internal(
    size_t inputsize,    /* input size in bytes */
    u_int8_t  *input,
    size_t outputsize,     /* output size in frames */
    mast_sample_t  *output)
{
    size_t i=0;

    if (outputsize < inputsize) {
        MAST_ERROR("decode_pcma: output buffer isn't big enough");
        return 0;
    }

    for (i=0; i < inputsize; i++)
    {
        output[i] = alaw_to_float( input[i] );
    };


    return inputsize;
}




// Initialise the PCMA codec
MastCodec_PCMA::MastCodec_PCMA( MastMimeType *type)
    : MastCodec(type)
{

    // Set default values
    this->samplerate = PCMA_DEFAULT_SAMPLERATE;
    this->channels = PCMA_DEFAULT_CHANNELS;

    // Apply MIME type parameters to the codec
    this->apply_mime_type_params( type );

}

