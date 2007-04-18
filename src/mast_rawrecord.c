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
#include "mpa_header.h"


#define PROGRAM_NAME "mast_rawrecord"


/* Global Variables */
int running = TRUE;					// true while the programme is running
char *arg_filename = NULL;			// filename of output file
char *arg_address = NULL;			// ip address / multicast group
char *arg_port = DEFAULT_RTP_PORT;	// port number




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



void parse_args(int argc, char **argv)
{
	int i;
	
	// Check the number of arguments
	if (argc < 2 || argc > 3) usage();


	// Extract the address
	arg_address = argv[1];
	for(i=0; i<strlen(arg_address); i++) {
		if (arg_address[i]=='/') {
			arg_address[i] = '\0';
			arg_port = &arg_address[i+1];
			break;
		}
	}
	
	// Extra argument?
	if (argc==3) {
		// Output filename
		arg_filename = argv[2];
	}

}



void signal_handler(int signum)
{
	switch( signum) {
		case SIGINT: fprintf(stderr, "Caught signal: SIGINT\n" ); break;
		case SIGTERM: fprintf(stderr, "Caught signal: SIGTERM\n" ); break;
		default: fprintf(stderr, "Caught signal: %d\n", signum ); break;
	}	
	
	running=0;
}



char* check_payload( mblk_t *hdr, mblk_t *body, int *payload_len, int *ts_delta )
{
	int payload_type = rtp_get_payload_type(hdr);
	

	switch( payload_type ) {
	
		case RTP_PAYLOAD_TYPE_MPA: {
			mpa_header_t mh;
			u_int32_t frag_offset = ((u_int32_t)body->b_rptr[1] << 8) | ((u_int32_t)body->b_rptr[0]);
			char *payload = body->b_rptr+4+frag_offset;
			if (frag_offset != 0) {
				fprintf( stderr, "Warning: frag_offset != 0\n" );
			}

			*payload_len = (body->b_wptr - body->b_rptr) - 4 - frag_offset;
			
			if (mpa_header_parse( (unsigned char *)payload, &mh)) {
				int frame_count = *payload_len / mh.framesize;
				*ts_delta = ((mh.samples * 90000) / mh.samplerate) * frame_count;
				
				return payload;

			} else {
				fprintf( stderr, "Failed to parse MPEG audio header\n" );
			}
			
			break;
		}
		
		default:
			fprintf(stderr, "Unsupported payload type: %d\n", payload_type );
		break;
		
	}
	
	return NULL;

}



void config_rtcp( RtpSession *session ) {
	char cname[ STR_BUF_SIZE ];
	char tool[ STR_BUF_SIZE ];
	char *hostname=NULL;


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

}


int main(int argc, char **argv)
{
	RtpSession *session=NULL;
	FILE* output_file = NULL;
	int ts=0;
	
	
	// Parse commandline parameters
	parse_args( argc, argv );
	
	fprintf(stderr, "Source Address: %s\n", arg_address);
	fprintf(stderr, "Source Port: %s\n", arg_port);
	
	// Open output file ?
	if (arg_filename) {
		fprintf(stderr, "Output file: %s\n", arg_filename);
		output_file = fopen( arg_filename, "wb" );
		if (output_file==NULL) {
			perror("failed to open output file");
			exit(-5);
		}
	} else {
		fprintf(stderr, "Output file: STDOUT\n");
		output_file = stdout;
	}
	
	
	// Initialise ortp
	ortp_set_log_level_mask( ORTP_WARNING|ORTP_ERROR|ORTP_FATAL );
	ortp_init();
	
	/* Add MPEG Audio payload type 14 in the AV profile.*/
	rtp_profile_set_payload(&av_profile, RTP_PAYLOAD_TYPE_MPA, &payload_type_mpeg_audio);

	
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);


	// Create new session
	session=rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_local_addr(session, arg_address, atoi(arg_port));

	
	// Set RTCP parameters
	config_rtcp( session );
	
	
	
	while(running)
	{
	
		// Fetch a packet
		mblk_t *pkt = rtp_session_recvm_with_ts(session, ts );
		if (pkt!=NULL) {
			char *payload = NULL;
			int payload_len = 0;
			int ts_delta = 0;

			payload = check_payload( pkt, pkt->b_cont, &payload_len, &ts_delta );
			if (payload) {
				if (fwrite( payload, payload_len, 1, output_file ) < 1) {
					perror("failed to write to output file");
					exit(-5);
				}
				ts += ts_delta;
			}
		}

	}


	rtp_session_destroy(session);
	ortp_exit();

	 
	// Success !
	return 0;
}


