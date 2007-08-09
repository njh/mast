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


#ifndef	_MASTAUDIOBUFFER_H_
#define	_MASTAUDIOBUFFER_H_

#include <sys/types.h>
#include "mast.h"


class MastAudioBuffer {

// Constructors
public:
	MastAudioBuffer(size_t frames, int samplerate, int channels);
	~MastAudioBuffer();


// Public methods
	//int resize(size_t samples);
	
	// Return the number of channels per frame
	int get_channels() { return this->channels; };

	// Return the samplerate of the audio in the buffer
	int get_samplerate() { return this->samplerate; };
	
	// Return the total size of the buffer (in frames)
	size_t get_buffer_size() { return this->buffer_size; };
	
	// Return the number of frames available for reading
	size_t get_read_space() { return this->buffer_used; };
	
	// Return amount of space (in frames) available for writing to
	size_t get_write_space()  { return this->buffer_size - this->buffer_used; };

	// Returns pointer to location to start reading frames	
	mast_sample_t* get_read_ptr() { return this->buffer; };
	
	// Returns pointer to location to start writing frames
	mast_sample_t* get_write_ptr() { return &this->buffer[ this->buffer_used * this->channels ]; }

	// Some data has been added to the buffer
	void add_frames( size_t frames );

	// Some data has been added to the buffer
	void remove_frames( size_t frames );

	// Delete the contents of the buffer
	void empty_buffer() { this->buffer_used = 0; };



private:
	int channels;
	int samplerate;

	size_t buffer_size;
	size_t buffer_used;

	mast_sample_t *buffer;

};


#endif	// _MASTAUDIOBUFFER_H_
