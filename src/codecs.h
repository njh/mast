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


typedef stuct {

	/* Callbacks */
	u_int32_t (*encode)(u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output);
	u_int32_t (*decode)(u_int32_t inputsize, u_int8_t *input, u_int32_t outputsize, int16_t *output);
	int (*deinit)(struct audio_output_struct *);

}


/* codec_l16.c */

int init_l16();

int delete_l16();

u_int32_t decode_l16(
	       u_int32_t inputsize, 
	       u_int8_t  *input,
	       u_int32_t outputsize, 
	       int16_t  *output);

u_int32_t encode_l16(
	       u_int32_t num_samples,
	       int16_t *input,
	       u_int32_t out_size,
	       u_int8_t *output);


/* codec_alaw.c */

int init_alaw();

int delete_alaw();

u_int32_t decode_alaw(
	       u_int32_t inputsize, 
	       u_int8_t  *input,
	       u_int32_t outputsize, 
	       int16_t  *output);

u_int32_t encode_alaw(
	       u_int32_t num_samples,
	       int16_t *input,
	       u_int32_t out_size,
	       u_int8_t *output);




/* codec_ulaw.c */

int init_ulaw();

int delete_ulaw();

u_int32_t decode_ulaw(
	       u_int32_t inputsize, 
	       u_int8_t  *input,
	       u_int32_t outputsize, 
	       int16_t  *output);

u_int32_t encode_ulaw(
	       u_int32_t num_samples,
	       int16_t *input,
	       u_int32_t out_size,
	       u_int8_t *output);


#endif
