/*

  utils.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#include "config.h"
#include "mast.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

int running = TRUE;
int exit_code = 0;
int quiet = 0;
int verbose = 0;

static void termination_handler(int signum)
{
    running = FALSE;
    switch(signum) {
        case SIGTERM: mast_info("Got termination signal"); break;
        case SIGINT:  mast_info("Got interupt signal"); break;
    }
    signal(signum, termination_handler);
}


void setup_signal_hander()
{
    signal(SIGTERM, termination_handler);
    signal(SIGINT, termination_handler);
    signal(SIGHUP, termination_handler);
}

void mast_log(mast_log_level level, const char *fmt, ...)
{
    time_t t = time(NULL);
    char *time_str;
    va_list args;

    // Display the message level
    switch(level) {
    case mast_LOG_DEBUG:
        if (!verbose)
            return;
        fprintf(stderr, "[DEBUG]   ");
        break;
    case mast_LOG_INFO:
        if (quiet)
            return;
        fprintf(stderr, "[INFO]    ");
        break;
    case mast_LOG_WARN:
        fprintf(stderr, "[WARNING] ");
        break;
    case mast_LOG_ERROR:
        fprintf(stderr, "[ERROR]   ");
        break;
    default:
        fprintf(stderr, "[UNKNOWN] ");
        break;
    }

    // Display timestamp
    time_str = ctime(&t);
    time_str[strlen(time_str) - 1] = 0; // remove \n
    fprintf(stderr, "%s  ", time_str);

    // Display the error message
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    // If an erron then stop
    if (level == mast_LOG_ERROR) {
        // Exit with a non-zero exit code if there was a fatal error
        exit_code++;
        if (running) {
            // Quit gracefully
            running = 0;
        } else {
            fprintf(stderr, "Fatal error while quiting; exiting immediately.\n");
            exit(-1);
        }
    }
}

