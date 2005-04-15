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


/*
    MPEG Audio frame handling courtesy of Scott Manley
    Borrowed from libshout 2.1 / mp3.c 
    With a few changes and fixes
 */
 
 
#include "config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "mpa_header.h"


#define MPA_MODE_STEREO		0
#define MPA_MODE_JOINT		1
#define MPA_MODE_DUAL		2
#define MPA_MODE_MONO		3


static const unsigned int bitrate[3][3][16] =
{
	{ // MPEG 1
		{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}, // Layer 1
		{0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0}, // Layer 2
		{0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0}  // Layer 3
	},
	{ // MPEG 2
		{0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0}, // Layer 1
		{0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}, // Layer 2
		{0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}  // Layer 3
	},
	{ // MPEG 2.5
		{0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0}, // Layer 1
		{0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}, // Layer 2
		{0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0}  // Layer 3
	}
};

static const unsigned int samplerate[3][4] =
{
	{ 44100, 48000, 32000, 0 }, // MPEG 1
	{ 22050, 24000, 16000, 0 }, // MPEG 2
	{ 11025, 12000,  8000, 0 }  // MPEG 2.5
};


static void parse_header(mpa_header_t *mh, u_int32_t header)
{
	mh->syncword = (header >> 20) & 0x0fff;

	mh->version = 2 - ((header >> 19) & 0x01);
	if ((mh->syncword & 0x01) == 0)
		mh->version = 3;

	mh->layer = 4-((header >> 17) & 0x03);
	if (mh->layer==4)
		mh->layer=0;
		
	mh->error_protection = ((header >> 16) & 0x01) ? 0 : 1;
	mh->bitrate_index = (header >> 12) & 0x0F;
	mh->samplerate_index = (header >> 10) & 0x03;
	mh->padding = (header >> 9) & 0x01;
	mh->extension = (header >> 8) & 0x01;
	mh->mode = (header >> 6) & 0x03;
	mh->mode_ext = (header >> 4) & 0x03;
	mh->copyright = (header >> 3) & 0x01;
	mh->original = (header >> 2) & 0x01;
	mh->emphasis = header & 0x03;

	if (mh->layer && mh->version) {
		mh->bitrate = bitrate[mh->version-1][mh->layer-1][mh->bitrate_index];
		mh->samplerate = samplerate[mh->version-1][mh->samplerate_index];
	} else {
		mh->bitrate = 0;
		mh->samplerate = 0;
	}

	if (mh->mode == MPA_MODE_MONO)
		mh->channels = 1;
	else
		mh->channels = 2;

	if (mh->version == 1)
		mh->samples = 1152;
	else
		mh->samples = 576;

	if(mh->samplerate)
		mh->framesize = (mh->samples * mh->bitrate * 1000 / mh->samplerate) / 8 + mh->padding;
}

// For debugging
void mpa_header_print( mpa_header_t *mh )
{
	if (mh->version==1)			fprintf(stderr, "version=MPEG-1\n");
	else if (mh->version==2)	fprintf(stderr, "version=MPEG-2\n");
	else if (mh->version==3)	fprintf(stderr, "version=MPEG-2.5\n");
	else 						fprintf(stderr, "version=unknown\n");

	fprintf(stderr, "layer=%d\n", mh->layer);
	
	if (mh->mode==MPA_MODE_STEREO)		fprintf(stderr, "mode=Stereo\n");
	else if (mh->mode==MPA_MODE_JOINT)	fprintf(stderr, "mode=Joint Stereo\n");
	else if (mh->mode==MPA_MODE_DUAL)	fprintf(stderr, "mode=Dual\n");
	else if (mh->mode==MPA_MODE_MONO)	fprintf(stderr, "mode=Mono\n");
	else 								fprintf(stderr, "mode=unknown\n");

	fprintf(stderr, "error_protection=%d\n", mh->error_protection);
	fprintf(stderr, "padding=%d\n", mh->padding);
	fprintf(stderr, "extension=%d\n", mh->extension);
	fprintf(stderr, "mode_ext=%d\n", mh->mode_ext);
	fprintf(stderr, "copyright=%d\n", mh->copyright);
	fprintf(stderr, "original=%d\n", mh->original);
	fprintf(stderr, "channels=%d\n", mh->channels);
	fprintf(stderr, "bitrate=%d\n", mh->bitrate);
	fprintf(stderr, "samplerate=%d\n", mh->samplerate);
	fprintf(stderr, "samples=%d\n", mh->samples);
	fprintf(stderr, "framesize=%d\n", mh->framesize);
}


// Parse mpeg audio header
// returns 1 if valid, or 0 if invalid
int mpa_header_parse( const unsigned char* buff, mpa_header_t *mh)
{
	u_int32_t head =
	    ((u_int32_t)buff[0] << 24) | 
		((u_int32_t)buff[1] << 16) |
		((u_int32_t)buff[2] << 8)  |
		((u_int32_t)buff[3]);

	/* fill out the header struct */
	parse_header(mh, head);

	/* check for syncword */
	if ((mh->syncword & 0x0ffe) != 0x0ffe)
		return 0;

	/* make sure layer is sane */
	if (mh->layer == 0)
		return 0;

	/* make sure version is sane */
	if (mh->version == 0)
		return 0;

	/* make sure bitrate is sane */
	if (mh->bitrate == 0)
		return 0;

	/* make sure samplerate is sane */
	if (mh->samplerate == 0)
		return 0;

	return 1;
}


