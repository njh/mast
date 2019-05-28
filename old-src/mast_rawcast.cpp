/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  Copyright (C) 2003-2007 Nicholas J. Humfrey <njh@aelius.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>


#include "MPA_Header.h"
#include "MastTool.h"
#include "mast.h"


#define MAST_TOOL_NAME      "mast_rawcast"
#define READ_BUFFER_SIZE    (8196)


/* Global Variables */
int g_loop_file = FALSE;
char* g_filename = NULL;



static int usage() {

    printf( "Multicast Audio Streaming Toolkit (version %s)\n", PACKAGE_VERSION);
    printf( "%s [options] <address>[/<port>] <filename>\n", MAST_TOOL_NAME);
    printf( "    -s <ssrc>     Source identifier in hex (default is random)\n");
    printf( "    -t <ttl>      Time to live\n");
    printf( "    -p <payload>  The payload type to send\n");
    printf( "    -z <size>     Set the per-packet payload size\n");
    printf( "    -d <dscp>     DSCP Quality of Service value\n");
    printf( "    -l            Loop the audio file\n");

    exit(1);

}


static void parse_cmd_line(int argc, char **argv, MastTool* tool)
{
    int ch;


    // Parse the options/switches
    while ((ch = getopt(argc, argv, "s:t:p:z:d:lh?")) != -1)
        switch (ch) {
        case 's':
            tool->set_session_ssrc( optarg );
            break;
        case 't':
            tool->set_multicast_ttl( optarg );
            break;
        case 'p':
            tool->set_payload_mimetype( optarg );
            break;
        case 'z':
            tool->set_payload_size_limit( optarg );
            break;
        case 'd':
            tool->set_session_dscp( optarg );
            break;
        case 'l':
            g_loop_file = TRUE;
            break;

        case '?':
        case 'h':
        default:
            usage();
        }


    // Parse the ip address and port
    if (argc > optind) {
        tool->set_session_address( argv[optind++] );
    } else {
        MAST_ERROR("missing address/port to send to");
        usage();
    }

    // Get the input file
    if (argc > optind) {
        g_filename = argv[optind++];
    } else {
        MAST_ERROR("missing input filename");
        usage();
    }


}



static FILE* open_source( char* filename )
{
    FILE* src = NULL;

    if (filename==NULL) {
        MAST_FATAL("filename is NULL");
    } else if (strcmp( filename, "-" ) == 0) {
        src = stdin;
    } else {
        src = fopen( filename, "rb" );
    }

    return src;
}


static void main_loop_gsm( MastTool *tool, FILE* input, u_int8_t* buffer )
{
    int ever_synced = 0;
    int synced = 0;
    int timestamp = 0;

    /* rfc3551: Blocks of 160 audio samples are compressed into 33 octets, for an
       effective data rate of 13,200 b/s.
       Every such buffer begins with a 4 bit signature (0xD)
    */

    while( mast_still_running() ) {
        int bytes_read = 0;
        int byte = -1;

        // Check for EOF
        if (feof( input )) {
            MAST_WARNING( "Reached EOF" );

            if (!ever_synced) {
                MAST_ERROR("Reached end of file without ever gaining sync." );
                break;
            }

            if (g_loop_file) {
                if (fseek(input, 0, SEEK_SET)) {
                    MAST_ERROR("Failed to seek to start of file: %s", strerror(errno) );
                    break;
                }

                // Clear the EOF error
                clearerr( input );

            } else {
                break;
            }
        }

        // Check for other error
        if (ferror(input)) {
            MAST_ERROR( "File read error: %s", strerror(errno) );
            break;
        }

        // Read a single byte from the input file
        byte = fgetc( input );
        if (byte < 0) continue;


        // Check that first nibble is the signature
        if ((byte & 0xF0) == GSM_FRAME_SIGNATURE) {

            // Now read in the rest of the frame
            buffer[0] = byte;
            bytes_read = fread( &buffer[1], 1, GSM_FRAME_BYTES-1, input);
            if (bytes_read<0) continue;

            // If not synced, then check the byte at the start of the next frame
            if (!synced) {
                byte = fgetc( input );
                if (byte < 0) continue;

                // Again, checked the signature
                if ((byte & 0xF0) == GSM_FRAME_SIGNATURE) {
                    synced=1;
                    ever_synced=1;
                    MAST_DEBUG( "Gained GSM Audio sync");
                    ungetc(byte, input);
                } else {
                    synced=0;
                    MAST_DEBUG( "Failed to gain GSM Audio sync");
                }
            }

            // FIXME: support multiple GSM frames per packet

            // Send out a packet
            if (synced) {

                // Send audio payload
                rtp_session_send_with_ts(tool->get_session(), buffer, GSM_FRAME_BYTES, timestamp);

                // Frame of GSM audio is 160 samples long
                timestamp += GSM_FRAME_SAMPLES;   //  * frames_per_packet
            }

        } else {
            // Lost sync
            if (synced) MAST_WARNING( "Lost GSM Audio sync");
            synced = 0;
        }

    }

}

