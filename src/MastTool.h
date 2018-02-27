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
 */

#ifndef _MASTTOOL_H_
#define _MASTTOOL_H_

#include <ortp/ortp.h>

#include "MastMimeType.h"
#include "mast.h"


class MastTool {

// Constructors
public:
    MastTool( const char* name, RtpSessionMode mode );
    ~MastTool();


// Public methods
    mblk_t *wait_for_rtp_packet( int timeout = DEFAULT_TIMEOUT );

    const char* get_tool_name() {
        return tool_name;
    };
    RtpSession* get_session() {
        return session;
    };
    RtpProfile* get_profile() {
        return profile;
    };
    int get_payloadtype_index() {
        return payloadtype_index;
    };
    PayloadType* get_payloadtype() {
        return payloadtype;
    };
    MastMimeType* get_payload_mimetype() {
        return mimetype;
    };
    size_t get_payload_size_limit() {
        return payload_size_limit;
    };

    void set_session_address( const char* address );
    void set_session_ssrc( const char* ssrc_str );
    void set_session_dscp( const char* dscp );
    void set_local_addr( const char* address, int port );
    void set_multicast_ttl( const char* ttl );
    void set_payloadtype_index( int idx );
    void set_payload_mimetype( const char* mimetype );
    void set_payload_mimetype_param( const char* pair );
    void set_payload_size_limit( const char* size_limit );

    /* Enable packet schduling for this tool */
    void enable_scheduling();

    /* Get string of the FQDN for this host */
    char* get_hostname();


// Private methods
private:
    void set_source_sdes();
    int parse_dscp( const char* value );


protected:
    MastMimeType *mimetype;
    RtpSession* session;
    RtpProfile* profile;
    PayloadType* payloadtype;
    int payloadtype_index;
    const char* tool_name;

    size_t payload_size_limit;

};


#endif // _MASTTOOL_H_
