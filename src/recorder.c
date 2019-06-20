/*
  recorder.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2018-2019  Nicholas Humfrey
  License: MIT
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "mast.h"


// Globals
const char * ifname = NULL;
mast_sdp_t sdp;

static void usage()
{
    fprintf(stderr, "MAST Recorder version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-recorder [options] [<file.sdp>]\n");
    fprintf(stderr, "   -a <address>   IP Address\n");
    fprintf(stderr, "   -i <iface>     Interface Name to listen on\n");
    fprintf(stderr, "   -p <port>      Port Number (default %s)\n", MAST_DEFAULT_PORT);
    fprintf(stderr, "   -r <rate>      Sample Rate (default %d)\n", MAST_DEFAULT_SAMPLE_RATE);
    fprintf(stderr, "   -f <format>    Audio Format (default L%d)\n", MAST_DEFAULT_SAMPLE_SIZE);
    fprintf(stderr, "   -c <channels>  Channel Count (default %d)\n", MAST_DEFAULT_CHANNEL_COUNT);
    fprintf(stderr, "   -v             Verbose Logging\n");
    fprintf(stderr, "   -q             Quiet Logging\n");

    exit(EXIT_FAILURE);
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "a:p:i:r:f:c:vq?h")) != -1) {
        switch (ch) {
        case 'a':
            mast_sdp_set_address(&sdp, optarg);
            break;
        case 'p':
            mast_sdp_set_port(&sdp, optarg);
            break;
        case 'i':
            ifname = optarg;
            break;
        case 'r':
            sdp.sample_rate = atoi(optarg);
            break;
        case 'f':
            mast_sdp_set_sample_format(&sdp, optarg);
            break;
        case 'c':
            sdp.channel_count = atoi(optarg);
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
        mast_sdp_parse_file(argv[0], &sdp);
    } else if (argc > 1) {
        usage();
    }

    // Validate parameters
    if (quiet && verbose) {
        mast_error("Can't be quiet and verbose at the same time.");
        usage();
    }

    if (strlen(sdp.address) < 1) {
        mast_error("No address specified");
        usage();
    }
}

int main(int argc, char *argv[])
{
    SNDFILE * file = NULL;
    mast_socket_t sock;
    int result;

    mast_sdp_set_defaults(&sdp);
    parse_opts(argc, argv);
    setup_signal_hander();

    mast_info(
        "Recording: %s [L%d/%d/%d]",
        sdp.session_name,
        sdp.sample_size, sdp.sample_rate, sdp.channel_count
    );

    result = mast_socket_open(&sock, sdp.address, sdp.port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }

    file = mast_writer_open("recording.wav", &sdp);
    if (file == NULL) {
        mast_error("Failed to open output file");
    }

    while(running) {
        mast_rtp_packet_t packet;

        int result = mast_rtp_recv(&sock, &packet);
        if (result < 0) break;

        // Is the Payload Type what we were expecting?
        if (sdp.payload_type == -1) {
            mast_info("Payload type of first packet: %d", packet.payload_type);
            mast_sdp_set_payload_type(&sdp, packet.payload_type);
        } else if (sdp.payload_type != packet.payload_type) {
            mast_warn("Received unexpected Payload Type: %d", packet.payload_type);
        }

        mast_debug("RTP packet ts=%lu seq=%u", packet.timestamp, packet.sequence);

        mast_writer_write(file, packet.payload, packet.payload_length);
    }

    if (file) {
        sf_close(file);
    }

    mast_socket_close(&sock);

    return exit_code;
}
