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


#ifndef	_MAST_CODEC_MPA_H_
#define	_MAST_CODEC_MPA_H_

#include "mast.h"
#include "MastCodec.h"


#ifdef HAVE_TWOLAME
#include <twolame.h>
#endif




class MastCodec_MPA : public MastCodec {

public:

	// Constructor
	MastCodec_MPA( MastMimeType* type );
	virtual ~MastCodec_MPA();

protected:

	// Set a codec parameter - returns 0 on success, or error number on failure
	virtual int set_param_internal( const char* name, const char* value );
	
	// Get a codec parameter - returns NULL if parameter doesn't exist
	virtual const char* get_param_internal( const char* name );

	// Internal: encode a packet of audio - returns number of bytes encoded, or -1 on failure
	virtual size_t encode_packet_internal( size_t num_frames, mast_sample_t *input, size_t out_size, u_int8_t *output );

	// Internal: return the number of frames per packet for the current parameters
	virtual size_t frames_per_packet_internal( size_t max_bytes );

private:

#ifdef HAVE_TWOLAME
	twolame_options *twolame;
#endif

};




#endif	// _MAST_CODEC_GSM_H_
