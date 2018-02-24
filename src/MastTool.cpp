/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
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

#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <resolv.h>

#include "MastTool.h"
#include "mast.h"



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






// Constructors
MastTool::MastTool( const char* tool_name, RtpSessionMode mode )
{
	int log_level = ORTP_WARNING|ORTP_ERROR|ORTP_FATAL;


	// Initialise defaults
	this->session = NULL;
	this->profile = &av_profile;
	this->mimetype = new MastMimeType();
	this->payloadtype = NULL;
	this->payloadtype_index = -1;
	this->tool_name = tool_name;
	this->payload_size_limit = DEFAULT_PAYLOAD_LIMIT;


	// Initialise the oRTP library
	ortp_init();

	
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

	// Enabled multicast loopback
	rtp_session_set_multicast_loopback(session, TRUE);

	// Callbacks
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)ssrc_changed_cb, 0);
	rtp_session_signal_connect(session,"payload_type_changed",(RtpCallback)pt_changed_cb, 0);
	rtp_session_signal_connect(session,"network_error",(RtpCallback)network_error_cb, 0);


	// Set the MPEG Audio payload type to 14 in the AV profile
	rtp_profile_set_payload(profile, RTP_MPEG_AUDIO_PT, &payload_type_mpeg_audio);


	// Set RTCP parameters
	this->set_source_sdes();

}





MastTool::~MastTool()
{
	
#ifdef DEBUGGING
	ortp_global_stats_display();
#endif

	// Free up memory used by session
	if (session) {
		rtp_session_uninit( session );
		session = NULL;
	}

	// Free up memory used by mimetype
	delete mimetype;
	
	// Shut down oRTP library
	ortp_exit();	

}


void MastTool::enable_scheduling()
{
	
	// Enable packet scheduling
	ortp_scheduler_init();

	// Enable session scheduling and blocking
	rtp_session_set_scheduling_mode(session, TRUE);
	rtp_session_set_blocking_mode(session, TRUE);
}




/* This isn't part of the public/publish oRTP API */
extern "C" {
	int rtp_session_rtp_recv (RtpSession * session, uint32_t user_ts);
};


/* Wait for an RTP packet to arrive and return it */
mblk_t *MastTool::wait_for_rtp_packet( int seconds )
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



/* Get string of the FQDN for this host */
char* MastTool::get_hostname()
{
	char hostname[ HOST_NAME_MAX ];
	char domainname[ DOMAIN_NAME_MAX ];
	struct hostent	*hp;
	
	if (::gethostname( hostname, HOST_NAME_MAX ) < 0) {
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
		char *fqdn = (char*)malloc( fqdn_len );
		snprintf( fqdn, fqdn_len, "%s.%s", hostname, domainname );
		return fqdn;
	}


	// What else can we try?
	return NULL;
}



/* Set the RTCP session description  */
void MastTool::set_source_sdes()
{
	char cname[ STR_BUF_SIZE ];
	char tool[ STR_BUF_SIZE ];
	char *hostname=NULL;

	hostname = this->get_hostname();
	snprintf( cname, STR_BUF_SIZE, "%s@%s", PACKAGE_NAME, hostname );
	snprintf( tool, STR_BUF_SIZE, "%s (%s/%s)", this->get_tool_name(), PACKAGE_NAME, PACKAGE_VERSION );
	free( hostname );
	
	rtp_session_set_source_description(
		this->session,		// RtpSession*
		cname,				// CNAME
		NULL,				// name
		NULL,				// email
		NULL,				// phone
		NULL,				// loc
		tool,				// tool
		NULL				// note
	);

}




/* Take an hex SSRC string (from command line) and set it for session */
void MastTool::set_session_ssrc( const char* ssrc_str )
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


/* Parse a DSCP class name into an integer value */
int MastTool::parse_dscp( const char* value )
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
	
	// See http://www.geant.net/server/show/conWebDoc.500
	else if (!strcmp(value,"EF"))		return 0x2E;
	else if (!strcmp(value,"LBE"))		return 0x08;
	else if (!strcmp(value,"DWS"))		return 0x20;
	else if (!strcmp(value,"PREMIUM"))	return 0x2E;

	// Accept integers too
	return atoi( value );
}





void MastTool::set_session_dscp( const char* dscp_str )
{
	int dscp_int = this->parse_dscp( dscp_str );

	if (rtp_session_set_dscp( this->get_session(), dscp_int )) {
		MAST_FATAL("Failed to set DSCP value");
	}

}


void MastTool::set_multicast_ttl( const char* ttl_str )
{
	int ttl_int = atoi( ttl_str );
	
	if (rtp_session_set_multicast_ttl( session, ttl_int )) {
		MAST_FATAL("Failed to set multicast TTL");
	}
}


void MastTool::set_payload_size_limit( char const* size_limit_str )
{
	int size_limit = atoi( size_limit_str );
	
	if (size_limit <= 1) MAST_WARNING( "Invalid payload size: %d", size_limit);

	this->payload_size_limit = size_limit;
}

void MastTool::set_session_address( const char* in_addr )
{
	char* address = strdup( in_addr );
	char* portstr = NULL;
	int port = DEFAULT_RTP_PORT;

	// Look for port in the address
	portstr = strchr(address, '/');
	if (portstr && strlen(portstr)>1) {
		*portstr = 0;
		portstr++;
		port = atoi(portstr);
	}

	
	// Make sure the port number is even
	if (port%2 == 1) port--;
	
	
	// Send of recieve address?
	if (this->session->mode == RTP_SESSION_RECVONLY) {
	
		// Set the local address/port
		if (rtp_session_set_local_addr( session, address, port, port + 1 )) {
			MAST_FATAL("Failed to set local address/port (%s/%d)", address, port);
		} else {
			MAST_INFO( "Local address: %s/%d", address,  port );
		}
		
	} else if (this->session->mode == RTP_SESSION_SENDONLY) {
	
		// Set the remote address/port
		if (rtp_session_set_remote_addr( session, address, port )) {
			MAST_FATAL("Failed to set remote address/port (%s/%d)", address, port);
		} else {
			MAST_INFO( "Remote address: %s/%d", address,  port );
		}
		
	} else {
		MAST_FATAL("Mode unsupported by MastTool: %d", this->session->mode);
	}
	
	free( address );
}


void MastTool::set_payloadtype_index( int idx )
{
	// Lookup the payload type
	PayloadType* pt = rtp_profile_get_payload( profile, idx );
	if (pt==NULL) MAST_WARNING("Failed to get payload type for index %d", idx);
	this->payloadtype = pt;
	
	// Store it
	this->payloadtype_index = idx;
	MAST_INFO( "Payload type index: %d", idx );

	// Apply it to the session
	if (rtp_session_set_send_payload_type( session, idx )) {
		MAST_FATAL("Failed to set session payload type index");
	}
}

void MastTool::set_payload_mimetype( const char* mimetype_str )
{
	// Parse the string
	this->mimetype->parse( mimetype_str );
}


void MastTool::set_payload_mimetype_param( const char* pair_str )
{
	// Parse the parameter
	this->mimetype->set_param( pair_str );
}
