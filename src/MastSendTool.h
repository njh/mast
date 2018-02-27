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
 */

#ifndef _MASTSENDTOOL_H_
#define _MASTSENDTOOL_H_

#include "MastTool.h"
#include "MastMimeType.h"
#include "MastAudioBuffer.h"
#include "MastCodec.h"
//#include "MastResampler.h"

class MastSendTool : public MastTool {


// Constructors
public:
    MastSendTool( const char* name );
    ~MastSendTool();


// Public methods
    void set_input_channels( int in_channels ) {
        this->channels = in_channels;
    };
    void set_input_samplerate( int in_samplerate ) {
        this->samplerate = in_samplerate;
    };
    int get_input_channels() {
        return this->channels;
    };
    int get_input_samplerate() {
        return this->samplerate;
    };
    void run();

// Private methods
    void prepare();


private:
    MastAudioBuffer *input_buffer;
    //MastAudioBuffer *resampled_buffer;
    //MastResampler *resampler;
    MastCodec *codec;                // Codec to encode the audio
    u_int8_t *payload_buffer;        // Payload buffer for encoded audio
    int frames_per_packet;           // Number of audio frames per packet
    int ts;                          // Current transmit timestamp

    int channels;                    // Number of input channels
    int samplerate;                  // Samplerate of input

};


// Each tool implements its own version of this function
size_t mast_fill_input_buffer( MastAudioBuffer* buffer );


#endif // _MASTSENDTOOL_H_
