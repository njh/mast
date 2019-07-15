/*
  meter.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include "mast.h"
#include "bytestoint.h"


// Globals
const char * ifname = NULL;
mast_sdp_t sdp;
int period = 125;  // Update every 125ms


static void usage()
{
    fprintf(stderr, "MAST Meter version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-meter [options] [<file.sdp>]\n");
    fprintf(stderr, "   -a <address>   IP Address\n");
    fprintf(stderr, "   -i <iface>     Interface Name to listen on\n");
    fprintf(stderr, "   -p <port>      Port Number (default %s)\n", MAST_DEFAULT_PORT);
    fprintf(stderr, "   -r <rate>      Sample Rate (default %d)\n", MAST_DEFAULT_SAMPLE_RATE);
    fprintf(stderr, "   -e <encoding>  Encoding (default %s)\n", mast_encoding_name(MAST_DEFAULT_ENCODING));
    fprintf(stderr, "   -c <channels>  Channel Count (default %d)\n", MAST_DEFAULT_CHANNEL_COUNT);
    fprintf(stderr, "   -v             Verbose Logging\n");
    fprintf(stderr, "   -q             Quiet Logging\n");

    exit(EXIT_FAILURE);
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "o:a:p:i:r:f:c:vq?h")) != -1) {
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
            sdp.encoding = mast_encoding_lookup(optarg);
            if (sdp.encoding < 0) mast_error("Invalid encoding format: %s", optarg);
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

static void display_peaks(int channel_count)
{
    int channel;
    float db;

    printf("[");
    for(channel=0; channel<channel_count; channel++) {
        if (channel != 0)
            printf(", ");
        db = mast_peak_read_and_reset(channel);
        printf("%3.3f", db);
    }
    printf("]\n");
}

int main(int argc, char *argv[])
{
    mast_socket_t sock;
    int result;
    unsigned int time = 0;
    int first = TRUE;

    mast_sdp_set_defaults(&sdp);
    parse_opts(argc, argv);
    setup_signal_hander();

    mast_info(
        "Receiving: %s [%s/%d/%d]",
        sdp.session_name,
        mast_encoding_name(sdp.encoding), sdp.sample_rate, sdp.channel_count
    );

    result = mast_socket_open_recv(&sock, sdp.address, sdp.port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }


    // Make STDOUT unbuffered
    setbuf(stdout, NULL);

    while(running) {
        mast_rtp_packet_t packet;
        unsigned int samples;

        int result = mast_rtp_recv(&sock, &packet);
        if (result < 0) break;

        if (first) {
            // Is the Payload Type what we were expecting?
            if (sdp.payload_type == -1) {
                mast_info("Payload type of first packet: %d", packet.payload_type);
                mast_sdp_set_payload_type(&sdp, packet.payload_type);
            } else if (sdp.payload_type != packet.payload_type) {
                mast_warn("Received unexpected Payload Type: %d", packet.payload_type);
            }

            mast_peak_init(sdp.channel_count);
            first = FALSE;
        }

        mast_peak_process_l24(packet.payload, packet.payload_length);

        samples = ((packet.payload_length / 3) / sdp.channel_count);
        time += (samples * 1000) / sdp.sample_rate;
        if (time > period) {
            display_peaks(sdp.channel_count);
            time = 0;
        }
    }

    mast_socket_close(&sock);

    return exit_code;
}
