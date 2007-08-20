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

#include "MastMimeType.h"
#include "mast.h"








MastMimeType::MastMimeType()
{
	int i;

	this->major = strdup(DEFAULT_PAYLOADTYPE_MAJOR);
	this->minor = strdup(DEFAULT_PAYLOADTYPE_MINOR);
	
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		this->param[i] = NULL;
	}

}


MastMimeType::MastMimeType( const char* string )
{
	// Initialise ourselves
	MastMimeType();
	
	// Now parse the string passed
	if (string) {
		this->parse( string );
	}
	
}



MastMimeType::~MastMimeType()
{
	int i;
	
	if (this->major) {
		free( this->major );
		this->major=NULL;
	}
	
	if (this->minor) {
		free( this->minor );
		this->minor=NULL;
	}

	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (this->param[i]) {
			delete this->param[i];
			this->param[i] = NULL;
		}
	}

}





int MastMimeType::istoken( char chr )
{
	if (isalnum(chr)) return TRUE;
	if (chr=='.') return TRUE;
	if (chr=='-') return TRUE;
	if (chr=='_') return TRUE;
	if (chr=='+') return TRUE;
	return FALSE;
}


int MastMimeType::parse( const char* string )
{
	char* foundslash=NULL;
	char* foundsemi=NULL;
	char* tmp=NULL;
	size_t i=0;
	
	
	// Duplicate the string so that we can play with it
	tmp = strdup( string );

	// Look for a slash from the start of the string	
	for(i=0; i<strlen(tmp); i++) {
		if (istoken(tmp[i])) continue;
		if (tmp[i]=='/') {
			tmp[i]=0x00;
			foundslash = &tmp[i];
		}
		break;
	}
	
	// If we found a slash, check the major type is 'audio'
	if (foundslash && strcmp( "audio", tmp )!=0) {
		MAST_FATAL("MIME Type is not audio/*");
	}
	
	// Copy the subtype from the position we think is the start
	if (foundslash) {
		this->minor = strdup(foundslash+1);
	} else {
		this->minor = strdup(tmp);
	}
	
	// Now find the end of the subtype
	for(i=0; i<strlen(this->minor); i++) {
		if (istoken(this->minor[i])) continue;
		if (this->minor[i] == ';') foundsemi = &this->minor[i];
		this->minor[i] = 0x00;
		break;
	}
	
	// Found a semi-colon at the end of the sub type?
	if (foundsemi) {
		char* ptr = foundsemi+1;
		
		// Loop through the following name=value pairs
		while( ptr && strlen( ptr ) ) {
			char* pair = ptr;
			char* end = NULL;
			
			// Find the end of the pair
			for(i=0; i<strlen(pair); i++) {
				if (!istoken(pair[i]) && pair[i]!='=') {
					pair[i] = 0x00;
					end = &pair[i];
					break;
				}
			}
			
			// Store it
			this->set_param( pair );

			// Another pair after this one?			
			if (end) ptr = end+1;
			else break;
		}

	
	}

	// Success
	return 0;
}


void MastMimeType::set_param( const char* pair )
{
	char* name = strdup(pair);
	char* value = NULL;
	size_t i;
	
	// Search for the equals sign
	for(i=0; i<strlen(name); i++) {
		if (name[i] == '=') {
			value = &name[i+1];
			name[i] = 0x00;
			break;
		}
	}
	
	// Set the parameter
	this->set_param( name, value );
	
	// Free up the temporary string
	free(name);
}


void MastMimeType::set_param( const char* name, const char* value )
{
	int existing = -1;
	int available = -1;
	int i;
	
	// Loop through the existing parameters, finding a place
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (this->param[i]==NULL) {
			if (available==-1) available = i;
		} else if (strcmp(this->param[i]->get_name(), name)==0) {
			existing = i;
			break;
		}
	}
	
	// Parameter name already stored?
	if (existing != -1) {
		// Re-use existing place in array
		this->param[existing]->set_value(value);
	} else if (available != -1) {
		// Use first available slot
		this->param[available] = new MastMimeTypeParam( name, value );
	} else {
		MAST_FATAL( "Failed to find space in array to store parameter %s=%s", name, value );
	}

}


void MastMimeType::print( )
{
	int i;
	MAST_INFO("Mime Type: %s/%s", this->major, this->minor);
	
	for(i=0; i<MAX_MIME_TYPE_PARAMS; i++) {
		if (this->param[i]) {
			MAST_INFO("Mime Parameter: %s=%s", this->param[i]->get_name(), this->param[i]->get_value());
		}
	}
}




MastMimeTypeParam::MastMimeTypeParam( const char* name, const char* value )
{
	this->set_name( name );
	this->set_value( value );
}

MastMimeTypeParam::MastMimeTypeParam( const char* string )
{
	// parse name=value

}

MastMimeTypeParam::~MastMimeTypeParam()
{
	if (this->name) free( this->name );
	if (this->value) free( this->value );
}

void MastMimeTypeParam::set_name( const char* name )
{
	if (this->name) free( this->name );
	this->name = strdup( name );
}

void MastMimeTypeParam::set_value( const char* value )
{
	if (this->value) free( this->value );
	this->value = strdup( value );
}

