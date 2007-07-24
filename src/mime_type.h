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

#include "codecs.h"

#ifndef	_MAST_MIME_TYPE_H_
#define	_MAST_MIME_TYPE_H_


// Maximum number of mime type parameters
#define	MAX_MIME_TYPE_PARAMS	(16)


typedef struct mast_mime_type_param {
	char* name;
	char* value;
} mast_mime_type_param_t;


typedef struct mast_mime_type {

	// Pointer to malloced memory containing data
	char* str;

	// Format: major/minor;name1=value1;name2=value2
	char* major;
	char* minor;
	
	// Array of parameters
	mast_mime_type_param_t param[ MAX_MIME_TYPE_PARAMS ];
	
} mast_mime_type_t;



// Prototypes of functions in mime_type.c
mast_mime_type_t * mast_mime_type_init();
int mast_mime_type_parse( mast_mime_type_t *type, const char* string );
void mast_mime_type_set_param( mast_mime_type_t *type, char* name, char* value );
void mast_mime_type_print( mast_mime_type_t *type );
void mast_mime_type_param_apply_codec( mast_mime_type_t *type, mast_codec_t *codec );
void mast_mime_type_deinit( mast_mime_type_t *type );



#endif	//_MAST_MIME_TYPE_H_
