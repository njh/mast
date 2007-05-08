/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2005 University of Southampton
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


#define PROGRAM_NAME "mast_mpacast"


/* Global Variables */
char* g_addr = NULL;
int g_port = DEFAULT_RTP_PORT;
int g_ssrc = DEFAULT_RTP_SSRC;
int g_ttl = DEFAULT_MULTICAST_TTL;
int g_mtu = DEFAULT_MAX_PAYLOAD_SIZE;
int g_loop = 0;
int g_running = TRUE;


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
	
	fprintf(stderr, "%s version %s.\n\n", PACKAGE_NAME, VERSION);
	fprintf(stderr, "%s [options] <addr>[/<port>] [file1 file2...]\n", PROGRAM_NAME);
	fprintf(stderr, "  -s <ssrc> (if unspecified it is random)\n");
	fprintf(stderr, "  -t <ttl> Time to live (default 5)\n");
	fprintf(stderr, "  -m <payload size> Maximum payload size (1450)\n");
	fprintf(stderr, "  -l loop file\n");
	
	exit(1);
}





static void
parse_cmd_line(int argc, char **argv)
{
	int ch;

	// Parse the options/switches
	while ((ch = getopt(argc, argv, "hs:t:lm:")) != -1) {
		switch (ch) {
			case 's':
				// ssrc
				g_ssrc = atoi( optarg );
			break;
			case 'l':
				g_loop = 1;
			break;
			case 't':
				g_ttl = atoi(optarg);
			break;
			case 'm':
				g_mtu = atoi(optarg);
			break;
			case '?':
			case 'h':
			default:
				usage();
		}
	}

/*
	if (conf->loop_file && !conf->filename) {
		fprintf(stderr, "Error: Can't loop unless streaming a file.\n");
		usage();
	}
*/

	
	// Parse the ip address and port
	if (argc > optind) {
		char* port_str = NULL;
		
		g_addr = argv[optind++];
		port_str = strchr(g_addr, '/');
		if (port_str && strlen(port_str)>1) {
			*port_str = 0;
			port_str++;
			g_port = atoi(port_str);
		}
	
	} else {
		fprintf(stderr, "Error: missing address/port.\n\n");
		usage();
	}
	
	// Make sure the port number is even
	if (g_port%2 == 1) g_port--;
	
}


/*
static FILE*
open_source( config_t* conf )
{
	FILE* src = NULL;
	
	if (conf->use_stdio) {
		src = stdin;
	} else if (conf->filename) {
		src = fopen( conf->filename, "r" );
		if (!src) perror( "Failed to open file for reading" );
	} else {
		fprintf(stderr, "open_source() failed: not STDIN or filename.\n");
		exit(-1);
	}

	return src;
}
*/


/*
static void
main_loop( mcast_socket_t* rtp_socket, rtp_packet_t* rtp_packet, FILE* src )
{
	mpa_header_t mh;
	unsigned char buf[8192];
	int synced=0;
	
	while(!feof(src)) {
		unsigned int byte = fgetc(src);
		
		if (byte == 0xFF) {
			
			// Read in the following 3 bytes of header
			buf[0] = byte;
			fread( &buf[1], 1, 3, src);


			// Get information about the frame
			if (mpa_header_parse( buf, &mh )) {
			
				// Once we see two valid MPEG Audio frames in a row
				// then we consider ourselves synced
				if (!synced) {
					// ignore the rest of the first frame
					fread( &buf[4], 1, mh.framesize-4, src);

					// read in next header
					fread( buf, 1, 4, src);			
					if (mpa_header_parse( buf, &mh )) {
						#ifdef DEBUG
							mpa_header_print( &mh );
						#endif
						synced = 1;
					}
				}

				
				// If synced then do something with the frame of MPEG audio
				if (synced) {
				
					// Read in the rest of the frame
					fread( &buf[4], 1, mh.framesize-4, src);
					
					// Copy into RTP packet
					memcpy( &rtp_packet->payload, buf, mh.framesize );
					
					// Send the packet
					rtp_packet_send( rtp_socket, rtp_packet, mh.framesize );
					
					fprintf(stderr,".");
				}
				
			} else {
				if (synced) fprintf(stderr, "Lost sync.\n");
				synced=0;
			}
			
		} else {
			if (synced) fprintf(stderr, "Lost sync.\n");
			synced=0;
		}

	}

}
*/

static RtpSession * init_rtp_session()
{
	RtpSession *session;
	char cname[ STR_BUF_SIZE ];
	char tool[ STR_BUF_SIZE ];
	char *hostname;
	
	// Initialise ortp
	ortp_set_log_level_mask( ORTP_WARNING|ORTP_ERROR|ORTP_FATAL );
	ortp_init();
	
	// Set the MPEG Audio payload type to 14 in the AV profile
	rtp_profile_set_payload(&av_profile, RTP_MPEG_AUDIO_PT, &payload_type_mpeg_audio);


	// Create new session
	session=rtp_session_new(RTP_SESSION_SENDONLY);
	rtp_session_set_remote_addr(session, g_addr, g_port);
	rtp_session_set_multicast_ttl(session, g_ttl);
	rtp_session_set_payload_type(session, RTP_MPEG_AUDIO_PT);
	
	// Static SSRC?
	if (g_ssrc) {
		DEBUG("g_ssrc=%d", g_ssrc);
		rtp_session_set_ssrc(session, g_ssrc);
	}
	
	// Set RTCP parameters
	hostname = gethostname_fqdn();
	snprintf( cname, STR_BUF_SIZE, "%s@%s", PACKAGE, hostname );
	snprintf( tool, STR_BUF_SIZE, "%s (%s/%s)", PROGRAM_NAME, PACKAGE_NAME, PACKAGE_VERSION );
	free( hostname );
	
	DEBUG("rtcp->cname=%s", cname);
	DEBUG("rtcp->tool=%s", tool);
	
	rtp_session_set_source_description(
		session,		// RtpSession*
		cname,			// CNAME
		NULL,			// name
		NULL,			// email
		NULL,			// phone
		NULL,			// loc
		tool,			// tool
		NULL			// note
	);
	
	return session;
}

static void shutdown_rtp_session( RtpSession* session )
{

	rtp_session_uninit( session );
	ortp_exit();	

}

int main(int argc, char **argv)
{
	RtpSession *session;

	// Parse command line parameters
	parse_cmd_line( argc, argv );
	
	DEBUG( "g_addr=%s", g_addr );
	DEBUG( "g_port=%d", g_port );
	
	
	// Create an RTP session
	session = init_rtp_session();
	
	while(g_running) {
		
	
	}
	
	shutdown_rtp_session(session);
	
	// Success !
	return 0;
}


