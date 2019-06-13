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
