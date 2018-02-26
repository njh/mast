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
#include <ctype.h>

#include <ortp/ortp.h>


#include "MPA_Header.h"
#include "mast.h"

/* Globals */
static int g_mast_running = TRUE;

static char mpeg_audio_mime_type[] = "mpa";

// RTP Payload Type for MPEG Audio
PayloadType    payload_type_mpeg_audio = {
    PAYLOAD_AUDIO_PACKETIZED, // type
    90000,    // clock rate
    0,        // bytes per sample N/A
    NULL,     // zero pattern N/A
    0,        // pattern_length N/A
    0,        // normal_bitrate
    mpeg_audio_mime_type, // MIME Type
    1,        // channels
    NULL,     // various format parameters for the incoming stream
    NULL,     // various format parameters for the outgoing stream
    0,        // flags
    NULL      // user data
};






/* Signal callback handler */
static void mast_termination_handler(int signum)
{
    switch(signum) {
    case SIGTERM:
        MAST_INFO("Got termination signal");
        break;
    case SIGINT:
        MAST_INFO("Got interupt signal");
        break;
    }

    mast_stop_running();
}

/* Returns true if the programme should keep running */
int mast_still_running()
{
    return g_mast_running;
}

/* Stop the process */
void mast_stop_running()
{
    g_mast_running = FALSE;
}

/* Setup signal handlers */
void mast_setup_signals()
{
    // Setup special handling of signals
    signal(SIGTERM, mast_termination_handler);
    signal(SIGINT, mast_termination_handler);
    //signal(SIGHUP, mast_termination_handler);

    g_mast_running = TRUE;
}




/* Handle an error and store the error message */
void mast_message_handler( int level, const char* file, int line, const char *fmt, ... )
{
    va_list args;
    char lastchar = fmt[ strlen(fmt)-1 ];
    FILE *fd = stderr;

    // If debugging is enabled then display the filename and line number
#ifdef DEBUGGING
    fprintf( fd, "[%s:%d] ", file, line );
#endif

    // Display the error message
    switch( level ) {
#ifdef DEBUGGING
    case MSG_LEVEL_DEBUG:
        fprintf( fd, "Debug: " );
        break;
    case MSG_LEVEL_INFO:
        fprintf( fd, "Info: " );
        break;
#endif
    case MSG_LEVEL_WARNING:
        fprintf( fd, "Warning: " );
        break;
    case MSG_LEVEL_ERROR:
        fprintf( fd, "Error: " );
        break;
    case MSG_LEVEL_FATAL:
        fprintf( fd, "Fatal Error: " );
        break;
    }

    // Display the message
    va_start( args, fmt );
    vfprintf( fd, fmt, args );
    if (isalpha(lastchar)) fprintf( fd, "." );
    fprintf( fd, "\n" );
    va_end( args );

    // If it is an error, then stop
    if (level==MSG_LEVEL_FATAL) {
        exit(-1);
    }
}











void mast_update_mpa_pt( mblk_t* packet )
{

    if (packet && packet->b_cont) {
        unsigned char* mpa_ptr = packet->b_cont->b_rptr + 4;
        MPA_Header mh;

        if (!mh.parse( mpa_ptr )) {
            MAST_FATAL("Failed to parse MPEG Audio header");
        }

        // Adjust the normal bitrate
        payload_type_mpeg_audio.normal_bitrate = mh.get_bitrate() * 1000;
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
        payload_size -= 4;    // extra 4 bytes of headers in MPA
    }

    // Lookup the payload details
    pt = rtp_profile_get_payload( &av_profile, rtp_get_payload_type( packet ) );
    if (pt == NULL) return 0;

    // Work out the duration of the packet
    return (payload_size * pt->clock_rate) / (pt->normal_bitrate/8);
}


