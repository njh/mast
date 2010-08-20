/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
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
 *  $Id$
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
#include <string.h>
#include <stdlib.h>

#include "MPA_Header.h"


#define MPA_MODE_STEREO		(0)
#define MPA_MODE_JOINT		(1)
#define MPA_MODE_DUAL		(2)
#define MPA_MODE_MONO		(3)

#define MPA_SYNC_WORC		(0x0ffe)


static const unsigned int bitrate_table[3][3][16] =
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

static const unsigned int samplerate_table[3][4] =
{
	{ 44100, 48000, 32000, 0 }, // MPEG 1
	{ 22050, 24000, 16000, 0 }, // MPEG 2
	{ 11025, 12000,  8000, 0 }  // MPEG 2.5
};



u_int32_t MPA_Header::get_channels()
{
	if (this->mode == MPA_MODE_MONO)
			return 1;
	else	return 2;
}

u_int32_t MPA_Header::get_bitrate()
{
	if (this->layer && this->version) {
		return bitrate_table[this->version-1][this->layer-1][this->bitrate_index];
	} else {
		return 0;
	}
}

u_int32_t MPA_Header::get_samplerate()
{
	if (this->version) {
		return samplerate_table[this->version-1][this->samplerate_index];
	} else {
		return 0;
	}
}

u_int32_t MPA_Header::get_samples()
{
	if (this->version == 1) {
		return 1152;
	} else {
		return 576;
	}
}

u_int32_t MPA_Header::get_framesize()
{
	u_int32_t samplerate = get_samplerate();
	
	if (samplerate) {
		return (this->get_samples() * this->get_bitrate() * 1000 / samplerate) / 8 + this->padding;
	} else {
		return 0;
	}
}


// Print out all field values for debugging
void MPA_Header::debug( FILE* out )
{
	fprintf(out, "MPEG Version      : ");
	if (get_version()==1)			fprintf(out, "MPEG-1");
	else if (get_version()==2)	fprintf(out, "MPEG-2");
	else if (get_version()==3)	fprintf(out, "MPEG-2.5");
	else 						fprintf(out, "unknown");
	fprintf(out, " (Layer %d)\n", get_layer());
	
	fprintf(out, "Mode              : ");
	if (get_mode()==MPA_MODE_STEREO)		fprintf(out, "Stereo\n");
	else if (get_mode()==MPA_MODE_JOINT)	fprintf(out, "Joint Stereo\n");
	else if (get_mode()==MPA_MODE_DUAL)	fprintf(out, "Dual\n");
	else if (get_mode()==MPA_MODE_MONO)	fprintf(out, "Mono\n");
	else 								fprintf(out, "unknown\n");

	fprintf(out, "Bitrate           : %d kbps\n", get_bitrate());
	fprintf(out, "Sample Rate       : %d Hz\n", get_samplerate());

	fprintf(out, "Error Protection  : %s\n", get_error_protection() ? "Yes" : "No" );
	fprintf(out, "Padding           : %s\n", get_padding() ? "Yes" : "No" );
	fprintf(out, "Extension Bit     : %s\n", get_extension() ? "Yes" : "No" );
	//fprintf(out, "mode_ext          : %d\n", get_mode_ext());
	fprintf(out, "Copyright         : %s\n", get_copyright() ? "Yes" : "No" );
	fprintf(out, "Original          : %s\n", get_original() ? "Yes" : "No" );
	//fprintf(out, "Channels          : %d\n", get_channels());
	fprintf(out, "Samples per frame : %d\n", get_samples());
	fprintf(out, "Bytes per frame   : %d\n", get_framesize());
}


// concise informational string
void MPA_Header::print( FILE* out )
{

	if (get_version()==1)			fprintf(out, "MPEG-1 ");
	else if (get_version()==2)	fprintf(out, "MPEG-2 ");
	else if (get_version()==3)	fprintf(out, "MPEG-2.5 ");
	else 						fprintf(out, "MPEG-?? ");

	fprintf(out, "layer %d, ", get_layer());
	fprintf(out, "%d kbps, ", get_bitrate());
	fprintf(out, "%d Hz, ", get_samplerate());
	
	if (this->mode==MPA_MODE_STEREO)		fprintf(out, "Stereo");
	else if (this->mode==MPA_MODE_JOINT)	fprintf(out, "Joint Stereo");
	else if (this->mode==MPA_MODE_DUAL)	fprintf(out, "Dual");
	else if (this->mode==MPA_MODE_MONO)	fprintf(out, "Mono");

	fprintf(out, "\n");
}



// Parse mpeg audio header
// returns 1 if valid, or 0 if invalid
int MPA_Header::parse( const u_int8_t* buff )
{
	u_int32_t header =
	    ((u_int32_t)buff[0] << 24) | 
		((u_int32_t)buff[1] << 16) |
		((u_int32_t)buff[2] << 8)  |
		((u_int32_t)buff[3]);

	/* fill out the header struct */
	this->syncword = (header >> 20) & 0x0fff;

	this->version = 2 - ((header >> 19) & 0x01);
	if ((this->syncword & 0x01) == 0)
		this->version = 3;

	this->layer = 4 - ((header >> 17) & 0x03);
	if (this->layer==4)
		this->layer=0;
		
	this->error_protection = ((header >> 16) & 0x01) ? 0 : 1;
	this->bitrate_index = (header >> 12) & 0x0F;
	this->samplerate_index = (header >> 10) & 0x03;
	this->padding = (header >> 9) & 0x01;
	this->extension = (header >> 8) & 0x01;
	this->mode = (header >> 6) & 0x03;
	this->mode_ext = (header >> 4) & 0x03;
	this->copyright = (header >> 3) & 0x01;
	this->original = (header >> 2) & 0x01;
	this->emphasis = header & 0x03;


	/* check for syncword */
	if ((this->syncword & MPA_SYNC_WORC) != MPA_SYNC_WORC)
		return 0;

	/* make sure layer is sane */
	if (this->get_layer() == 0)
		return 0;

	/* make sure version is sane */
	if (this->get_version() == 0)
		return 0;

	/* make sure bitrate is sane */
	if (this->get_bitrate() == 0)
		return 0;

	/* make sure samplerate is sane */
	if (this->get_samplerate() == 0)
		return 0;

	/* Good; it is valid */
	return 1;
}


// Constructors
MPA_Header::MPA_Header()
{
	// Initialise variables
	syncword = 0;
	layer = 0;
	version = 0;
	error_protection = 0;
	bitrate_index = 0;
	samplerate_index = 0;
	padding = 0;
	extension = 0;
	mode = 0;
	mode_ext = 0;
	copyright = 0;
	original = 0;
	emphasis = 0;

}


MPA_Header::MPA_Header( const u_int8_t* buff )
{
	MPA_Header();
	this->parse( buff );
}


