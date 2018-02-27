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

#include <stdlib.h>
#include <unistd.h>

#include "MastSendTool.h"
#include "mast.h"




// Constructors
MastSendTool::MastSendTool( const char* tool_name )
    :MastTool( tool_name, RTP_SESSION_SENDONLY )
{

    // Initialise Stuff
    this->input_buffer = NULL;
    //this->resampled_buffer = NULL;
    //this->resampler = NULL;
    this->codec = NULL;
    this->payload_buffer = NULL;
    this->frames_per_packet = 0;
    this->ts = 0;
    this->channels = DEFAULT_CHANNELS;
    this->samplerate = 0;


}



MastSendTool::~MastSendTool()
{
    // Free up the buffers audio/read buffers
    if (payload_buffer) {
        free(payload_buffer);
        payload_buffer=NULL;
    }

    if (input_buffer) {
        delete input_buffer;
        input_buffer=NULL;
    }

    // Delete Codec object
    if (codec) {
        delete codec;
        codec=NULL;
    }

}


void MastSendTool::prepare()
{

    // Display some information about the chosen payload type
    MAST_INFO( "Sending SSRC: 0x%x", session->snd.ssrc );
    MAST_INFO( "Input Format: %d Hz, %s", samplerate, channels==2 ? "Stereo" : "Mono");
    mimetype->print();


    // Load the codec
    if (codec != NULL) MAST_WARNING("Codec has already been created" );
    codec = MastCodec::new_codec( mimetype );
    if (codec == NULL) MAST_FATAL("Failed to get create codec" );
    MAST_INFO( "Output Format: %d Hz, %s", codec->get_samplerate(), codec->get_channels()==2 ? "Stereo" : "Mono");

    // Work out the payload type to use
    if (strcmp("MPA", codec->get_type())==0) {
        // MPEG Audio is a special case
        this->set_payloadtype_index( RTP_MPEG_AUDIO_PT );
        payloadtype->channels = codec->get_channels();

    } else {
        // Ask oRTP for the index
        int index = ::rtp_profile_find_payload_number( profile, codec->get_type(), codec->get_samplerate(), codec->get_channels() );
        if ( index<0 ) MAST_FATAL("Failed to get payload type information from oRTP");
        this->set_payloadtype_index( index );
    }

    // Calculate the packet size
    frames_per_packet = codec->frames_per_packet( payload_size_limit );
    if (frames_per_packet<=0) MAST_FATAL( "Invalid number of samples per packet" );

    // Create audio buffers
    input_buffer = new MastAudioBuffer( frames_per_packet, samplerate, channels );
    if (input_buffer == NULL) MAST_FATAL("Failed to create audio input buffer");
    //resampled_buffer = new MastAudioBuffer( frames_per_packet, codec->get_samplerate(), codec->get_channels() );
    //if (resampled_buffer == NULL) MAST_FATAL("Failed to create resampled audio buffer");

    // Allocate memory for the packet buffer
    payload_buffer = (u_int8_t*)malloc( payload_size_limit );
    if (payload_buffer == NULL) MAST_FATAL("Failed to allocate memory for payload buffer");

}


void MastSendTool::run()
{
    // Set things up
    this->prepare();

    // The main loop
    while( mast_still_running() )
    {
        int frames_read = ::mast_fill_input_buffer( input_buffer );
        int payload_bytes = 0;

        // Was there an error?
        if (frames_read < 0) break;

        // Encode audio
        payload_bytes = codec->encode_packet( input_buffer, payload_size_limit, payload_buffer );
        if (payload_bytes<0)
        {
            MAST_ERROR("Codec encode failed" );
            break;
        }

        // We used the audio up
        input_buffer->empty_buffer();

        if (payload_bytes) {
            // Send out an RTP packet
            ::rtp_session_send_with_ts(session, payload_buffer, payload_bytes, ts);

            // Calculate the timestamp increment
            ts+=((frames_read * payloadtype->clock_rate) / codec->get_samplerate());   //  * frames_per_packet;
        }


    }


}

