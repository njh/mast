/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003 University of Southampton
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
#include <string.h>
#include <sys/types.h>


#include <netinet/in.h>

#include "codecs.h"


static int Initialised = 0;           /* Initialising counter */

/*
static const char   Signature[5]  = "alaw"; // Standard Codec Signature
static const char	*Name = "A-Law 2:1"; 
static const char   *Version  = "0.1"; 
static const char   *Copyright  = "2002 OpenQuicktime Team"; 
static const char   *ModuleAuthor  = "Nicholas Humfrey"; 
static const char   *CodecAuthor  = "Erik de Castro Lopo"; 
*/



int
init_l16()
{
	return ++Initialised;
}

int
delete_l16()
{
	return --Initialised;
}



u_int32_t
decode_l16(
	       u_int32_t inputsize,		/* input size in bytes */
	       u_int8_t  *input,
	       u_int32_t outputsize, 	/* output size in samples */
	       int16_t  *output)
{
	register int i;
	register int num_samples = (inputsize/2);
	int16_t *input16 = (int16_t*)input;

	if (outputsize < inputsize) {
		fprintf(stderr, "decode_l16: output buffer isn't big enough.\n");
		return 0;
	}


	for(i=0; i< num_samples; i++) {
		output[i] = ntohs(input16[i]);
	}

	return num_samples;
}



u_int32_t
encode_l16(
	       u_int32_t inputsize, 	/* input size in samples */
	       int16_t *input,
	       u_int32_t outputsize,	/* output size in bytes */
	       u_int8_t *output)
{
	register int i;
	register int input_bytes = (inputsize*2);
	int16_t *output16 = (int16_t*)output;

	if (outputsize < input_bytes) {
		fprintf(stderr, "encode_l16: output buffer isn't big enough.\n");
		return 0;
	}


	for(i=0; i< inputsize; i++) {
		output16[i] = htons(input[i]);
	}

	return input_bytes;
}


