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

#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <ortp/ortp.h>
#include <ortp/payloadtype.h>

#include "config.h"
#include "mast.h"



/* This table duplicates information provided by oRTP
   but oRTP doesn't know about stereo channels.... 
*/
static mast_payload_t static_payloadtypes[] =
{
	{ 0, "PCMU", 8000, 1 },
	{ 3, "GSM", 8000, 1 },
	{ 4, "G723", 8000, 1 },
	{ 5, "DVI4", 8000, 1 },
	{ 6, "DVI4", 16000, 1 },
	{ 7, "LPC", 8000, 1 },
	{ 8, "PCMA", 8000, 1 },
	{ 9, "G722", 8000, 1 },
	{ 10, "L16", 44100, 2 },
	{ 11, "L16", 44100, 1 },
	{ 12, "QCELP", 8000, 1 },
	{ 13, "CN", 8000, 1 },
	/* MPA goes here */
	{ 15, "G728", 8000, 1 },
	{ 16, "DVI4", 11025, 1 },
	{ 17, "DVI4", 22050, 1 },
	{ 17, "G729", 8000, 1 },
	{ 0, "", 0, 0 },
};
	


/*
	Search for a static payload number
*/
int mast_search_payload_num( mast_payload_t* pt )
{
	int i=0;

	// Search for a static payload type
	for(i=0; strlen(static_payloadtypes[i].subtype); i++) {
		if (strcasecmp( pt->subtype, static_payloadtypes[i].subtype) == 0 &&
		    pt->clockrate == static_payloadtypes[i].clockrate &&
		    pt->channels == static_payloadtypes[i].channels) {
			return static_payloadtypes[i].number;
		}
	}

	// Didn't find a static type - must be dynamic
	return 96;
}



/* 
	Build a full MIME type based user's choice and filling in the blanks 
*/
mast_payload_t* mast_make_payloadtype( const char* chosen, int in_rate, int in_chan )
{
	mast_payload_t* pt = NULL;
	char* tmpstr = strdup( chosen );
	char* subtype = NULL;
	char* rate_str = NULL;
	char* chan_str = NULL;
	
	// Format is: mimetype/subtype/clockrate/channels

	// Locate the start of the subtype
	if (strncmp("audio/", tmpstr, 6) == 0) {
		subtype = tmpstr+6;
	} else {
		subtype = tmpstr;
	}
	
	// Parse the rest of the string
	rate_str = strchr(subtype, '/');
	if (rate_str && strlen(rate_str)>1) {
		*rate_str = 0;
		rate_str++;
		
		// Look for another slash
		chan_str = strchr(rate_str, '/');
		if (chan_str && strlen(chan_str)>1) {
			*chan_str = 0;
			chan_str++;
		} else {
			chan_str = NULL;
		}
	} else {
		rate_str = NULL;
	}

	
	// Malloc the datastructure
	pt = (mast_payload_t*)malloc( sizeof(mast_payload_t) );
	if (pt==NULL) MAST_FATAL("Failed to allocate memory for mast_payload_t");
	strcpy( pt->subtype, subtype );

	// Use the user specified clockrate if given	
	if (rate_str) {
		pt->clockrate = atoi(rate_str);
	} else {
		pt->clockrate = in_rate;
	}
	
	// Use the user specified number of channels if given	
	if (chan_str) {
		pt->channels = atoi(chan_str);
	} else {
		pt->channels = in_chan;
	}
	
	// Is is a static payload available?
	pt->number = mast_search_payload_num( pt );

	free( tmpstr );

	return pt;
}


char* mast_mime_string( mast_payload_t* pt ) {
	char* outstr = NULL;

	// Create the final output string
	outstr = malloc( STR_BUF_SIZE+1 );
	snprintf( outstr, STR_BUF_SIZE, "audio/%s/%d/%d", pt->subtype, pt->clockrate, pt->channels );

	return outstr;
}



