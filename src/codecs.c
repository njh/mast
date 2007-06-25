/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2007 University of Southampton
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



