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


#ifndef	_MAST_H_
#define	_MAST_H_


// Defaults
#define DEFAULT_RTP_SSRC			(0)
#define DEFAULT_MULTICAST_TTL		(5)
#define DEFAULT_MAX_PAYLOAD_SIZE	(1450)
#define DEFAULT_RTP_PORT			(5004)

// Standard string buffer size
#define STR_BUF_SIZE				(1025)

// Payload type for MPEG Audio
#define RTP_MPEG_AUDIO_PT			(14)



// Use by gethostname()
#ifndef HOST_NAME_MAX
#ifdef _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX	_POSIX_HOST_NAME_MAX
#else
#define HOST_NAME_MAX	(256)
#endif
#endif

#ifndef DOMAIN_NAME_MAX
#define DOMAIN_NAME_MAX	(1024)
#endif



#define DEFAULT_RTP_PORT		"5004"
#define RTP_PAYLOAD_TYPE_MPA	(14)

// Message levels
#define MSG_LEVEL_DEBUG		(1)
#define MSG_LEVEL_INFO		(2)
#define MSG_LEVEL_WARNING	(3)
#define MSG_LEVEL_ERROR		(4)

// Only display debugging messages if debugging is enabled
#ifdef DEBUGGING
#define DEBUG(s ...)  message_handler( MSG_LEVEL_DEBUG, __FILE__, __LINE__, s )
#else
#define DEBUG
#endif

#define INFO(s ...)  message_handler( MSG_LEVEL_INFO, __FILE__, __LINE__, s )
#define WARNING(s ...)  message_handler( MSG_LEVEL_WARNING, __FILE__, __LINE__, s )
#define ERROR(s ...)  message_handler( MSG_LEVEL_ERROR, __FILE__, __LINE__, s )

// In util.c
extern char* gethostname_fqdn();
extern void message_handler( int level, const char* file, int line, char *fmt, ... );


#endif
