/*

  sap-server.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#include "mast.h"


// Globals
const char *address = MAST_SAP_ADDRESS_LOCAL;
const char *port = MAST_SAP_PORT;
const char *ifname = NULL;
const char *sdp_path = NULL;
int publish_period = 10;

static void usage()
{
    fprintf(stderr, "MAST SAP Server version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-sap-server [options] <path>\n");
    fprintf(stderr, "   -a <address>    Multicast address to listen on (default %s)\n", address);
    fprintf(stderr, "   -p <port>       Port number to lisen on (default %s)\n", port);
    fprintf(stderr, "   -i <interface>  Network interface to publish to on\n");
    fprintf(stderr, "   -t <secs>       Number of seconds between publishes (default %ds)\n", publish_period);
    fprintf(stderr, "   -v              Enable verbose mode\n");
    fprintf(stderr, "   -q              Enable quiet mode\n");

    exit(EXIT_FAILURE);
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "a:p:i:t:vq?h")) != -1) {
        switch (ch) {
        case 'a':
            address = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'i':
            ifname = optarg;
            break;
        case 't':
            publish_period = atoi(optarg);
            break;
        case 'v':
            verbose = TRUE;
            break;
        case 'q':
            quiet = TRUE;
            break;
        case '?':
        case 'h':
        default:
            usage();
        }
    }

    // Check remaining arguments
    argc -= optind;
    argv += optind;
    if (argc == 1) {
        sdp_path = argv[0];
    } else {
        usage();
    }

    // Validate parameters
    if (quiet && verbose) {
        mast_error("Can't be quiet and verbose at the same time.");
        usage();
    }
}

int main(int argc, char *argv[])
{
    mast_socket_t sock;
    char buffer[MAST_SDP_MAX_LEN];
    mast_sdp_t sdp;
    int result;

    parse_opts(argc, argv);
    setup_signal_hander();

    result = mast_read_file_string(sdp_path, buffer, sizeof(buffer));
    if (result) {
        return EXIT_FAILURE;
    }

    result = mast_sdp_parse_string(buffer, &sdp);
    if (result) {
        return EXIT_FAILURE;
    }

    result = mast_socket_open_send(&sock, address, port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }

    while(running) {
        mast_sap_send_sdp_string(&sock, buffer, MAST_SAP_MESSAGE_ANNOUNCE);
        sleep(publish_period);
    }

    mast_socket_close(&sock);

    return exit_code;
}
