/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003 University of Southampton
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


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "config.h"
#include "audio.h"
#include "pcm6cast.h"
#include "codecs.h"


static
audio_t* audio_init_record( )
{
	audio_t *audio = (audio_t*)malloc(sizeof(audio_t));

	audio->channels = 0;
	audio->samplerate = 0;
	audio->format = -1;
	audio->encoded_frame_size = 0;
	
	audio->ptr = NULL;
	audio->fp = NULL;
	audio->skew = 0;
	
	audio->buffer = NULL;
	audio->buf_size = 0;
	
	/* Set the callbacks to NULL */
	audio->read_callback = NULL;
	audio->write_callback = NULL;
	audio->close_callback = NULL;

	audio->codec_encode_callback = NULL;
	audio->codec_decode_callback = NULL;
	audio->codec_close_callback = NULL;
	
	return audio;
}


static void
audio_init_codec( audio_t *audio, int payload_type, int payload_size )
{

	/* Channels depends on the payload */
	audio->format = payload_type;
	switch(audio->format) {
		case 0:
			init_ulaw();
			audio->codec_encode_callback = encode_ulaw;
			audio->codec_decode_callback = decode_ulaw;
			audio->codec_close_callback = delete_ulaw;

			audio->channels = 1;
			audio->samplerate = 8000;
			audio->buf_size = payload_size*2;
			audio->encoded_frame_size = 1 * audio->channels;
		break;
		case 8:
			init_alaw();
			audio->codec_encode_callback = encode_alaw;
			audio->codec_decode_callback = decode_alaw;
			audio->codec_close_callback = delete_alaw;

			audio->channels = 1;
			audio->samplerate = 8000;
			audio->buf_size = payload_size*2;
			audio->encoded_frame_size = 1 * audio->channels;
		break;
		case 10:
			init_l16();
			audio->codec_encode_callback = encode_l16;
			audio->codec_decode_callback = decode_l16;
			audio->codec_close_callback = delete_l16;

			audio->channels = 2;
			audio->samplerate = 44100;
			audio->buf_size = payload_size;
			audio->encoded_frame_size = 2 * audio->channels;
		break;
		case 11:
			init_l16();
			audio->codec_encode_callback = encode_l16;
			audio->codec_decode_callback = decode_l16;
			audio->codec_close_callback = delete_l16;

			audio->channels = 1;
			audio->samplerate = 44100;
			audio->buf_size = payload_size;
			audio->encoded_frame_size = 2 * audio->channels;
		break;
		default:
			fprintf(stderr, "Error: unsupported payload: %d\n", payload_type);
			exit(2);
		break;
	}


	/* Allocate the audio decoding buffer */
	if (audio->buf_size) {
		audio->buffer = (short*)malloc( audio->buf_size * sizeof(short) );
		if (!audio->buffer) {
			perror("Error: failed to allocate audio decoding buffer.\n");
			exit(2);
		}
	}

}


audio_t* audio_open_encoder( config_t *config ) 
{
	audio_t *audio = audio_init_record( );


	/* if no payload has been chosen the use default */
	if (config->payload_type == -1) config->payload_type = DEFAULT_PAYLOAD;


	/* Decide the payload size */
	if (config->payload_size<=0) {
		switch(config->payload_type) {
			case  0: config->payload_size = 320;  break;
			case  8: config->payload_size = 320;  break;
			case 10: config->payload_size = 1280; break;
			case 11: config->payload_size = 640;  break;
		}
	}


	/* Configure the codec/encoder */
	audio_init_codec( audio, config->payload_type, config->payload_size );
	
	
	/* Open input file */
	if (config->use_stdio) {
		audio->read_callback = audio_stdio_read;
	}


#ifdef USE_SNDFILE
	else if (config->filename) {
		audio_file_open_input( audio, config->filename );
		audio->read_callback = audio_file_read;
		audio->close_callback = audio_file_close;
		
	}
#endif
	
#ifdef USE_OSS
	else if (config->devname) {
		// open audio device
		audio_oss_open_input( audio, config->devname );
		audio->read_callback = audio_oss_read;
		audio->close_callback = audio_oss_close;
	}
#endif	
	
	/* Call the callback if there is one */
	if (config->audio_init_callback) {
		config->audio_init_callback( (void*)audio );
	}
	
	
	return audio;
}


