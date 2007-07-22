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
	char* foundsemi=NULL;
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
		if (type->minor[i] == ';') foundsemi = &type->minor[i];
		type->minor[i] = 0x00;
		break;
	}
	
	// Found a semi-colon at the end of the sub type?
	if (foundsemi) {
		char* ptr = foundsemi+1;
		
		// Loop through the following name=value pairs
		while( ptr && strlen( ptr ) ) {
			char* name = ptr;
			char* value = NULL;
			char* found = NULL;
			
			// Find the end of name
			found=NULL;
			for(i=0; i<strlen(name); i++) {
				if (istoken(name[i])) continue;
				if (name[i] == '=') found = &name[i];
				else MAST_WARNING("Was expecting '=' but found '%c'", name[i]);
				name[i] = 0x00;
				break;
			}
			
			// Found equals sign?
			if (found) value = found+1;
			else break;
		
			// Now look for the end of value
			found = NULL;
			for(i=0; i<strlen(value); i++) {
				if (istoken(value[i])) continue;
				if (value[i] == ';') found = &value[i];
				else MAST_WARNING("Was expecting ';' but found '%c'", value[i]);
				value[i] = 0x00;
				break;
			}

			// Store it
			mast_mime_type_set_param( type, name, value );

			// Another pair after this one?			
			if (found) ptr = found+1;
			else break;
		}

	
	}

	// Success
	return 0;
}



void mast_mime_type_set_param( mast_mime_type_t *type, char* name, char* value )
{
	int existing = -1;
	int available = -1;
	int i;
	
	// Loop through the existing parameters, finding a place
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (type->param[i].name==NULL) {
			if (available==-1) available = i;
		} else if (strcmp(type->param[i].name, name)==0) {
			existing = i;
			break;
		}
	}
	
	// Parameter name already stored?
	if (existing != -1) {
		// Re-use existing place in array
		type->param[existing].value = value;
	} else if (available != -1) {
		// Use first available slot
		type->param[available].name = name;
		type->param[available].value = value;
	} else {
		MAST_FATAL( "Failed to find space in array to store parameter %s=%s", name, value );
	}

}


void mast_mime_type_print( mast_mime_type_t *type )
{
	int i;
	MAST_INFO("Mime Type: %s/%s", type->major, type->minor);
	
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (type->param[i].name) {
			MAST_INFO("Mime Parameter: %s=%s", type->param[i].name, type->param[i].value);
		}
	}
}

void mast_mime_type_param_apply_codec( mast_mime_type_t *type, mast_codec_t *codec )
{
	int i;

	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (type->param[i].name) {
			int err = mast_codec_set_param( codec, type->param[i].name, type->param[i].value );
			if (err==-1) {
				MAST_FATAL("Parameter '%s' is not supported by codec", type->param[i].name);
			} else if (err==-2) {
				MAST_FATAL("Value '%s' is not supported by paramter", type->param[i].value);
			}
		}
	}

}

void mast_mime_type_deinit( mast_mime_type_t *type )
{
	if (type) {
	
		if (type->str) free( type->str );
		free( type );
	}
	
}


