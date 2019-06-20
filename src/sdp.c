/*

  sdp.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/


#include "mast.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void sdp_origin_parse(mast_sdp_t *sdp, char* line, int line_num)
{
    char *session_id, *nettype, *addrtype, *addr;

    strsep(&line, " ");              // 1: Username
    session_id = strsep(&line, " "); // 2: Session Id
    strsep(&line, " ");              // 3: Session Version
    nettype = strsep(&line, " ");    // 4: Network Type
    addrtype = strsep(&line, " ");   // 5: Address Type
    addr = strsep(&line, " ");       // 6: Address

    if (session_id) {
        strcpy(sdp->session_id, session_id);
    } else {
        mast_error("Failed to parse session id on line %d", line_num);
    }

    if (nettype == NULL || strcmp(nettype, "IN") != 0) {
        mast_error("SDP origin net type is not 'IN': %s", nettype);
    }

    if (addrtype == NULL || (strcmp(addrtype, "IP4") != 0 && strcmp(addrtype, "IP6") != 0)) {
        mast_error("SDP Origin address type is not IP4/IP6: %s", nettype);
    }

    if (addr) {
        strcpy(sdp->session_origin, addr);
    } else {
        mast_error("Failed to parse origin address on line %d", line_num);
    }
}

static void sdp_connection_parse(mast_sdp_t *sdp, char* line, int line_num)
{
    char *nettype = strsep(&line, " ");
    char *addrtype = strsep(&line, " ");
    char *addr = strsep(&line, " /");

    if (nettype == NULL || strcmp(nettype, "IN") != 0) {
        mast_error("SDP net type is not 'IN': %s", nettype);
        return;
    }

    if (addrtype == NULL || (strcmp(addrtype, "IP4") != 0 && strcmp(addrtype, "IP6") != 0)) {
        mast_warn("SDP Address type is not IP4/IP6: %s", nettype);
        return;
    }

    if (addr) {
        strcpy(sdp->address, addr);
    } else {
        mast_error("Failed to parse connection address on line %d", line_num);
    }
}

static void sdp_media_parse(mast_sdp_t *sdp, char* line, int line_num)
{
    char *media = strsep(&line, " ");
    char *port = strsep(&line, " ");
    char *proto = strsep(&line, " ");
    char *fmt = strsep(&line, " ");

    if (media == NULL || strcmp(media, "audio") != 0) {
        mast_error("SDP media type is not audio: %s", media);
    }

    if (port == NULL || strlen(port) > 2) {
        strcpy(sdp->port, port);
    } else {
        mast_error("Invalid connection port: %s", port);
    }

    if (proto == NULL || strcmp(proto, "RTP/AVP") != 0) {
        mast_error("SDP transport protocol is not RTP/AVP: %s", proto);
    }

    if (fmt == NULL || strlen(fmt) > 2) {
        mast_error("SDP media format is not valid: %s", fmt);
    } else {
        sdp->payload_type = atoi(fmt);
    }
}

static void sdp_attribute_parse(mast_sdp_t *sdp, char* line, int line_num)
{
    char *attr = strsep(&line, ":");
    if (attr == NULL) return;

    if (strcmp(attr, "rtpmap") == 0) {
        char *pt = strsep(&line, " ");

        if (pt && atoi(pt) == sdp->payload_type) {
            char *format = strsep(&line, "/");
            char *sample_rate = strsep(&line, "/");
            char *channel_count = strsep(&line, "/");

            if (format && format[0] == 'L') {
                sdp->sample_size = atoi(&format[1]);
            } else {
                mast_warn("Unknown sample format: %s", format);
                sdp->sample_size = 0;
            }
            if (sample_rate) sdp->sample_rate = atoi(sample_rate);
            if (channel_count) sdp->channel_count = atoi(channel_count);
        }
    } else if (strcmp(attr, "ptime") == 0) {
        sdp->packet_duration = atoi(line);
    }
}

static int sdp_parse_line(char* line, mast_sdp_t* sdp, int line_num)
{
    // Remove whitespace from the end of the line
    for(int i=strlen(line) - 1; i>0; i--) {
        if (isspace(line[i])) {
            line[i] = '\0';
        } else {
            break;
        }
    }

    if (line_num == 1 && strcmp("v=0", line) != 0) {
        mast_warn("First line of SDP is not v=0", line_num);
        return 1;
    }

    if (!islower(line[0])) {
        mast_warn("Line %d of SDP file does not start with a lowercase letter", line_num);
        return 1;
    }

    if (line[1] != '=') {
        mast_warn("Line %d of SDP file is not an equals sign");
        return 1;
    }

    if (strlen(line) < 3) {
        mast_warn("Line %d of SDP file is too short", line_num);
        return 1;
    }

    switch (line[0]) {
    case 'o':
        sdp_origin_parse(sdp, &line[2], line_num);
        break;

    case 's':
        strcpy(sdp->session_name, &line[2]);
        break;

    case 'i':
        strcpy(sdp->information, &line[2]);
        break;

    case 'c':
        sdp_connection_parse(sdp, &line[2], line_num);
        break;

    case 'm':
        sdp_media_parse(sdp, &line[2], line_num);
        break;

    case 'a':
        sdp_attribute_parse(sdp, &line[2], line_num);
        break;
    }

    return 0;
}


int mast_sdp_parse_file(const char* filename, mast_sdp_t* sdp)
{
    char buffer[2048];
    int bytes;

    // Open the file for reading
    FILE* file = fopen(filename, "rb");
    if (!file) {
        mast_error(
            "Failed to open SDP file '%s': %s",
            filename,
            strerror(errno)
        );
        return -1;
    }

    // Read as much as we can into the buffer
    bytes = fread(buffer, 1, sizeof(buffer)-1, file);
    if (bytes <= 0) {
        mast_error(
            "Error reading from SDP file '%s': %s",
            filename,
            strerror(errno)
        );
        return -1;
    }

    // Now parse the SDP string
    buffer[bytes] = '\0';
    return mast_sdp_parse_string(buffer, sdp);
}

int mast_sdp_parse_string(const char* str, mast_sdp_t* sdp)
{
    char line_buffer[255];
    size_t start = 0;
    size_t end = 0;
    size_t str_len = strlen(str);
    int line_num = 1;

    memset(sdp, 0, sizeof(mast_sdp_t));

    while (start < str_len) {
        // Find the end of the line
        end = 0;
        for (int i = start; i < str_len; i++) {
            if (str[i] == '\n') {
                end = i;
                break;
            }
        }

        if (end == 0) {
            mast_warn("Failed to find the end of line %d", line_num);
            break;
        } else {
            int line_len = end-start;
            int result;

            if (line_len > sizeof(line_buffer)-1) {
                mast_warn("Ingoring line %d because it is too long", line_num);
                break;
            } else {
                memcpy(line_buffer, &str[start], line_len);
                line_buffer[line_len] = '\0';

                result = sdp_parse_line(line_buffer, sdp, line_num);
                if (result) return result;
            }

            start = end + 1;
            line_num++;
        }
    }

    // Success
    return 0;
}
