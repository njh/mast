/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003-2007 University of Southampton
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
 *  $Id:$
 */

#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <resolv.h>

#include <ortp/ortp.h>


#include "config.h"
#include "mast.h"
#include "mpa_header.h"


/* Globals */
static int mast_running = TRUE;
static char* mast_tool_name = NULL;


// RTP Payload Type for MPEG Audio
PayloadType	payload_type_mpeg_audio = {
	PAYLOAD_AUDIO_PACKETIZED, // type
	90000,	// clock rate
	0,		// bytes per sample N/A
	NULL,	// zero pattern N/A
	0,		// pattern_length N/A
	0,		// normal_bitrate
	"mpa",	// MIME Type
	0		// flags
};





/* oRTP callbacks */
static void ssrc_changed_cb(RtpSession *session)
{
	MAST_INFO("The ssrc has changed");

	/* reset the session */
	rtp_session_reset( session );

}

static void pt_changed_cb(RtpSession *session)
{
	MAST_INFO("The payload type has changed");
}

static void network_error_cb(RtpSession *session, const char* msg)
{
	MAST_INFO("Network error: %s", msg);
}


/* Initialise the oRTP library */
RtpSession *mast_init_ortp( char* tool_name, int mode )
{
	RtpSession* session = NULL;
	RtpProfile* profile = &av_profile;
	int log_level = ORTP_WARNING|ORTP_ERROR|ORTP_FATAL;
	
	// Store the tool name
	mast_tool_name = tool_name;
	
	// Initialise the oRTP library
	ortp_init();
	ortp_scheduler_init();

	
	// Set the logging message level
#ifdef DEBUGGING
	MAST_DEBUG( "Compiled with debugging enabled" );
	log_level |= ORTP_DEBUG;
	log_level |= ORTP_MESSAGE;
#endif	
	ortp_set_log_level_mask(log_level);


	// Create RTP session
	session = rtp_session_new( mode );
	if (session==NULL) {
		MAST_FATAL( "Failed to create oRTP session.\n" );	
	}

	// Configure the RTP session
	rtp_session_set_scheduling_mode(session, TRUE);
	rtp_session_set_blocking_mode(session, TRUE);
	rtp_session_set_multicast_loopback(session, TRUE);

	// Callbacks
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)ssrc_changed_cb, 0);
	rtp_session_signal_connect(session,"payload_type_changed",(RtpCallback)pt_changed_cb, 0);
	rtp_session_signal_connect(session,"payload_type_changed",(RtpCallback)network_error_cb, 0);


	// Set the MPEG Audio payload type to 14 in the AV profile
	rtp_profile_set_payload(profile, RTP_MPEG_AUDIO_PT, &payload_type_mpeg_audio);



	// Set RTCP parameters
	mast_set_source_sdes( session );

	return session;
}


/* De-initialise the oRTP library */
void mast_deinit_ortp( RtpSession *session )
{

#ifdef DEBUGGING
	ortp_global_stats_display();
#endif

	rtp_session_uninit( session );
	ortp_exit();	

}


/* Get string of the FQDN for this host */
char* mast_gethostname()
{
	char hostname[ HOST_NAME_MAX ];
	char domainname[ DOMAIN_NAME_MAX ];
	struct hostent	*hp;
	
	if (gethostname( hostname, HOST_NAME_MAX ) < 0) {
		// Failed
		return NULL;
	}
	
	// If it contains a dot, then assume it is a FQDN
    if (strchr(hostname, '.') != NULL)
		return strdup( hostname );

    // Try resolving the hostname into a FQDN
    if ( (hp = gethostbyname( hostname )) != NULL ) {
    	if (strchr(hp->h_name, '.') != NULL)
    		return strdup( hp->h_name );
    }

	// Try appending our domain name
	if ( getdomainname( domainname, DOMAIN_NAME_MAX) == 0
	     && strlen( domainname ) )
	{
		int fqdn_len = strlen( hostname ) + strlen( domainname ) + 2;
		char *fqdn = malloc( fqdn_len );
		snprintf( fqdn, fqdn_len, "%s.%s", hostname, domainname );
		return fqdn;
	}


	// What else can we try?
	return NULL;
}


/* Get the name of the current tool */
char* mast_get_tool_name()
{
	if (mast_tool_name) return mast_tool_name;
	else return "Unknown Tool";
}