static void main_loop_mpa( MastTool *tool, FILE* input, u_int8_t* buffer )
{
    MPA_Header mh;
    u_int8_t* mpabuf = buffer+4;
    int timestamp = 0;
    int ever_synced = 0;
    int synced = 0;

    // Four null bytes at start of buffer
    // which make up the MPEG Audio RTP header
    // (see rfc2250)
    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;


    while( mast_still_running() ) {
        int bytes_read = 0;
        int byte = -1;

        // Check for EOF
        if (feof( input )) {
            MAST_WARNING( "Reached EOF" );

            if (!ever_synced) {
                MAST_ERROR("Reached end of file without ever gaining sync." );
                break;
            }

            if (g_loop_file) {
                if (fseek(input, 0, SEEK_SET)) {
                    MAST_ERROR("Failed to seek to start of file: %s", strerror(errno) );
                    break;
                }

                // Clear the EOF error
                clearerr( input );

            } else {
                break;
            }
        }

        // Check for other error
        if (ferror(input)) {
            MAST_ERROR( "File read error: %s", strerror(errno) );
            break;
        }

        // Read a single byte from the input file
        byte = fgetc( input );
        if (byte < 0) continue;


        // Check for signature byte
        if (byte == 0xFF) {

            // Read in the following 3 bytes of header
            mpabuf[0] = byte;
            bytes_read = fread( &mpabuf[1], 1, 3, input);
            if (bytes_read!=3) continue;


            // Get information about the frame
            if (mh.parse( mpabuf )) {

                // Once we see two valid MPEG Audio frames in a row
                // then we consider ourselves synced
                if (!synced) {
                    // ignore the rest of the first frame
                    bytes_read = fread( &mpabuf[4], 1, mh.get_framesize()-4, input);
                    if (bytes_read!=(int)mh.get_framesize()-4) continue;

                    // read in next header
                    bytes_read = fread( mpabuf, 1, 4, input);
                    if (bytes_read!=4) continue;

                    // Check that the next header is valid too
                    if (mh.parse( mpabuf )) {
                        MAST_INFO( "Gained sync on MPEG audio stream" );
                        mh.print( stdout );
                        synced = 1;
                        ever_synced = 1;

                        // Work out how big payload will be
                        if (tool->get_payload_size_limit() < mh.get_framesize()) {
                            MAST_FATAL("audio frame size is bigger than packet size limit");
                        }

                        // FIXME: support multiple frames per packet
                        printf("MPEG Audio Frame size: %d bytes\n", mh.get_framesize() );
                    }


                    // FIXME: reimplement so we don't loose first frame


                }


                // If synced then do something with the frame of MPEG audio
                if (synced) {

                    // Read in the rest of the frame
                    fread( &mpabuf[4], 1, mh.get_framesize()-4, input);

                    // Send audio payload (plus 4 null bytes at the start)
                    rtp_session_send_with_ts(tool->get_session(), buffer, mh.get_framesize()+4, timestamp);

                    // Timestamp for MPEG Audio is based on fixed 90kHz clock rate
                    timestamp += ((mh.get_samples() * 90000) / mh.get_samplerate());   //  * frames_per_packet

                }

            } else {
                if (synced) MAST_WARNING( "Lost MPEG Audio sync");
                synced=0;
            }

        } else {
            if (synced) MAST_WARNING( "Lost MPEG Audio sync");
            synced=0;
        }

    }

}


static void select_payload_type( MastTool* tool, MastMimeType* mime )
{
    const char* mime_subtype = mime->get_minor();

    // This method only works for a few select payload types
    // the MastSendTool::set_chosen_payload() suppots lots more
    if (strcmp(mime_subtype, "MPA")==0) {
        tool->set_payloadtype_index( RTP_MPEG_AUDIO_PT );
    } else if (strcmp(mime_subtype, "GSM")==0) {
        tool->set_payloadtype_index( RTP_GSM_PT );
    } else {
        MAST_FATAL("The payload type '%s' isn't support by %s", mime_subtype, tool->get_tool_name());
    }

}



int main(int argc, char **argv)
{
    MastTool *tool = NULL;
    FILE* input = NULL;
    u_int8_t *buffer = NULL;


    // Create an RTP session
    tool = new MastTool( MAST_TOOL_NAME, RTP_SESSION_SENDONLY );
    tool->enable_scheduling();

    // Parse the command line arguments
    // and configure the session
    parse_cmd_line( argc, argv, tool );


    // Select the payload type based on the chosen MIME type
    select_payload_type( tool, tool->get_payload_mimetype() );


    // Open the input source
    input = open_source( g_filename );
    if (!input) {
        MAST_FATAL( "Failed to open input file: %s", strerror(errno) );
    }

    // Allocate memory for the read buffer
    buffer = (u_int8_t*)malloc( READ_BUFFER_SIZE );
    if (!buffer) {
        MAST_FATAL( "Failed to allocate memory for the read buffer: %s", strerror(errno) );
    }

    // Setup signal handlers
    mast_setup_signals();



    // The main loop
    if (tool->get_payloadtype_index() == RTP_MPEG_AUDIO_PT) {

        // MPEG Audio header parser
        main_loop_mpa( tool, input, buffer );

    } else if (tool->get_payloadtype_index() == RTP_GSM_PT) {

        // GSM header parser
        main_loop_gsm( tool, input, buffer );

    } else {
        MAST_ERROR("Unsuppored raw payload type index: %d", tool->get_payloadtype_index() );
    }




    // Free the read buffer
    if (buffer) free(buffer);

    // Close input file
    if (fclose( input )) {
        MAST_ERROR("Failed to close input file:\n%s", strerror(errno) );
    }

    // Close RTP session
    delete tool;


    // Success !
    return 0;
}


