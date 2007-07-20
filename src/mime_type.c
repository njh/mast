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

#include "mime_type.h"
#include "mast.h"


mast_mime_type_t * mast_mime_type_init()
{
	mast_mime_type_t *type = NULL;
	int i;
	
	// Allocate memory for the mast_mime_type structure
	type = malloc( sizeof(mast_mime_type_t) );
	if (type==NULL) {
		MAST_FATAL("Failed to allocate memory for mast_mime_type_t");
	}

	type->str = NULL;
	type->major = "audio";
	type->minor = NULL;
	
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		type->param[i].name = NULL;
		type->param[i].value = NULL;
	}

	return type;
}



static int istoken( char chr )
{
	if (isalnum(chr)) return TRUE;
	if (chr=='.') return TRUE;
	if (chr=='-') return TRUE;
	if (chr=='+') return TRUE;
	return FALSE;
}


int mast_mime_type_parse( mast_mime_type_t *type, const char* string )
{
	char* foundslash=NULL;
	int i=0;
	
	
	// Duplicate the string so that we can play with it
	type->str = strdup( string );

	// Look for a slash from the start of the string	
	for(i=0; i<strlen(type->str); i++) {
		if (istoken(type->str[i])) continue;
		if (type->str[i]=='/') {
			type->str[i]=0x00;
			foundslash = &type->str[i];
		}
		break;
	}
	
	// If we found a slash, check the major type is 'audio'
	if (foundslash && strcmp( "audio", type->str )!=0) {
		MAST_FATAL("MIME Type is not audio/*");
	}
	
	// Copy the subtype from the position we think is the start
	if (foundslash) {
		type->minor = foundslash+1;
	} else {
		type->minor = type->str;
	}
	
	// Now find the end of the subtype
	for(i=0; i<strlen(type->minor); i++) {
		if (istoken(type->minor[i])) continue;
		type->minor[i] = 0x00;
		break;
	}

	// Success
	return 0;
}



void mast_mime_type_set_param( mast_mime_type_t *type, char* name, char* value )
{

}


void mast_mime_type_print( mast_mime_type_t *type )
{
	MAST_INFO("Mime Type: %s/%s", type->major, type->minor);
}


void mast_mime_type_free( mast_mime_type_t *type )
{
	if (type) {
	
		if (type->str) free( type->str );
		free( type );
	}
	
}


