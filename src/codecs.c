/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
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


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "codecs.h"
#include "mast.h"


// Table of codecs
static struct {
	char* mime_subtype;
	mast_codec_t* (*init)();
} mast_codecs[] = {
//	{ "DVI4",	mast_init_dvi4 },
	{ "GSM",	mast_init_gsm },
	{ "L16",	mast_init_l16 },
	{ "LPC",	mast_init_lpc },
	{ "PCMA",	mast_init_pcma },
	{ "PCMU",	mast_init_pcmu },
	{ NULL, 	NULL }
};



// Find a codec and initialise it
mast_codec_t* mast_init_codec( char* mime_subtype )
{
	int i;
	
	for(i=0; mast_codecs[i].mime_subtype; i++) {
		if (strcasecmp( mime_subtype, mast_codecs[i].mime_subtype ) == 0) {
			// Found a matching codec
			return mast_codecs[i].init();
		}
	}
	
	// Didn't find match
	MAST_WARNING( "Failed to find codec for MIME type audio/%s", mime_subtype );
	return NULL;
}



