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


#include <ortp/payloadtype.h>


static char null_bytes[] = {0x00, 0x00, 0x00, 0x00};


PayloadType payload_type_l16_mono=
{
	PAYLOAD_AUDIO_CONTINUOUS, // type
	44100,		// clock rate
	16,			// bits per frame
	null_bytes,	// zero pattern
	2,			// pattern_length
	705600,		// normal_bitrate (44100 x 16bits per frame x 1 channel)
	"L16",		// MIME Type
};


PayloadType payload_type_l16_stereo=
{
	PAYLOAD_AUDIO_CONTINUOUS, // type
	44100,		// clock rate
	32,			// bits per frame
	null_bytes,	// zero pattern
	4,			// pattern_length
	1411200,	// normal_bitrate (44100 x 16bits per frame x 2 channels)
	"L16",		// MIME Type
};



void mast_register_extra_payloads() {
	
	rtp_profile_set_payload(&av_profile, 11, &payload_type_l16_mono);
	rtp_profile_set_payload(&av_profile, 10, &payload_type_l16_stereo);

}