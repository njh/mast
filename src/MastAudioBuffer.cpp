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

#include "MastAudioBuffer.h"
#include "mast.h"




// Constructors
MastAudioBuffer::MastAudioBuffer( size_t frames, int samplerate, int channels )
{
	size_t buffer_bytes = frames * channels * sizeof( mast_sample_t );

	// Store the parameters
	this->channels = channels;
	this->samplerate = samplerate;
	this->buffer_size = frames;

	// Allocate memory
	this->buffer = (mast_sample_t*)malloc( buffer_bytes );
	if (this->buffer==NULL) MAST_FATAL("Failed to allocate memory for audio buffer");
	this->buffer_used = 0;
	
}


// Add audio fames to the buffer
void MastAudioBuffer::add_frames( size_t frames )
{
	if (buffer_used + frames > buffer_size) {
		MAST_WARNING("No space in buffer to add %d frames", frames);
	}

	// Add on the frames to the buffer usage
	buffer_used += frames;
}


// Remove audio from the buffer
void MastAudioBuffer::remove_frames( size_t frames )
{
	size_t used_bytes = sizeof(mast_sample_t) * frames * channels;
	size_t buffer_bytes = buffer_size * channels * sizeof( mast_sample_t );
	size_t remaining_bytes = buffer_bytes-used_bytes;
	
	// Check that the number of frames is valid
	if (frames > this->buffer_used) {
		MAST_ERROR("Tried to remove more frames than were in the buffer");
		return;
	}

	// Move remaining audio to the start of the buffer
	if (remaining_bytes) {
		MAST_DEBUG( "Moving %d bytes remaining in audio buffer", remaining_bytes );
		memmove( this->buffer, this->buffer + used_bytes, remaining_bytes );
	}
	
	// Change the number of frames available in buffer
	this->buffer_used -= frames;
}


// Destructor
MastAudioBuffer::~MastAudioBuffer()
{
	// Free the memory used by the buffer
	if (this->buffer) {
		free( this->buffer );
		this->buffer = NULL;
	}

}



