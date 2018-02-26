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
#include <netinet/in.h>

#include "MastCodec_MPA.h"
#include "mast.h"

#define MPA_DEFAULT_CHANNELS       (2)
#define MPA_DEFAULT_SAMPLERATE     (44100)



// Only compile this code if TwoLAME is available
#ifdef HAVE_TWOLAME



// Calculate the number of samples per packet
size_t MastCodec_MPA::frames_per_packet_internal( size_t max_bytes )
{
    int frames_per_packet = max_bytes/twolame_get_framelength( this->twolame );
    MAST_DEBUG("MPEG Audio Frames per Packet: %d", frames_per_packet );

    return frames_per_packet*TWOLAME_SAMPLES_PER_FRAME;
}



int MastCodec_MPA::set_param_internal( const char* name, const char* value )
{

    /*    Parameters listed in RFC3555 for audio/MPA:

        layer: 1,2,3
        samplerate:
        mode: "stereo", "joint_stereo", "single_channel", "dual_channel"
        bitrate: the data rate for the audio bit stream
        ptime: duration of each packet in milliseconds
        maxptime: maximum duration of each packet in milliseconds

        Example:
        audio/MPA;layer=2;mode=stereo;bitrate=160;
    */

    if (strcmp(name, "layer")==0) {
        int layer = atoi(value);
        if (layer!=2) return -2;
    } else if (strcmp(name, "mode")==0) {
        TWOLAME_MPEG_mode mode;
        if (strcmp(value, "stereo")==0) mode = TWOLAME_STEREO;
        else if (strcmp(value, "joint_stereo")==0) mode = TWOLAME_JOINT_STEREO;
        else if (strcmp(value, "single_channel")==0) mode = TWOLAME_MONO;
        else if (strcmp(value, "dual_channel")==0) mode = TWOLAME_DUAL_CHANNEL;
        else return -2;

        if (twolame_set_mode( this->twolame, mode )) {
            MAST_WARNING("Failed to set mode");
        }
    } else if (strcmp(name, "bitrate")==0) {
        int bitrate = atoi(value);
        if (twolame_set_bitrate( this->twolame, bitrate )) {
            MAST_WARNING("Failed to set bitrate");
        } else {
            MAST_DEBUG("Set bitrate to %d", bitrate);
        }
    } else {
        // Unsupported parameter
        return -1;
    }

    // Success
    return 0;
}


const char* MastCodec_MPA::get_param_internal( const char* name )
{

    if (strcmp(name, "layer")==0) {
        // We only support layer 2
        return "2";
    } else {
        // Unsupported
        return NULL;
    }

}


// Encode a packet's payload
size_t MastCodec_MPA::encode_packet_internal(
    size_t inputsize,     /* input size in frames */
    mast_sample_t *input,
    size_t outputsize,    /* output size in bytes */
    u_int8_t *output)
{
    int bytes_encoded = 0;

    // MPEG Audio header is 4 bytes at the start of the packet
    output[0] = 0x00;
    output[1] = 0x00;
    output[2] = 0x00;
    output[3] = 0x00;

    // Encode the audio using twolame
    bytes_encoded = twolame_encode_buffer_float32_interleaved( this->twolame, input, inputsize, output+4, outputsize-4 );
    //MAST_DEBUG( "mast_encode_mpa: encoded %d samples to %d bytes (max %d bytes)", inputsize, bytes_encoded, outputsize );

    return bytes_encoded+4;
}




MastCodec_MPA::~MastCodec_MPA()
{
    if (twolame) {
        // De-initialise twolame
        twolame_close( &twolame );
    }

}



// Initialise the codec
MastCodec_MPA::MastCodec_MPA( MastMimeType *type)
    : MastCodec(type)
{

    // Set default values
    this->samplerate = MPA_DEFAULT_SAMPLERATE;
    this->channels = MPA_DEFAULT_CHANNELS;


    // Initialise twolame
    this->twolame = twolame_init();
    if (this->twolame==NULL) {
        MAST_FATAL( "Failed to initialise TwoLame" );
    }

    // Configure twolame
    if (twolame_set_num_channels( this->twolame, this->channels )) {
        MAST_WARNING( "Failed to set number of input channels" );
    }
    if (twolame_set_in_samplerate( this->twolame, this->samplerate )) {
        MAST_WARNING( "Failed to set number of input samplerate" );
    }


    // Apply MIME type parameters to the codec
    this->apply_mime_type_params( type );


    // Get TwoLAME ready to go...
    twolame_init_params( this->twolame );

}


#endif // HAVE_TWOLAME

