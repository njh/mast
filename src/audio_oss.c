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

#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "audio.h"


#ifdef HAVE_OSS

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <fcntl.h>


/* Some systems don't have AFMT_S16_NE */
#ifndef AFMT_S16_NE
# ifdef WORDS_BIGENDIAN
#   define AFMT_S16_NE AFMT_S16_BE
# else
#   define AFMT_S16_NE AFMT_S16_LE
# endif
#endif


static void
init_oss_soundcard( audio_t *audio ) {

	int format = AFMT_S16_NE;
	int channels = audio->channels;
	int speed = audio->samplerate;
	


	/* Reset the sound card */
	if (ioctl(fileno(audio->fp), SNDCTL_DSP_RESET, 0)<0) {
		perror( "SNDCTL_DSP_RESET" );
		exit(1);
	}
	

	/* Native endian Signed 16-bit audio */
	if (ioctl(fileno(audio->fp), SNDCTL_DSP_SETFMT, &format)<0) {
		perror( "SNDCTL_DSP_SETFMT" );
		exit(1);
	}
	if (format != AFMT_S16_NE) {
		fprintf(stderr, "audio_oss_open_input: failed to set format.\n");
		exit(1);
	}
	
	/* Set the number of channels */
	if (ioctl(fileno(audio->fp), SNDCTL_DSP_CHANNELS, &channels)<0) {
		perror( "SNDCTL_DSP_CHANNELS" );
		exit(1);
	}
	if (channels != audio->channels) {
		fprintf(stderr, "audio_oss_open_input: failed to set number if channels to %d\n", audio->channels);
		exit(1);
	}
	

	/* Set the sample rate */
	if (ioctl(fileno(audio->fp), SNDCTL_DSP_SPEED, &speed)<0) {
		perror( "SNDCTL_DSP_SPEED" );
		exit(1);
	}
	
	/* Is the selected sample rate within 5% ? */
	if (speed != audio->samplerate) {
		if (((100 * abs(speed - audio->samplerate)) / audio->samplerate) > 5
		    || speed < audio->samplerate) {
			fprintf(stderr, "audio_oss_open_input: failed to set sample rate to %d (closest is %d).\n",
			audio->samplerate, speed);
			exit(1);
		} else {
			audio->skew = speed - audio->samplerate;
			fprintf(stderr, "audio_oss_open_input: working arround audio skew (%d vs %d).\n",
				audio->samplerate, speed);
		}
	}


}


void
audio_oss_open_input( audio_t *audio, char* devname )
{

	if ((audio->fp = fopen(devname, "rb")) == NULL) {
		perror( "Can't open sound card for input" );
		exit(1);
	}
	
	
	/* Initialise the sound card */
	init_oss_soundcard( audio );
		
}

void
audio_oss_open_output( audio_t *audio, char* devname )
{

	if ((audio->fp = fopen(devname, "wb")) == NULL) {
		perror( "Can't open sound card for output" );
		exit(1);
	}
	
	
	/* Initialise the sound card */
	init_oss_soundcard( audio );

}



/* Fetch, encode and return X bytes of audio */
int audio_oss_read( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)
	unsigned int read_samples=0;
	static int skew_counter=0;



	/* Fix skew with some Linux sound cards */
	if (skew_counter>audio->samplerate && audio->skew) {
		int samp = audio->skew * audio->channels;
	
		read_samples = fread( audio->buffer, 2, samp, audio->fp );
		if (read_samples == -1) {
			perror("audio read skew from sound card");
			exit(1);
		}
		
		skew_counter -= audio->samplerate;
	}


	/* Read from Sound card */
	read_samples = fread( audio->buffer, 2, samples, audio->fp );
	if (read_samples == -1) {
		perror("audio read from sound card");
		exit(1);
	}
	
	
	/* Increment counter until next skew fix */
	if (audio->skew) skew_counter += read_samples;


	return read_samples / audio->channels;
}


void
audio_oss_write( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)
	int samples_written = fwrite( audio->buffer, 2, samples, audio->fp );
	if (samples_written == -1) {
		perror("audio write to OSS");
		exit(1);
	}
	
	if (samples_written != samples) {
		fprintf(stderr, "audio_oss_write: Didn't write all the data I was supposed to.\n");
	}

}


/* Close audio */
void audio_oss_close( audio_t* audio )
{
	if ( audio->fp ) {
		fclose( audio->fp );
		audio->fp = NULL;
	}
}


#else

void audio_oss_open_input( audio_t *audio, char* devname )
{ fprintf(stderr, "Linux soundcard support (OSS) isn't available.\n"); exit(1); }

void audio_oss_open_output( audio_t *audio, char* devname )
{ fprintf(stderr, "Linux soundcard support (OSS) isn't available.\n"); exit(1); }

int  audio_oss_read( audio_t* audio, int bytes )
{ fprintf(stderr, "Linux soundcard support (OSS) isn't available.\n"); exit(1); return 0; }

void  audio_oss_write( audio_t* audio, int decoded_bytes )
{ fprintf(stderr, "Linux soundcard support (OSS) isn't available.\n"); exit(1); }

void audio_oss_close( audio_t* audio )
{ fprintf(stderr, "Linux soundcard support (OSS) isn't available.\n"); exit(1); }



#endif
