/*

  mast.h

  Copyright (C) 2018  Nicholas Humfrey
  License: MIT

*/

#include "config.h"

#ifndef MAST_H
#define MAST_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif



// ------- SAP packet handling ---------

#define MAST_SDP_MIME_TYPE   "application/sdp"

enum
{
  MAST_SAP_MESSAGE_ANNOUNCE = 0,
  MAST_SAP_MESSAGE_DELETE = 1
};

typedef struct
{
   uint8_t message_type;
   uint16_t message_id_hash;

   // FIXME: add originating source address

   char sdp[2048];
} mast_sap_t;


int mast_sap_parse(const uint8_t* data, size_t data_len, mast_sap_t* sap);


// ------- Utilities ---------

void setup_signal_hander();
extern int running;
extern int exit_code;
extern int verbose;
extern int quiet;

typedef enum {
    mast_LOG_DEBUG,
    mast_LOG_INFO,
    mast_LOG_WARN,
    mast_LOG_ERROR
} mast_log_level;


void mast_log(mast_log_level level, const char *fmt, ...);

// Only display debug if verbose
#define mast_debug( ... ) \
		mast_log(mast_LOG_DEBUG, __VA_ARGS__ )

// Don't show info when quiet
#define mast_info( ... ) \
		mast_log(mast_LOG_INFO, __VA_ARGS__ )

#define mast_warn( ... ) \
		mast_log(mast_LOG_WARN, __VA_ARGS__ )

// All errors are fatal
#define mast_error( ... ) \
		mast_log(mast_LOG_ERROR, __VA_ARGS__ )


#endif
