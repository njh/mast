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

#include <stdlib.h>
#include <unistd.h>
#include <samplerate.h>

#include "MastResampler.h"
#include "mast.h"




// Constructors
MastResampler::MastResampler( int in_channels, int quality )
{
    int src_err = 0;

    // Initialise the resampling ration
    this->ratio = 1.0f;

    // Store the number of channels
    this->channels = in_channels;

    // Create resampling engine
    src_state = src_new( quality, channels, &src_err );
    if (src_state==NULL || src_err!=0) {
        MAST_FATAL("Failed to initialise SRC resampling engine: %s", src_strerror( src_err ) );
    }


}


// Add audio fames to the buffer
MastResampler::~MastResampler()
{
    // Delete the resampling engine
    src_delete( src_state );
}

// Re-sample one audio buffer to another
MastResampler::resample(MastAudioBuffer *input, MastAudioBuffer *output);
{


    // Check that the number of channels are the same
    if (input->get_channels() != output->get_channels()) {
        MAST_ERROR("Number of channels in input buffer is different to the number of channels in the output buffer");
        return;
    }

    /*


        // Is the samplerate conversion buffer big enough?
        src_buf_len = (input_frames * ratio * 1.1);
        if (src_buf_len > codec->src_buffer_len) {
            codec->src_buffer = realloc( codec->src_buffer, src_buf_len*sizeof(float));
            if (codec->src_buffer==NULL) {
                MAST_FATAL("Failed to allocate memory for samplerate conversion");
            }
            codec->src_buffer_len = src_buf_len;
        }

        // Configure the sample rate conversion
        memset( &params, 0, sizeof(SRC_DATA) );
        params.data_in = input;
        params.input_frames = input_frames;
        params.data_out = codec->src_buffer;
        params.output_frames = codec->src_buffer_len;
        params.src_ratio = ratio;

        // Perform the conversion
        err = src_process(codec->src_state, &params );
        if (err) {
            MAST_FATAL("Failed to perform samplerate conversion: %s", src_strerror( err ) );
        }

        MAST_DEBUG("input_frames=%d, input_frames_used=%d, output_frames=%d, output_frames_gen=%d",
                    params.input_frames, params.input_frames_used, params.output_frames, params.output_frames_gen );

    */


}