audio_t* audio_open_decoder( config_t *config )
{
	audio_t *audio = audio_init_record( );

	
	/* Initialise audio decoding / codec */
	audio_init_codec( audio, config->payload_type, config->payload_size );


	if (config->use_stdio) {
		/* Send to stdout */
		audio->write_callback = audio_stdio_write;
		
	}

#ifdef USE_SNDFILE
	else if (config->filename) {
		/* Open output file */
		audio_file_open_output( audio, config->filename );
		audio->write_callback = audio_file_write;
		audio->close_callback = audio_file_close;
	}
#endif

#ifdef USE_SDL	
	else if (config->use_sdl) {
		// open SDL for output
		audio_sdl_open_output( audio );
		audio->write_callback = audio_sdl_write;
		audio->close_callback = audio_sdl_close;
	}
#endif

#ifdef USE_OSS
	else if (config->devname) {
		// open audio device
		audio_oss_open_output( audio, config->devname );
		audio->write_callback = audio_oss_write;
		audio->close_callback = audio_oss_close;
	}
#endif	


	/* Call the callback if there is one */
	if (config->audio_init_callback) {
		config->audio_init_callback( (void*)audio );
	}

	return audio;
}



/* Decode and output audio */
void audio_decode( audio_t* audio, char* buffer, int bytes )
{
	int decoded_samples;


	/* Decode the audio */
	if (audio->codec_decode_callback) {
		decoded_samples = audio->codec_decode_callback( bytes, buffer, audio->buf_size, audio->buffer );
	} else {
		fprintf(stderr, "Error codec_decode_callback is not set.\n");
		exit(1);
	}

	if (decoded_samples <= 0 && bytes > 0) {
		fprintf(stderr, "Failed to decode audio.\n");
		return;
	}
	
	
	//fprintf(stderr,"audio_decode: bytes_in=%d, samples_out=%d\n", bytes, decoded_samples);
	
	/* Call the callback to write audio out */
	if (audio->write_callback) {
		audio->write_callback( audio, decoded_samples );
	} else {
		fprintf(stderr, "Error write_callback is not set.\n");
	}
		
}



/* Fetch, encode and return X bytes of audio */
int audio_encode( audio_t* audio, char* buffer, int bytes_wanted )
{
	unsigned int read_samples=0, endcoded_bytes, samples_to_read;

	/* How many samples do we need to read to be encoded to 'bytes */
	if (audio->format == 8 || audio->format == 0) {
		samples_to_read = bytes_wanted;
	} else {
		samples_to_read = bytes_wanted / sizeof( short );
	}	
	


	/* Is audio buffer big enough ? */ 
	if (samples_to_read > audio->buf_size) {
		fprintf(stderr, "encode_audio: Error audio buffer isn't big enough.\n");
		fprintf(stderr, "  audio buffer = %d samples\n", audio->buf_size);
		fprintf(stderr, "  needed = %d samples\n", samples_to_read);
		exit(1);
	}
	

	/* Call the callback to read audio in */
	if (audio->read_callback) {
		read_samples = audio->read_callback( audio, samples_to_read );
	} else {
		fprintf(stderr, "Error read_callback is not set.\n");
		exit(1);
	}


	/* Not enough ?  - put silence on the end  */
	if (read_samples < samples_to_read && read_samples > 0) {
		int i;
		for(i=read_samples; i<samples_to_read; i++) {
			audio->buffer[i] = 0;
		}
		read_samples = samples_to_read;
	}
	
	/* Encode the audio */
	if (audio->codec_encode_callback) {
		endcoded_bytes = audio->codec_encode_callback( read_samples, audio->buffer, bytes_wanted, buffer );
	} else {
		fprintf(stderr, "Error codec_encode_callback is not set.\n");
		exit(1);
	}

	//fprintf(stderr, "bytes_wanted=%d,   read_samples=%d,   read_bytes=%d,   endcoded_bytes=%d\n",
	//		bytes_wanted, read_samples, read_bytes, endcoded_bytes);
	
	
	return endcoded_bytes;
}




/* Close audio */
void audio_close( audio_t* audio )
{
	if (!audio) return;

	/* Close the codec */
	if (audio->codec_close_callback) {
		audio->codec_close_callback();
	}
	
	/* Clean up audio buffer */
	if (audio->buffer) {
		free(audio->buffer);
		audio->buffer = NULL;
	}
	
	
	/* Shut down audio I/O system */
	if (audio->close_callback) {
		audio->close_callback( audio );
	}
	
	free( audio );
}



