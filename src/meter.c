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

enum meter_modes {
    METER_MODE_DPM,
    METER_MODE_DB_ARRAY
};

// Globals
const char * ifname = NULL;
mast_sdp_t sdp;
int period = 125;  // Update every 125ms
int console_width = 79;
int mode = METER_MODE_DPM;

int dpeak = 0;
int dtime = 0;
int decay_len = 0;


static void usage()
{
    fprintf(stderr, "MAST Meter version %s\n\n", PACKAGE_VERSION);
    fprintf(stderr, "Usage: mast-meter [options] [<file.sdp>]\n");
    fprintf(stderr, "   -m <mode>      Display Mode (default dpm)\n");
    fprintf(stderr, "   -a <address>   IP Address\n");
    fprintf(stderr, "   -i <iface>     Interface Name to listen on\n");
    fprintf(stderr, "   -p <port>      Port Number (default %s)\n", MAST_DEFAULT_PORT);
    fprintf(stderr, "   -r <rate>      Sample Rate (default %d)\n", MAST_DEFAULT_SAMPLE_RATE);
    fprintf(stderr, "   -e <encoding>  Encoding (default %s)\n", mast_encoding_name(MAST_DEFAULT_ENCODING));
    fprintf(stderr, "   -c <channels>  Channel Count (default %d)\n", MAST_DEFAULT_CHANNEL_COUNT);
    fprintf(stderr, "   -P <milisecs>  Update period (default %dms)\n", period);
    fprintf(stderr, "   -v             Verbose Logging\n");
    fprintf(stderr, "   -q             Quiet Logging\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Modes:\n");
    fprintf(stderr, "   dpm            Console Digital Peak Meter\n");
    fprintf(stderr, "   array          Array of dB values\n");

    exit(EXIT_FAILURE);
}

static int parse_meter_mode(const char* str)
{
    int mode = -1;

    if (strcmp("dpm", str) == 0) {
        mode = METER_MODE_DPM;
    } else if (strcmp("array", str) == 0) {
        mode = METER_MODE_DB_ARRAY;
    } else {
        mast_error("Unknown meter mode: %s", str);
        usage();
    }

    return mode;
}

static void parse_opts(int argc, char **argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "m:a:p:i:r:f:c:vq?h")) != -1) {
        switch (ch) {
        case 'm':
            mode = parse_meter_mode(optarg);
            break;
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

/*
  IEC 60268-18 scaling
  Taken from meterbridge by Steve Harris and used with permission

	db: the signal stength in db
	width: the size of the meter
*/
static int iec_26818_scale(float db, int width) {
    float def = 0.0f; /* Meter deflection %age */

    if (db < -70.0f) {
        def = 0.0f;
    } else if (db < -60.0f) {
        def = (db + 70.0f) * 0.25f;
    } else if (db < -50.0f) {
        def = (db + 60.0f) * 0.5f + 2.5f;
    } else if (db < -40.0f) {
        def = (db + 50.0f) * 0.75f + 7.5;
    } else if (db < -30.0f) {
        def = (db + 40.0f) * 1.5f + 15.0f;
    } else if (db < -20.0f) {
        def = (db + 30.0f) * 2.0f + 30.0f;
    } else if (db < 0.0f) {
        def = (db + 20.0f) * 2.5f + 50.0f;
    } else {
        def = 100.0f;
    }

    return (int)( (def / 100.0f) * ((float) width) );
}

static void display_console_peak_meter(int width)
{
    float db = mast_peak_read_and_reset_all();
    int size = iec_26818_scale(db, width);
    int i;

    if (size > dpeak) {
        dpeak = size;
        dtime = 0;
    } else if (dtime++ > decay_len) {
        dpeak = size;
    }

    printf("\r");

    for(i=0; i<size-1; i++) {
        printf("#");
    }

    if (dpeak==size) {
        printf("I");
    } else {
        printf("#");
        for(i=0; i<dpeak-size-1; i++) {
            printf(" ");
        }
        printf("I");
    }

    for(i=0; i<width-dpeak; i++) {
        printf(" ");
    }
}


static void display_console_peak_scale( int width )
{
    int i=0;
    const int marks[11] = { 0, -5, -10, -15, -20, -25, -30, -35, -40, -50, -60 };
    char *scale = malloc( width+1 );
    char *line = malloc( width+1 );

    // Initialise the scale
    for(i=0; i<width; i++) {
        scale[i] = ' ';
        line[i]='_';
    }
    scale[width] = 0;
    line[width] = 0;

    // 'draw' on each of the db marks
    for(i=0; i < 11; i++) {
        char mark[5];
        int pos = iec_26818_scale( marks[i], width )-1;
        int spos, slen;

        // Create string of the db value
        snprintf(mark, 4, "%d", marks[i]);

        // Position the label string
        slen = strlen(mark);
        spos = pos-(slen/2);
        if (spos<0) spos=0;
        if (spos+strlen(mark)>width) spos=width-slen;
        memcpy( scale+spos, mark, slen );

        // Position little marker
        line[pos] = '|';
    }

    // Print it to screen
    printf("%s\n", scale);
    printf("%s\n", line);
    free(scale);
    free(line);
}

static void display_peak_db_array(int channel_count)
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

static void init_meter(int channel_count)
{
    mast_peak_init(channel_count);

    switch(mode) {
    case METER_MODE_DPM:
        // Calculate the decay length (should be 1600ms)
        decay_len = 1600 / period;

        display_console_peak_scale(console_width);
        break;
    case METER_MODE_DB_ARRAY:
        // No initialisation required
        break;
    }
}

static void display_meter(int channel_count)
{
    switch(mode) {
    case METER_MODE_DPM:
        display_console_peak_meter(console_width);
        break;
    case METER_MODE_DB_ARRAY:
        display_peak_db_array(channel_count);
        break;
    }
}

int main(int argc, char *argv[])
{
    mast_socket_t sock;
    int result;
    unsigned int time = 0;
    int first_packet = TRUE;

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

        if (first_packet) {
            // Is the Payload Type what we were expecting?
            if (sdp.payload_type == -1) {
                mast_info("Payload type of first packet: %d", packet.payload_type);
                mast_sdp_set_payload_type(&sdp, packet.payload_type);
            } else if (sdp.payload_type != packet.payload_type) {
                mast_warn("Received unexpected Payload Type: %d", packet.payload_type);
            }

            init_meter(sdp.channel_count);
            first_packet = FALSE;
        }

        mast_peak_process_l24(packet.payload, packet.payload_length);

        samples = ((packet.payload_length / 3) / sdp.channel_count);
        time += (samples * 1000) / sdp.sample_rate;
        if (time > period) {
            display_meter(sdp.channel_count);
            time = 0;
        }
    }

    mast_socket_close(&sock);

    return exit_code;
}
