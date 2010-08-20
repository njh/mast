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
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "MastCodec.h"
#include "MastCodec_GSM.h"
#include "MastCodec_L16.h"
#include "MastCodec_LPC.h"
#include "MastCodec_MPA.h"
#include "MastCodec_PCMA.h"
#include "MastCodec_PCMU.h"
#include "MastMimeType.h"
#include "mast.h"



// Factory class method to create sub-class of the desired type
MastCodec* MastCodec::new_codec( MastMimeType* type )
{
	const char* subtype = type->get_minor();
	
	// Make sure it is audio/*
	if (strcmp(type->get_major(), "audio")) {
		MAST_ERROR( "MastCodec only supports MIME type audio/*");
		return NULL;
	}

	// There must be a way of doing this using an loop/array of classes in C++
	if (strcmp( subtype, "GSM")==0) return new MastCodec_GSM( type );
	else if (strcmp( subtype, "L16")==0) return new MastCodec_L16( type );
	else if (strcmp( subtype, "LPC")==0) return new MastCodec_LPC( type );
#ifdef HAVE_TWOLAME
	else if (strcmp( subtype, "MPA")==0) return new MastCodec_MPA( type );
#endif
	else if (strcmp( subtype, "PCMA")==0) return new MastCodec_PCMA( type );
	else if (strcmp( subtype, "PCMU")==0) return new MastCodec_PCMU( type );
	//else if (strcmp( subtype, "G722")==0) return new MastCodec_G722( type );
	//else if (strcmp( subtype, "DVI4")==0) return new MastCodec_DVI4( type );
	else {
		MAST_ERROR( "Failed to find codec for MIME type %s/%s", type->get_major(), type->get_minor() );
		return NULL;
	}

}


MastCodec::MastCodec( MastMimeType* type )
{

	// Initialise the codec's paramters to defaults
	this->samplerate = 0;
	this->channels = 0;
	this->type = type->get_minor();

}


MastCodec::~MastCodec()
{
	MAST_DEBUG("MastCodec sub-class does not implement de-constructor");
}


void MastCodec::apply_mime_type_params( MastMimeType *type )
{
	int i;

	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		MastMimeTypeParam* param = type->get_param(i);
		if (param) {
			int err = this->set_param( param->get_name(), param->get_value() );
			if (err==-1) {
				MAST_FATAL("Parameter '%s' is not supported by codec", param->get_name());
			} else if (err==-2) {
				MAST_FATAL("Value '%s' is not supported by paramter", param->get_value());
			}
		}
	}
}

// Get the number of samples per packet for the current parameters
size_t MastCodec::frames_per_packet( size_t max_bytes )
{
	int frames_per_packet = this->frames_per_packet_internal( max_bytes );
	if (frames_per_packet < 1) {
		MAST_DEBUG("frames_per_packet is less than 1");
		return -1;
	}
	
	MAST_DEBUG("Frames Per Packet: %d", frames_per_packet );
	MAST_INFO("Packet duration (ptime): %d ms", (frames_per_packet*1000)/samplerate );
	
	return frames_per_packet;
}

// Set a codec parameter - returns 0 on success, or error number on failure
int MastCodec::set_param( const char* name, const char* value )
{
	
	// Automatically alllow parameter if no change is required
	if (strcmp(name, "channels")==0 && atoi(value)==this->channels) {
		return 0;
	}
	if (strcmp(name, "samplerate")==0 && atoi(value)==this->samplerate) {
		return 0;
	}

	// Pass it to the codec's method
	return this->set_param_internal( name, value );
}

// Get a codec parameter - returns NULL if parameter doesn't exist
const char* MastCodec::get_param( const char* name )
{
	// Can we deal with it?
	if (strcmp(name, "channels")==0) {
		if (this->channels==1) return "1";
		else if (this->channels==2) return "2";
		else {
			MAST_WARNING("Number of channels isn't 1 or 2: %d", this->channels);
			return "?";
		}
	}

	// Call the codec's method
	return this->get_param_internal( name );
	
	//return NULL;
}

// Encode - returns number of bytes encoded, or -1 on failure
size_t MastCodec::encode_packet( MastAudioBuffer *input, size_t outsize, u_int8_t *output )
{
	size_t num_frames = frames_per_packet_internal( outsize );
	mast_sample_t *samples = input->get_read_ptr();
	size_t bytes = -1;
	
	// Check that the input buffer is the same format as the packet payload
	if (input->get_channels() != this->get_channels()) {
		MAST_WARNING( "Input buffer doesn't have same number of channels as codec" );
	}
	if (input->get_samplerate() != this->get_samplerate()) {
		MAST_WARNING( "Input buffer doesn't have same samplerate as codec" );
	}
	
	// Check that there is enough audio in the input buffer
	if (input->get_read_space() < num_frames) {
		MAST_WARNING( "Input buffer doesn't have audio to read from" );
	}
	

	// Call the codec's method
	bytes = this->encode_packet_internal( num_frames, samples, outsize, output );
	
	// Remove the frames we have used from the buffer
	input->remove_frames( num_frames );
	
	return bytes;
}


// Decode  - returns number of samples decoded, or -1 on failure
size_t MastCodec::decode_packet( size_t inputsize, u_int8_t *input, MastAudioBuffer *output )
{
	
	// Check that the output buffer is the same format as the packet payload
	if (output->get_channels() != this->get_channels()) {
		MAST_WARNING( "Output buffer have same number of channels as codec" );
	}
	if (output->get_samplerate() != this->get_samplerate()) {
		MAST_WARNING( "Output buffer have same number of channels as codec" );
	}


	// Call the codec's method
	return this->decode_packet_internal( inputsize, input, output->get_write_space(), output->get_write_ptr() );
}




/*
 *   The following methods should be overridden in the actually codecs / subclasses
 */




// Internal/virtual: Set a codec parameter - returns 0 on success, or error number on failure
int MastCodec::set_param_internal( const char* name, const char* value )
{
	MAST_ERROR("Codec does not support setting parameters");
	return -1;
}

// Internal/virtual: Get a codec parameter - returns NULL if parameter doesn't exist
const char* MastCodec::get_param_internal( const char* name )
{
	MAST_ERROR("Codec does not support getting parameters");
	return NULL;
}

// Internal/virtual: Get the number of frames per packet for the current parameters
size_t MastCodec::frames_per_packet_internal( size_t max_bytes )
{
	MAST_ERROR("Codec does not support getting number of frames per packet");
	return -1;
}

// Internal/virtual: Encode - returns number of bytes encoded, or -1 on failure
size_t MastCodec::encode_packet_internal( size_t input_frames, float *input, size_t out_size, u_int8_t *output )
{
	MAST_ERROR("Codec does not support encoding");
	return -1;
}

// Internal/virtual: Decode - returns number of samples decoded, or -1 on failure
size_t MastCodec::decode_packet_internal( size_t inputsize, u_int8_t *input, size_t outputsize, float *output )
{
	MAST_ERROR("Codec does not support decoding");
	return -1;
}



