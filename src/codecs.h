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


#ifndef	_CODECS_H_
#define	_CODECS_H_


/* this data structure shouldn't really be accessed from outside of codec subsystem */
typedef struct mast_codec_s {

	// Number of channels (for encoding)
	int channels;
	
	// Sample rate (for encoding)
	int samplerate;
	
	// Identifier for this codec
	const char* subtype;

	// Codec specific callbacks - can be NULL if unsupported
	int (*samples_per_packet)(struct mast_codec_s *, int max_bytes);
	int (*set_param)(struct mast_codec_s *, const char* name, const char* value);
	const char* (*get_param)(struct mast_codec_s *, const char* name);
	u_int32_t (*encode_packet)(struct mast_codec_s *, u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output);
	u_int32_t (*decode_packet)(struct mast_codec_s *, u_int32_t inputsize, u_int8_t *input, u_int32_t outputsize, int16_t *output);
	int (*deinit)(struct mast_codec_s *);
	
	// Called just before we start encoding/decoding
	// - returns 0 on success, or error number on failure
	int (*prepare)(struct mast_codec_s *);


	// Pointer for internal use by codecs
	void* ptr;

} mast_codec_t;


// Specific codec Initialisers
//int mast_init_dvi4(mast_codec_t*);	// codec_dvi4.c
int mast_init_gsm(mast_codec_t*);		// codec_gsm.c
int mast_init_l16(mast_codec_t*);		// codec_l16.c
int mast_init_lpc(mast_codec_t*);		// codec_lpc.c
int mast_init_mpa(mast_codec_t*);		// codec_mpa.c
int mast_init_pcma(mast_codec_t*);		// codec_alaw.c
int mast_init_pcmu(mast_codec_t*);		// codec_ulaw.c



/*------- Public Functions --------*/

// Find codec, initialise, configure and prepare
// Parameters samplerate and channels SHOULD be given when encoding
// and MUST be set to zero when decoding.
mast_codec_t* mast_codec_init( const char* subtype, int samplerate, int channels );

// Get the number of samples per packet for the current parameters
int mast_codec_samples_per_packet( mast_codec_t* codec, int max_bytes );

// Set a codec parameter - returns 0 on success, or error number on failure
int mast_codec_set_param( mast_codec_t* codec, const char* name, const char* value );

// Get a codec parameter - returns NULL if parameter doesn't exist
const char* mast_codec_get_param( mast_codec_t* codec, const char* name );

// Encode a packet of audio - returns number of bytes encoded, or -1 on failure
int mast_codec_encode_packet( mast_codec_t* codec, u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output );

// Decode a packet of audio - returns number of samples decoded, or -1 on failure
int mast_codec_decode_packet( mast_codec_t* codec, u_int32_t inputsize, u_int8_t *input, u_int32_t outputsize, int16_t *output );

// De-initialise codec, returns 0 on success or error number
void mast_codec_deinit( mast_codec_t* codec );




#endif
