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


#ifndef _MAST_CODEC_H_
#define _MAST_CODEC_H_

#include "MastAudioBuffer.h"
#include "MastMimeType.h"
#include "mast.h"



class MastCodec {

public:
    // Static/class methods
    static MastCodec* new_codec( MastMimeType* type );


public:
    // Constructor
    MastCodec( MastMimeType* type );
    virtual ~MastCodec();


    // Public: Get the number of samples per packet for the current parameters
    size_t frames_per_packet( size_t max_bytes );

    // Set a codec parameter - returns 0 on success, or error number on failure
    int set_param( const char* name, const char* value );

    // Get a codec parameter - returns NULL if parameter doesn't exist
    const char* get_param( const char* name );

    // Encode a packet of audio - returns number of bytes encoded, or -1 on failure
    size_t encode_packet( MastAudioBuffer *input, size_t out_size, u_int8_t *output );

    // Decode a packet of audio - returns number of samples decoded, or -1 on failure
    size_t decode_packet( size_t inputsize, u_int8_t *input, MastAudioBuffer *output );


    // Get codec parameters
    int get_channels() {
        return this->channels;
    };
    int get_samplerate() {
        return this->samplerate;
    };
    const char* get_type() {
        return this->type;
    };



protected:

    // Internal: set a codec parameter - returns 0 on success, or error number on failure
    virtual int set_param_internal( const char* name, const char* value );

    // Internal: get a codec parameter - returns NULL if parameter doesn't exist
    virtual const char* get_param_internal( const char* name );

    // Internal: get the number of frames per packet for the current parameters
    virtual size_t frames_per_packet_internal( size_t max_bytes );

    // Internal: encode a packet of audio - returns number of bytes encoded, or -1 on failure
    virtual size_t encode_packet_internal( size_t num_frames, mast_sample_t *input, size_t out_size, u_int8_t *output );

    // Internal: decode a packet of audio - returns number of samples decoded, or -1 on failure
    virtual size_t decode_packet_internal( size_t inputsize, u_int8_t *input, size_t outputsize, mast_sample_t *output );


    // Apply MIME type parameters to codec settings
    void apply_mime_type_params( MastMimeType *type );

    // Audio input parameters
    int channels;
    int samplerate;

    // Identifier for this codec
    const char* type;

};




#endif // _MAST_CODEC_H_
