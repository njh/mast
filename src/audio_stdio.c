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
#include "mast.h"



void audio_stdio_write( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)

	/* Write to stdout */
	int samples_written = fwrite( audio->buffer, 2, samples, stdout );
	if (samples_written == -1) {
		perror("audio write to stdout");
		exit(1);
	}
	
	if (samples_written != samples) {
		fprintf(stderr, "audio_stdio_write: Didn't write all the data I was supposed to.\n");
	}

}



int audio_stdio_read( audio_t* audio, int samples )
{
	// samples is the total number of samples (left+right)

	/* Read from stdin */
	int read_samples = fread( audio->buffer, 2, samples, stdin );
	if (read_samples == -1) {
		perror("audio read from stdin");
		exit(1);
	}

	return read_samples;
}


