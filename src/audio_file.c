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
#include "mast_config.h"
#include "audio.h"



#ifdef HAVE_SNDFILE

#include <sndfile.h>


void
audio_file_open_input( audio_t *audio, char* filename  )
{
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;
	SF_INFO sndinfo;

	/* Not null */
	if (sndfile) {
		fprintf(stderr, "audio_file_open_input() Warning file is already open.\n");
		return;
	}


	/* Zero the file info */
	memset( &sndinfo, 0, sizeof( sndinfo ) );

	/* Open audio file */
	sndfile = sf_open(filename, SFM_READ, &sndinfo);
	if (sndfile == NULL) {
		fprintf(stderr, "Failed to open file %s.\n", filename);
		fprintf(stderr, "%s\n", sf_strerror(sndfile));
		exit(3);
	}
	else audio->ptr = sndfile;


	/* Check the sample rate and choose payload */
	if (audio->format == 0 || audio->format == 8) {
		if (sndinfo.channels != 1 || sndinfo.samplerate != 8000) {
			fprintf(stderr, "Error: input file is not 8kHz mono.\n");
			exit(3);
		}
	} else if (audio->format == 10) {
		if (sndinfo.channels != 2 || sndinfo.samplerate != 44100) {
			fprintf(stderr, "Error: input file is not 44.1kHz stereo.\n");
			exit(3);
		}
	} else if (audio->format == 11) {
		if (sndinfo.channels != 1 || sndinfo.samplerate != 44100) {
			fprintf(stderr, "Error: input file is not 44.1kHz mono.\n");
			exit(3);
		}
	}
	
}


void
audio_file_open_output( audio_t *audio, char* filename )
{
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;
	SF_INFO sndinfo;

	// Set the file info
	memset( &sndinfo, 0, sizeof( sndinfo ) );
	sndinfo.samplerate = audio->samplerate;
	sndinfo.channels = audio->channels;


	// *** Does libsndfile have a nicer way of doing this ? ***
	if (strstr(filename, ".wav")) {
		// Use WAVE if it contains .wav
		sndinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16) ; 
		fprintf(stderr, "Writing stream to WAVE file: %s\n", filename);
	} else {
		// Otherwise use aiff
		sndinfo.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM_16) ; 
		fprintf(stderr, "Writing stream to AIFF file: %s\n", filename);
	}

	
	// Open the file and check for errors
	sndfile = sf_open(filename, SFM_WRITE, &sndinfo);
	if (sndfile == NULL) {
		fprintf(stderr, "Failed to open file '%s' for writing.\n", filename);
		fprintf(stderr, "%s\n", sf_strerror(sndfile));
		exit(3);
	}
	else audio->ptr = sndfile;
	
}


void audio_file_write( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;
	
	sf_write_short( sndfile, audio->buffer, samples );
		
}



/* read X bytes from the file */
int audio_file_read( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;
	int read_frames;
	
	/* Read in 'frames' frames from file */
	read_frames = sf_read_short( sndfile, audio->buffer, samples);

	return read_frames;
}



/* Rewind the file back to the begining */
void audio_file_rewind( audio_t* audio )
{
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;

	sf_seek( sndfile, 0, SEEK_SET );
}


void audio_file_close( audio_t* audio )
{
	SNDFILE* sndfile = (SNDFILE*)audio->ptr;

	if ( sndfile ) {
		sf_close( sndfile );
		audio->ptr = NULL;	
	}
}


#else

	void audio_file_open_input( audio_t *audio, char* filename )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); }
	
	void audio_file_open_output( audio_t *audio, char* filename )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); }
	
	void audio_file_write( audio_t* audio, int bytes )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); }
	
	int  audio_file_read( audio_t* audio, int bytes )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); return 0; }
	
	void audio_file_rewind( audio_t* audio )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); }

	
	void audio_file_close( audio_t* audio )
	{ fprintf(stderr, "libsndfile isn't available.\n"); exit(1); }

#endif
