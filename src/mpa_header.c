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
 */
 
 
#include "config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "mpa_header.h"




#define MPA_VERSION_1		3
#define MPA_VERSION_2		2
#define MPA_VERSION_25		0

#define MPA_LAYER_I			3
#define MPA_LAYER_II		2
#define MPA_LAYER_III		1

#define MPA_MODE_STEREO		0
#define MPA_MODE_JOINT		1
#define MPA_MODE_DUAL		2
#define MPA_MODE_MONO		3


static const unsigned int bitrate[3][3][16] =
{
	{
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 }
	}, {
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }
	}, {
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }
	}
};

static const unsigned int samplerate[3][4] =
{
	{ 44100, 48000, 32000, 0 },
	{ 22050, 24000, 16000, 0 },
	{ 11025, 8000, 8000, 0 }
};


static void parse_header(mpa_header_t *mh, u_int32_t header)
{
	mh->syncword = (header >> 20) & 0x0fff;
	mh->version = ((header >> 19) & 0x01) ? 0 : 1;
	if ((mh->syncword & 0x01) == 0)
		mh->version = 2;
	mh->layer = 3 - ((header >> 17) & 0x03);
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

	mh->stereo = (mh->mode == MPA_MODE_MONO) ? 1 : 2;
	mh->bitrate = bitrate[mh->version][mh->layer][mh->bitrate_index];
	mh->samplerate = samplerate[mh->version][mh->samplerate_index];

	if (mh->version == 0)
		mh->samples = 1152;
	else
		mh->samples = 576;

	if(mh->samplerate)
		mh->framesize = (mh->samples * mh->bitrate * 1000 / mh->samplerate) / 8 + mh->padding;
}



int mpa_header(char* buff, mpa_header_t *mh)
{
	u_int32_t head =
	    (buff[0] << 24) | 
		(buff[1] << 16) |
		(buff[2] << 8)  |
		(buff[3] << 0);

	/* fill out the header struct */
	parse_header(mh, head);

	/* check for syncword */
	if ((mh->syncword & 0x0ffe) != 0x0ffe)
		return 0;

	/* make sure bitrate is sane */
	if (mh->bitrate == 0)
		return 0;

	/* make sure samplerate is sane */
	if (mh->samplerate == 0)
		return 0;

	return 1;
}


