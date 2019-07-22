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
        strncpy(sdp->session_id, session_id, sizeof(sdp->session_id)-1);
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
        strncpy(sdp->session_origin, addr, sizeof(sdp->session_origin)-1);
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
        mast_sdp_set_address(sdp, addr);
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
        mast_sdp_set_port(sdp, port);
    } else {
        mast_error("Invalid connection port: %s", port);
    }

    if (proto == NULL || strcmp(proto, "RTP/AVP") != 0) {
        mast_error("SDP transport protocol is not RTP/AVP: %s", proto);
    }

    if (fmt == NULL || strlen(fmt) > 2) {
        mast_error("SDP media format is not valid: %s", fmt);
    } else {
        mast_sdp_set_payload_type(sdp, atoi(fmt));
    }
}

static void sdp_attribute_parse(mast_sdp_t *sdp, char* line, int line_num)
{
    char *attr = strsep(&line, ":");
    if (attr == NULL) return;

    if (strcmp(attr, "rtpmap") == 0) {
        char *pt = strsep(&line, " ");

        if (pt && atoi(pt) == sdp->payload_type) {
            char *encoding = strsep(&line, "/");
            char *sample_rate = strsep(&line, "/");
            char *channel_count = strsep(&line, "/");

            if (encoding) mast_sdp_set_encoding_name(sdp, encoding);
            if (sample_rate) sdp->sample_rate = atoi(sample_rate);
            if (channel_count) sdp->channel_count = atoi(channel_count);
        }
    } else if (strcmp(attr, "ptime") == 0) {
        sdp->packet_duration = atof(line);
    } else if (strcmp(attr, "ts-refclk") == 0) {
        char *clksrc_type = strsep(&line, "=");
        char *ptp_version = strsep(&line, ":");
        char *ptp_gmid = strsep(&line, ":");
        char *ptp_domain = strsep(&line, ":");

        if (clksrc_type && strcmp(clksrc_type, "ptp") == 0) {
            if (ptp_version && strcmp(ptp_version, "IEEE1588-2008") != 0) {
                mast_warn("PTP version is not IEEE1588-2008: %s", ptp_version);
            }

            if (ptp_gmid) {
                strncpy(sdp->ptp_gmid, ptp_gmid, sizeof(sdp->ptp_gmid)-1);
            }

            if (ptp_domain && strcmp(ptp_domain, "0") != 0) {
                mast_warn("PTP domain is not 0: %s", ptp_version);
            }
        } else {
            mast_warn("SDP Clock Source is not PTP");
        }
    } else if (strcmp(attr, "mediaclk") == 0) {
        char *mediaclk_type = strsep(&line, "=");
        char *clock_offset = strsep(&line, " ");

        if (mediaclk_type && strcmp(mediaclk_type, "direct") == 0) {
            if (clock_offset) {
                sdp->clock_offset = atoll(clock_offset);
            }
        } else {
            mast_warn("SDP Media Clock is not set to direct: %s", mediaclk_type);
        }
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
        strncpy(sdp->session_name, &line[2], sizeof(sdp->session_name)-1);
        break;

    case 'i':
        strncpy(sdp->information, &line[2], sizeof(sdp->information)-1);
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
    char buffer[MAST_SDP_MAX_LEN];
    int result = mast_read_file_string(filename, buffer, sizeof(buffer));
    if (result)
        return result;

    // Now parse the SDP string
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
    sdp->encoding = -1;

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

void mast_sdp_set_defaults(mast_sdp_t *sdp)
{
    memset(sdp, 0, sizeof(mast_sdp_t));

    mast_sdp_set_port(sdp, MAST_DEFAULT_PORT);

    sdp->payload_type = -1;
    mast_sdp_set_encoding(sdp, MAST_DEFAULT_ENCODING);
    sdp->sample_rate = MAST_DEFAULT_SAMPLE_RATE;
    sdp->channel_count = MAST_DEFAULT_CHANNEL_COUNT;
}

void mast_sdp_set_address(mast_sdp_t *sdp, const char *address)
{
    if (address && strlen(address) > 1) {
        strncpy(sdp->address, address, sizeof(sdp->address)-1);
    } else {
        sdp->address[0] = '\0';
    }
}

void mast_sdp_set_port(mast_sdp_t *sdp, const char *port)
{
    if (port && strlen(port) > 1) {
        strncpy(sdp->port, port, sizeof(sdp->port)-1);
    } else {
        sdp->port[0] = '\0';
    }
}

void mast_sdp_set_payload_type(mast_sdp_t *sdp, int payload_type)
{
    sdp->payload_type = payload_type;

    if (payload_type == 10) {
        mast_sdp_set_encoding(sdp, MAST_ENCODING_L16);
        sdp->sample_rate = 44100;
        sdp->channel_count = 2;
    } else if (payload_type == 11) {
        mast_sdp_set_encoding(sdp, MAST_ENCODING_L16);
        sdp->sample_rate = 44100;
        sdp->channel_count = 1;
    } else if (payload_type < 96) {
        mast_error("Unsupported static payload type: %d", payload_type);
    }
}

void mast_sdp_set_encoding(mast_sdp_t *sdp, int encoding)
{
    sdp->encoding = encoding;

    switch(encoding) {
    case MAST_ENCODING_L8:
        sdp->sample_size = 8;
        break;
    case MAST_ENCODING_L16:
    case MAST_ENCODING_PCMU:
    case MAST_ENCODING_PCMA:
    case MAST_ENCODING_G722:
    case MAST_ENCODING_GSM:
        sdp->sample_size = 16;
        break;
    case MAST_ENCODING_L24:
        sdp->sample_size = 24;
        break;
    default:
        sdp->sample_size = 0;
        sdp->encoding = -1;
        break;
    }
}

void mast_sdp_set_encoding_name(mast_sdp_t *sdp, const char* encoding_name)
{
    int encoding = mast_encoding_lookup(encoding_name);
    mast_sdp_set_encoding(sdp, encoding);
    if (encoding < 0) {
        mast_error("Unsupported encoding: %s", encoding_name);
    }
}
