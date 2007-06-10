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

	mh->layer = 4 - ((header >> 17) & 0x03);
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
void mpa_header_print( FILE* out, mpa_header_t *mh )
{
	fprintf(out, "MPEG Version      : ");
	if (mh->version==1)			fprintf(out, "MPEG-1");
	else if (mh->version==2)	fprintf(out, "MPEG-2");
	else if (mh->version==3)	fprintf(out, "MPEG-2.5");
	else 						fprintf(out, "unknown");
	fprintf(out, " (Layer %d)\n", mh->layer);
	
	fprintf(out, "Mode              : ");
	if (mh->mode==MPA_MODE_STEREO)		fprintf(out, "Stereo\n");
	else if (mh->mode==MPA_MODE_JOINT)	fprintf(out, "Joint Stereo\n");
	else if (mh->mode==MPA_MODE_DUAL)	fprintf(out, "Dual\n");
	else if (mh->mode==MPA_MODE_MONO)	fprintf(out, "Mono\n");
	else 								fprintf(out, "unknown\n");

	fprintf(out, "Bitrate           : %d kbps\n", mh->bitrate);
	fprintf(out, "Sample Rate       : %d Hz\n", mh->samplerate);

	fprintf(out, "Error Protection  : %s\n", mh->error_protection ? "Yes" : "No" );
	fprintf(out, "Padding           : %s\n", mh->padding ? "Yes" : "No" );
	fprintf(out, "Extension Bit     : %s\n", mh->extension ? "Set" : "Not Set" );
	//fprintf(out, "mode_ext          : %d\n", mh->mode_ext);
	fprintf(out, "Copyright         : %s\n", mh->copyright ? "Yes" : "No" );
	fprintf(out, "Original          : %s\n", mh->original ? "Yes" : "No" );
	//fprintf(out, "Channels          : %d\n", mh->channels);
	fprintf(out, "Samples per frame : %d\n", mh->samples);
	fprintf(out, "Bytes per frame   : %d\n", mh->framesize);
}


// Parse mpeg audio header
// returns 1 if valid, or 0 if invalid
int mpa_header_parse( const u_int8_t* buff, mpa_header_t *mh)
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


