/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2005 University of Southampton
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 */


#ifndef	_AUDIO_H_
#define	_AUDIO_H_

#include "mast.h"


typedef struct audio_struct {

	int channels;
	int samplerate;
	int format;
	
	/* Size in bytes of a single frame of encoded audio */
	int encoded_frame_size;
	
	/* Size in bytes of a single frame of raw audio */
	//int raw_frame_size;
	
	short* buffer;	/* Audio sample buffer */
	int buf_size;	/* Number of samples that fit in buffer */
	
	/* Sound file */
	void* ptr;
	
	/* Sound card */
	FILE *fp;
	int skew;
		
	
	// Callback to read in some audio
	int(*read_callback)( struct audio_struct* audio, int samples );
	void(*write_callback)( struct audio_struct* audio, int samples );
	void(*close_callback)( struct audio_struct* audio );
	
	u_int32_t(*codec_encode_callback)( u_int32_t num_samples, int16_t *input, u_int32_t out_size, u_int8_t *output);
	u_int32_t(*codec_decode_callback)( u_int32_t inputsize, u_int8_t  *input, u_int32_t outputsize, int16_t  *output);
	int(*codec_close_callback)();

	
} audio_t;



/* Generic interface */
extern audio_t* audio_open_encoder( config_t *config );
extern audio_t* audio_open_decoder( config_t *config );
extern int audio_encode( audio_t* audio, char* buffer, int samples );
extern void audio_decode( audio_t* audio, char* buffer, int samples );
extern void audio_close( audio_t* audio );


/* STDIN/STDOUT */
extern void audio_stdio_write( audio_t* audio, int samples );
extern int  audio_stdio_read( audio_t* audio, int samples );

/* File */
extern void audio_file_open_input( audio_t *audio, char* filename );
extern void audio_file_open_output( audio_t *audio, char* filename );
extern void audio_file_write( audio_t* audio, int samples );
extern int  audio_file_read( audio_t* audio, int samples );
extern void audio_file_rewind( audio_t* audio );
extern void audio_file_close( audio_t* audio );

/* OSS Soundcard */
extern void audio_oss_open_input( audio_t *audio, char* devname );
extern void audio_oss_open_output( audio_t *audio, char* devname );
extern int  audio_oss_read( audio_t* audio, int samples );
extern void audio_oss_write( audio_t* audio, int samples );
extern void audio_oss_close( audio_t* audio );




#endif

