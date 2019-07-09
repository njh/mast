/*
  info.c

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
    fprintf(stderr, "MAST Info version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-info [options] <file.sdp>\n");
    fprintf(stderr, "   -i <iface>     Interface Name to listen on\n");
    fprintf(stderr, "   -v             Verbose Logging\n");
    fprintf(stderr, "   -q             Quiet Logging\n");

    exit(EXIT_FAILURE);
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "i:vq?h")) != -1) {
        switch (ch) {
        case 'i':
            ifname = optarg;
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
    } else {
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
    mast_rtp_packet_t packet;
    mast_socket_t sock;
    uint64_t tai;
    int result;

    mast_sdp_set_defaults(&sdp);
    parse_opts(argc, argv);
    setup_signal_hander();


    result = mast_socket_open_recv(&sock, sdp.address, sdp.port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }


    // Wait for an RTP packet
    result = mast_rtp_recv(&sock, &packet);
    if (result < 0) exit(-1);

    // Is the Payload Type what we were expecting?
    if (sdp.payload_type == -1) {
        mast_sdp_set_payload_type(&sdp, packet.payload_type);
    } else if (sdp.payload_type != packet.payload_type) {
        mast_warn("RTP packet Payload Type does not match SDP: %d", packet.payload_type);
    }

    // Display information about the session
    printf("\n");
    printf("Session Information\n");
    printf("===================\n");
    printf("Session ID       : %s-%s\n", sdp.session_origin, sdp.session_id);
    printf("Dest Address     : %s:%s\n", sdp.address, sdp.port );
    printf("Session Name     : %s\n", sdp.session_name );
    printf("Description      : %s\n", sdp.information );
    printf("PTP Grandmaster  : %s\n", sdp.ptp_gmid );
    printf("RTP Clock Offset : %llu\n", sdp.clock_offset );
    printf("\n");

    printf("Payload Details\n");
    printf("===============\n");
    printf("Encoding         : %s\n", mast_encoding_name(sdp.encoding));
    printf("Sample Rate      : %d Hz\n", sdp.sample_rate);
    printf("Channel Count    : %d\n", sdp.channel_count);
    printf("Packet Duration  : %f ms\n", sdp.packet_duration);
    printf("\n");

    // Display information about the packet received
    printf("RTP Header\n");
    printf("==========\n");
    printf("Payload type     : %u\n", packet.payload_type );
    printf("Payload size     : %u bytes\n", packet.payload_length );
    printf("SSRC Identifier  : 0x%8.8x\n", packet.ssrc );
    printf("Marker Bit       : %s\n", packet.marker ? "Set" : "Not Set");
    printf("Sequence Number  : %u\n", packet.sequence );

    tai = (((uint64_t)packet.timestamp + sdp.clock_offset) / sdp.sample_rate );
    printf("Timestamp        : %u\n", packet.timestamp );
    printf("TAI Seconds      : %llu\n", tai);
    printf("\n");

    mast_socket_close(&sock);

    return exit_code;
}
