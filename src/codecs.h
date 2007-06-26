/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2007 University of Southampton
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
 *  $Id:$
 */


#ifndef	_CODECS_H_
#define	_CODECS_H_


typedef struct mast_codec_s {

	// Encode callback - returns number of bytes encoded, or -1 on failure
	u_int32_t (*encode)(struct mast_codec_s *, u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output);
	
	// Decode callback - returns number of samples decoded, or -1 on failure
	u_int32_t (*decode)(struct mast_codec_s *, u_int32_t inputsize, u_int8_t *input, u_int32_t outputsize, int16_t *output);
	
	// De-initialise callback, returns 0 on success or error number
	int (*deinit)(struct mast_codec_s *);

	// Pointer for internal use by codecs
	void* ptr;

} mast_codec_t;


// codecs.c
mast_codec_t* mast_init_codec( char* mime_subtype );


// Specific codec Initialisers
mast_codec_t* mast_init_dvi4();		// codec_dvi4.c
mast_codec_t* mast_init_gsm();		// codec_gsm.c
mast_codec_t* mast_init_l16();		// codec_l16.c
mast_codec_t* mast_init_lpc();		// codec_lpc.c
mast_codec_t* mast_init_pcma();		// codec_alaw.c
mast_codec_t* mast_init_pcmu();		// codec_ulaw.c


#endif