/* Set the RTCP session description  */
void mast_set_source_sdes( RtpSession *session )
{
	char cname[ STR_BUF_SIZE ];
	char tool[ STR_BUF_SIZE ];
	char *hostname=NULL;

	hostname = mast_gethostname();
	snprintf( cname, STR_BUF_SIZE, "%s@%s", PACKAGE_NAME, hostname );
	snprintf( tool, STR_BUF_SIZE, "%s (%s/%s)", mast_get_tool_name(), PACKAGE_NAME, PACKAGE_VERSION );
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


/* Handle an error and store the error message */
void mast_message_handler( int level, const char* file, int line, char *fmt, ... )
{
	va_list args;
	FILE *fd = stderr;
	
	// If debugging is enabled then display the filename and line number
#ifdef DEBUGGING
	fprintf( fd, "[%s:%d] ", file, line );
#endif
	
	// Display the error message
	switch( level ) {
		case MSG_LEVEL_DEBUG:	fprintf( fd, "Debug: " ); break;
		case MSG_LEVEL_INFO:	fprintf( fd, "Info: " ); break;
		case MSG_LEVEL_WARNING:	fprintf( fd, "Warning: " ); break;
		case MSG_LEVEL_ERROR:	fprintf( fd, "Error: " ); break;
		case MSG_LEVEL_FATAL:	fprintf( fd, "Fatal Error: " ); break;
	}
	
	// Display the message
	va_start( args, fmt );
	vfprintf( fd, fmt, args );
	fprintf( fd, ".\n" );
	va_end( args );
	
	// If it is an error, then stop
	if (level==MSG_LEVEL_FATAL) {
		exit(-1);
	}
}



/* Signal callback handler */
static void mast_termination_handler(int signum)
{
	mast_running = FALSE;
	switch(signum) {
		case SIGTERM:	fprintf(stderr, "Got termination signal.\n"); break;
		case SIGINT:	fprintf(stderr, "Got interupt signal.\n"); break;
	}
}

/* Returns true if the programme should keep running */
int mast_still_running()
{
	return mast_running;
}

/* Setup signal handlers */
void mast_setup_signals()
{
	// Setup special handling of signals
	signal(SIGTERM, mast_termination_handler);
	signal(SIGINT, mast_termination_handler);
	//signal(SIGHUP, mast_termination_handler);

	// Set this 
	mast_running = TRUE;

}



/* Parse a DSCP class name into an integer value */
int mast_parse_dscp( const char* value )
{

	if (!strcmp(value,"BE"))         return 0x00;
	
	else if (!strcmp(value,"CS0"))   return 0x00;
	else if (!strcmp(value,"CS1"))   return 0x08;
	else if (!strcmp(value,"CS2"))   return 0x10;
	else if (!strcmp(value,"CS3"))   return 0x18;
	else if (!strcmp(value,"CS4"))   return 0x20;
	else if (!strcmp(value,"CS5"))   return 0x28;
	else if (!strcmp(value,"CS6"))   return 0x30;
	else if (!strcmp(value,"CS7"))   return 0x38;
	
	else if (!strcmp(value,"AF11"))  return 0x0A;
	else if (!strcmp(value,"AF12"))  return 0x0C;
	else if (!strcmp(value,"AF13"))  return 0x0E;
	else if (!strcmp(value,"AF21"))  return 0x12;
	else if (!strcmp(value,"AF22"))  return 0x14;
	else if (!strcmp(value,"AF23"))  return 0x16;
	else if (!strcmp(value,"AF31"))  return 0x1a;
	else if (!strcmp(value,"AF32"))  return 0x1C;
	else if (!strcmp(value,"AF33"))  return 0x1E;
	else if (!strcmp(value,"AF41"))  return 0x22;
	else if (!strcmp(value,"AF42"))  return 0x24;
	else if (!strcmp(value,"AF43"))  return 0x26;
	
	else if (!strcmp(value,"EF"))    return 0x2E;

	// Accept integers too
	return atoi( value );
}



// Take an hex SSRC string (from command line) and set it for session
void mast_set_session_ssrc( RtpSession * session, char* ssrc_str )
{
	int ssrc = 0;
	
	// Remove 0x from the start of the string
	if (optarg[0] == '0' && (optarg[1] == 'x' || optarg[1] == 'X')) {
		ssrc_str += 2;
	}
	
	// Parse the hexadecimal number into an integer
	if (sscanf(ssrc_str, "%x", &ssrc)<=0) {
		MAST_ERROR("SSRC should be a hexadeicmal number");
		return;
	}
	
	// Set it in the session
	MAST_DEBUG( "SSRC for session set to 0x%x", ssrc );
	rtp_session_set_ssrc( session, ssrc );

}



/* This isn't part of the public/publish oRTP API */
int rtp_session_rtp_recv (RtpSession * session, uint32_t user_ts);


mblk_t *mast_wait_for_rtp_packet( RtpSession * session, int seconds )
{
	struct timeval timeout;
	fd_set readfds;
	int retval = -1;


	/* reset the session */
	rtp_session_reset( session );
	
	/* set the timeout */
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	/* Watch socket to see when it has input */
	FD_ZERO(&readfds);
	FD_SET( rtp_session_get_rtp_socket(session), &readfds);
	retval = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);


	// Check return value 
	if (retval == -1) {
		MAST_ERROR("select() failed: %s", strerror(errno));
		return NULL;
	} else if (retval==0) {
		MAST_ERROR("Timed out waiting for packet after %d seconds", seconds);
		return NULL;
	}
	
	

	/* recieve packet and put it in queue */
	rtp_session_rtp_recv( session, 0 );
	
	
	/* check that there is something in the queue */
	if (qempty(&session->rtp.rq) ) {
		MAST_ERROR("Queue is empty after trying to recieve packet");
		return NULL;
	}
	
	/* take the packet off the queue and return it */
	return getq(&session->rtp.rq);

}




