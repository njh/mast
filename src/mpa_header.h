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



typedef struct {
	unsigned int syncword;
	unsigned int layer;
	unsigned int version;
	unsigned int error_protection;
	unsigned int bitrate_index;
	unsigned int samplerate_index;
	unsigned int padding;
	unsigned int extension;
	unsigned int mode;
	unsigned int mode_ext;
	unsigned int copyright;
	unsigned int original;
	unsigned int emphasis;
	unsigned int channels;
	unsigned int bitrate;
	unsigned int samplerate;
	unsigned int samples;
	unsigned int framesize;
} mpa_header_t;



// Get parse the header of a frame of mpeg audio
int mpa_header_parse( const u_int8_t* buff, mpa_header_t *mh);

void mpa_header_print( FILE* out, mpa_header_t *mh );
