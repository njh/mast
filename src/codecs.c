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

#include "config.h"
#include "codecs.h"
#include "mast.h"


// Table of codecs
static struct {
	char* mime_subtype;
	int (*init)( mast_codec_t* codec );
} mast_codecs[] = {
//	{ "DVI4",	mast_init_dvi4 },
	{ "GSM",	mast_init_gsm },
	{ "L16",	mast_init_l16 },
	{ "LPC",	mast_init_lpc },
	{ "PCMA",	mast_init_pcma },
	{ "PCMU",	mast_init_pcmu },
	{ "MPA",	mast_init_mpa },
	{ NULL, 	NULL }
};


static int istoken( char chr )
{
	if (isalnum(chr)) return TRUE;
	if (chr=='.') return TRUE;
	if (chr=='-') return TRUE;
	if (chr=='+') return TRUE;
	return FALSE;
}


static char* parse_mime_subtype( const char* mime_type )
{
	const char* foundslash=NULL;
	char* subtype = NULL;
	int i=0;

	// Look for a slash from the start of the string	
	for(i=0; i<strlen(mime_type); i++) {
		if (istoken(mime_type[i])) continue;
		if (mime_type[i]=='/') foundslash = &mime_type[i+1];
		break;
	}
	
	// If we found a slash, check the major type is 'audio'
	if (foundslash && strncmp( "audio/", mime_type, 6 )!=0) {
		MAST_FATAL("MIME Type is not audio/*");
	}
	
	// Copy the subtype from the position we think is the start
	if (foundslash) subtype = strdup( foundslash );
	else subtype = strdup( mime_type );
	
	// Now find the end of the subtype
	for(i=0; i<strlen(subtype); i++) {
		if (istoken(subtype[i])) continue;
		subtype[i] = 0;
		break;
	}

	return subtype;
}


// Find a codec and initialise it
mast_codec_t* mast_codec_init( char* mime_type, int samplerate, int channels )
{
	mast_codec_t* codec = NULL;
	char* mime_subtype = NULL;
	int found_codec = FALSE;
	int i;

	// Extract just the subtype from the MIME type string
	mime_subtype = parse_mime_subtype(mime_type);
	MAST_DEBUG("mime_subtype=%s", mime_subtype);

	
	// Allocate memory for the codec
	codec = malloc( sizeof(mast_codec_t) );
	if (codec==NULL) {
		MAST_FATAL("Failed to allocate memory for mast_codec_t data structure");
	}
	
	// Zero the memory and fill in
	memset( codec, 0, sizeof(mast_codec_t) );
	codec->channels = channels;
	codec->samplerate = samplerate;
	codec->subtype = mime_subtype;

	
	// Search for a matching codec
	for(i=0; mast_codecs[i].mime_subtype; i++) {
		if (strcasecmp( mime_subtype, mast_codecs[i].mime_subtype ) == 0) {
			// Found a matching codec
			if (mast_codecs[i].init( codec )) {
				MAST_DEBUG( "Failed to initialise codec" );
				free( codec );
				return NULL;
			}
			found_codec = TRUE;
			break;
		}
	}
	
	// Didn't find match
	if(!found_codec) {
		MAST_ERROR( "Failed to find codec for MIME type audio/%s", mime_subtype );
		free(codec);
		return NULL;
	}
	
	
	
	// FIXME: set parameters
	
	
	// Prepare to start encoding/decoding
	if (codec->prepare( codec )) {
		MAST_WARNING( "Codec failed to prepare to encode/decode." );
		mast_codec_deinit(codec);
		return NULL;
	}
	
	
	return codec;
}


// Get the number of samples per packet for the current parameters
int mast_codec_samples_per_packet( mast_codec_t* codec, int max_bytes )
{
	return codec->samples_per_packet( codec, max_bytes );
}

// Set a codec parameter - returns 0 on success, or error number on failure
int mast_codec_set_param( mast_codec_t* codec, const char* name, const char* value )
{
	if (codec->set_param) {
		return codec->set_param( codec, name, value );
	} else {
		MAST_WARNING("Codec does not support setting parameters");
		return -1;
	}
}

// Get a codec parameter - returns NULL if parameter doesn't exist
const char* mast_codec_get_param( mast_codec_t* codec, const char* name )
{
	if (codec->get_param) {
		return codec->get_param( codec, name );
	} else {
		MAST_WARNING("Codec does not support getting parameters");
		return NULL;
	}
}

// Encode - returns number of bytes encoded, or -1 on failure
int mast_codec_encode( mast_codec_t* codec, u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output )
{
	if (codec->encode) {
		return codec->encode( codec, num_samples, input, out_size, output );
	} else {
		MAST_ERROR("Codec does not support encoding");
		return -1;
	}
}

// Decode  - returns number of samples decoded, or -1 on failure
int mast_codec_decode( mast_codec_t* codec, u_int32_t inputsize, u_int8_t *input, u_int32_t outputsize, int16_t *output )
{
	if (codec->decode) {
		return codec->decode( codec, inputsize, input, outputsize, output );
	} else {
		MAST_ERROR("Codec does not support decoding");
		return -1;
	}
}


void mast_codec_deinit( mast_codec_t* codec )
{

	if (codec) {
		if (codec->deinit) {
			// Codec specific deinitialisation
			codec->deinit( codec );
		}
		
		free( codec );
	}

}


