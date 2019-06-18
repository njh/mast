/*

  sap-client.c

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
const char *dir = NULL;

static void usage()
{
    fprintf(stderr, "mast-sap-client version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-sap-client [options] [<dir>]\n");
    fprintf(stderr, "   -a <address>    Multicast address to listen on (default %s)\n", address);
    fprintf(stderr, "   -p <port>       Port number to lisen on (default %s)\n", port);
    fprintf(stderr, "   -i <interface>  Network interface to listen on\n");
    fprintf(stderr, "   -v              Enable verbose mode\n");
    fprintf(stderr, "   -q              Enable quiet mode\n");

    exit(EXIT_FAILURE);
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "a:p:i:vq?h")) != -1) {
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
        dir = argv[0];
    } else if (argc > 1) {
        usage();
    }

    // Check that the directory exists
    if (dir && !mast_directory_exists(dir)) {
        mast_error("Directory to write SDP files to doesn't exist.");
        exit(EXIT_FAILURE);
    }

    // Validate parameters
    if (quiet && verbose) {
        mast_error("Can't be quiet and verbose at the same time.");
        usage();
    }
}

static int sdp_filepath(mast_sdp_t *sdp, char* filepath, int filepath_max_len)
{
    return snprintf(
               filepath, filepath_max_len,
               "%s/%s-%s.sdp",
               dir,
               sdp->session_origin,
               sdp->session_id
           );
}

static void write_sdp_file(mast_sap_t *sap, const char *filepath)
{
    FILE *file;
    int result;

    file = fopen(filepath, "wb");
    if (!file) {
        mast_error(
            "Failed to open SDP file '%s' for writing: %s",
            filepath,
            strerror(errno)
        );
        return;
    }

    result = fwrite(sap->sdp, strlen(sap->sdp), 1, file);
    if (result != 1) {
        mast_error(
            "Failed to write to SDP file '%s': %s",
            filepath,
            strerror(errno)
        );
    }

    fclose(file);
}

static void receive_sap_packet(mast_socket_t *sock)
{
    uint8_t packet[2048];
    mast_sap_t sap;
    mast_sdp_t sdp;
    const char* verb;
    int packet_len, result;

    packet_len = mast_socket_recv(sock, packet, sizeof(packet));
    if (packet_len <= 0) return;
    mast_debug("Received: %d bytes", packet_len);

    // Parse the SAP packet
    result = mast_sap_parse(packet, packet_len, &sap);
    if (result) return;
    verb = (sap.message_type == MAST_SAP_MESSAGE_ANNOUNCE) ? "Announce" : "Delete";

    // Then parse the SDP content
    result = mast_sdp_parse(sap.sdp, &sdp);
    if (result) return;

    mast_info(
        "SAP %s: %s - %s/%s [L%d/%d/%d]",
        verb, sdp.session_name,
        sdp.address, sdp.port,
        sdp.sample_size, sdp.sample_rate, sdp.channel_count
    );


    if (dir) {
        char filepath[PATH_MAX];
        sdp_filepath(&sdp, filepath, PATH_MAX-1);

        if (sap.message_type == MAST_SAP_MESSAGE_ANNOUNCE) {
            write_sdp_file(&sap, filepath);
        } else if (sap.message_type == MAST_SAP_MESSAGE_DELETE) {
            result = unlink(filepath);
            if (result) {
                mast_error(
                    "Failed to delete SDP file '%s': %s",
                    filepath,
                    strerror(errno)
                );
            }
        }
    }
}


int main(int argc, char *argv[])
{
    mast_socket_t sock;
    int result;

    parse_opts(argc, argv);
    setup_signal_hander();

    result = mast_socket_open(&sock, address, port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }

    while(running) {
        receive_sap_packet(&sock);
    }

    mast_socket_close(&sock);

    return exit_code;
}
