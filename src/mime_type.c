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
#include <ctype.h>

#include "config.h"
#include "codecs.h"
#include "mast.h"


void parse_mime_type( mime_type_t *type, const char* string )
{
	char* str = st
	char* foundslash=NULL;
	int i=0;

	// Look for a slash from the start of the string	
	for(i=0; i<strlen(string); i++) {
		if (istoken(string[i])) continue;
		if (string[i]=='/') foundslash = &string[i+1];
		break;
	}
	
	// If we found a slash, check the major type is 'audio'
	if (foundslash && strncmp( "audio/", string, 6 )!=0) {
		MAST_FATAL("MIME Type is not audio/*");
	}
	
	// Copy the subtype from the position we think is the start
	if (foundslash) {
		type.major = malloc( foundslash-string )
		type.minor = strdup( foundslash );
	} else {
		type.major = strdup( "audio" );
		type.minor = strdup( string );
	}
	
	// Now find the end of the subtype
	for(i=0; i<strlen(subtype); i++) {
		if (istoken(subtype[i])) continue;
		subtype[i] = 0;
		break;
	}


}


