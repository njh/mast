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


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "audio.h"


#ifdef USE_SDL

#include <SDL.h>
#include "sfifo.h"


/* The audio function callback takes the following parameters:
       stream:  A pointer to the audio buffer to be filled
       len:     The length (in bytes) of the audio buffer
*/
static void
audio_callback_sdl(void *udata, Uint8 *stream, int len)
{
	audio_t *audio = (audio_t*)udata;
	int read;


	/* Only play if we have data left */
	if ( sfifo_used( &audio->fifo ) < len ) {
		fprintf(stderr, "Didn't have any audio data for SDL (buffer underflow) :(\n");
		SDL_PauseAudio(1);
		return;
	}


	/* Read audio from FIFO to SDL's buffer */
	read = sfifo_read( &audio->fifo, stream, len );
	
	if (len!=read)
		fprintf(stderr, "Error reading from the FIFO (wanted=%u, read=%u).\n", len, read);

} 


void 
audio_sdl_open_output( audio_t* audio )
{

	SDL_AudioSpec wanted;

	// Initalise SDL
	if (SDL_Init( SDL_INIT_AUDIO ) ) {
		fprintf(stderr, "Failed to initialise SDL: %s\n", SDL_GetError());
		exit(2);
	}

	/* L16 uncompressed audio data, using 16-bit signed representation in twos 
	   complement notation - system endian-ness. */
	wanted.format = AUDIO_S16SYS;
	wanted.samples = 1024;  /* Good low-latency value for callback */ 
	wanted.callback = audio_callback_sdl; 
	wanted.userdata = audio; 
	wanted.channels = audio->channels; 
	wanted.freq = audio->samplerate; 

	
	/* Open the audio device, forcing the desired format */
	if ( SDL_OpenAudio(&wanted, NULL) ) {
		fprintf(stderr, "Couldn't open SDL audio: %s\n", SDL_GetError());
		exit(2);
	}


	// Initalise FIFO (with 1 second buffer)
	sfifo_init( &audio->fifo, audio->samplerate * audio->channels * sizeof(short) );
	sfifo_flush( &audio->fifo );

}



void
audio_sdl_write( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)

	/* Bung decoded audio into the FIFO 
		 SDL Audio locking probably isn't actually needed
		 as SFIFO claims to be thread safe...
	*/
	SDL_LockAudio();
	sfifo_write( &audio->fifo, (char*)audio->buffer, samples*sizeof(short));
	SDL_UnlockAudio();
	

	/* Unpause once the buffer is 80% full */
	if (sfifo_used(&audio->fifo) > (sfifo_size(&audio->fifo)*0.8) ) SDL_PauseAudio(0);
		
}



/* Close audio */
void audio_sdl_close( audio_t* audio )
{

	SDL_CloseAudio();
	
	SDL_Quit();
}


#else

void audio_sdl_open_output( audio_t* audio )
{ fprintf(stderr, "SDL isn't available.\n"); exit(1); }

void audio_sdl_write( audio_t* audio, int decoded_bytes )
{ fprintf(stderr, "SDL isn't available.\n"); exit(1); }

void audio_sdl_close( audio_t* audio )
{ fprintf(stderr, "SDL isn't available.\n"); exit(1); }

#endif