void mast_update_mpa_pt( mblk_t* packet ) 
{

	if (packet && packet->b_cont) {
		unsigned char* mpa_ptr = packet->b_cont->b_rptr + 4;
		mpa_header_t mh;
		
		if (!mpa_header_parse( mpa_ptr, &mh)) {
			MAST_FATAL("Failed to parse MPEG Audio header");
		}
	
		// Adjust the normal bitrate
		payload_type_mpeg_audio.normal_bitrate = mh.bitrate * 1000;
	}
	
}


/* Return the size of an RTP packet (in bytes) */
int mast_rtp_packet_size( mblk_t* packet ) {
	if (packet && packet->b_cont) {
		return (packet->b_cont->b_wptr - packet->b_cont->b_rptr);
	} else {
		return 0;
	}
}


/* Return the duration of an RTP packet (timestamp difference) */
int mast_rtp_packet_duration( mblk_t* packet )
{
	PayloadType* pt = NULL;
	int payload_size = mast_rtp_packet_size(packet);
	if (payload_size==0) return 0;
	
	// Update oRTP payload type, if it is MPEG Audio
	if (rtp_get_payload_type( packet ) == RTP_MPEG_AUDIO_PT) {
		mast_update_mpa_pt( packet );
		payload_size -= 4;	// extra 4 bytes of headers in MPA
	}

	// Lookup the payload details	
	pt = rtp_profile_get_payload( &av_profile, rtp_get_payload_type( packet ) );
	if (pt == NULL) return 0;
	
	// Work out the duration of the packet
	return (payload_size * pt->clock_rate) / (pt->normal_bitrate/8);
}


/* Return the recommended number of samples to send per packet */
int mast_calc_samples_per_packet( PayloadType *pt, int max_packet_size )
{
	int bits_per_sample = (pt->normal_bitrate / pt->clock_rate);
	int bytes_per_unit = (pt->normal_bitrate*SAMPLES_PER_UNIT)/(pt->clock_rate*8);
	int units_per_packet = max_packet_size / bytes_per_unit;
	int samples_per_packet = units_per_packet * SAMPLES_PER_UNIT;

	// Display packet size information
	MAST_DEBUG( "Bits per sample: %d", bits_per_sample );
	MAST_DEBUG( "Bytes per Unit: %d", bytes_per_unit );
	MAST_DEBUG( "Samples per Unit: %d", SAMPLES_PER_UNIT );
	MAST_DEBUG( "Units per packet: %d", units_per_packet );
	MAST_INFO( "Packet size: %d bytes", (samples_per_packet * bits_per_sample)/8 );
	MAST_INFO( "Packet duration: %d ms", (samples_per_packet*1000 / pt->clock_rate));

	return samples_per_packet;
}


/* Return a payloadtype based on the codec, samplerate and number of channels */
PayloadType* mast_choose_payloadtype( RtpSession * session, const char* payload_type, int samplerate, int channels )
{
	RtpProfile* profile = &av_profile;
	PayloadType* pt = NULL;
	int pt_idx = -1;

	pt_idx = rtp_profile_find_payload_number( profile, payload_type, samplerate, channels );
	if ( pt_idx<0 ) return NULL;
	MAST_INFO( "Payload type index: %d", pt_idx );


	// Get the PayloadType structure
	pt = rtp_profile_get_payload( profile, pt_idx );
	if (pt == NULL) return NULL;


	// Set the payload type in the session
	if (rtp_session_set_send_payload_type( session, pt_idx )) {
		MAST_FATAL("Failed to set session payload type index");
	}

	return pt;
}

