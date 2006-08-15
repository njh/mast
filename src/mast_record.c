/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2006 University of Southampton
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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <ortp/ortp.h>

#include "config.h"
#include "mast.h"


#define PROGRAM_NAME "mast_record"


int running = 1;


// RTP Payload Type for MPEG Audio
PayloadType	payload_type_mpeg_audio={
	PAYLOAD_AUDIO_PACKETIZED, // type
	90000,	// clock rate
	0,		// bytes per sample N/A
	NULL,	// zero pattern N/A
	0,		// pattern_length N/A
	0,		// normal_bitrate
	"mpa",	// MIME Type
	0		// flags
};



int usage() {
	
	fprintf(stderr, "%s package version %s.\n\n", PACKAGE, VERSION);
	fprintf(stderr, "%s <addr>/<port> [<filename>]\n", PROGRAM_NAME);
	
	exit(1);
	
}



int main(int argc, char **argv)
{
	RtpSession *session;
	char cname[ STR_BUF_SIZE ];
	char tool[ STR_BUF_SIZE ];
	char *hostname;
	
	// Initialise ortp
	ortp_set_log_level_mask( ORTP_WARNING|ORTP_ERROR|ORTP_FATAL );
	ortp_init();
	
	/* Add MPEG Audio payload type 14 in the AV profile.*/
	rtp_profile_set_payload(&av_profile, 14, &payload_type_mpeg_audio);

	

	// Create new session
	session=rtp_session_new(RTP_SESSION_RECVONLY);
	//rtp_session_set_remote_addr(session, chan->multicast_ip, chan->multicast_port);

	
	// Set RTCP parameters
	hostname = gethostname_fqdn();
	snprintf( cname, STR_BUF_SIZE, "%s@%s", PACKAGE_NAME, hostname );
	snprintf( tool, STR_BUF_SIZE, "%s (%s %s)", PROGRAM_NAME, PACKAGE_NAME, PACKAGE_VERSION );
	free( hostname );
	
	rtp_session_set_source_description(
		session,			// RtpSession*
		cname,				// CNAME
		NULL,				// name
		NULL,				// email
		NULL,				// phone
		NULL,				// loc
		tool,				// tool
		NULL				// note
	);
	
	
	while(running)
	{
		have_more=1;
		while (have_more){
			err=rtp_session_recv_with_ts(session,buffer,160,ts,&have_more);
			if (err>0) stream_received=1;
			/* this is to avoid to write to disk some silence before the first RTP packet is returned*/	
			if ((stream_received) && (err>0)) {
				size_t ret = fwrite(buffer,1,err,outfile);
				if (sound_fd>0)
					ret = write(sound_fd,buffer,err);
			}
		}
		ts+=160;
		//ortp_message("Receiving packet.");
	}


	rtp_session_destroy(session);
	ortp_exit();
	
	 
	// Success !
	return 0;
}


