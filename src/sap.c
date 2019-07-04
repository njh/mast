/*

  sap.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#include "mast.h"
#include <string.h>

#include <arpa/inet.h>
#include <netinet/ip.h>

int mast_sap_parse(const uint8_t* data, size_t data_len, mast_sap_t* sap)
{
    int offset = 4;
    memset(sap, 0, sizeof(mast_sap_t));

    if (data_len < 12) {
        // FIXME: pick a better minimum packet length
        mast_debug("Error: received SAP packet is too short");
        return 1;
    }

    if (((data[0] & 0x20) >> 5) != 1) {
        mast_debug("Error: received SAP Version is not 1");
        return 1;
    }

    if (((data[0] & 0x10) >> 4) == 0) {
        inet_ntop(AF_INET, &data[4], sap->message_source, sizeof(sap->message_source));
        offset += 4;
    } else {
        inet_ntop(AF_INET6, &data[4], sap->message_source, sizeof(sap->message_source));
        offset += 16;
    }

    // Store the message type (announce or delete)
    sap->message_type = ((data[0] & 0x04) >> 2);

    if (((data[0] & 0x02) >> 1) == 1) {
        mast_debug("Error: received SAP packet is encrypted");
        return 1;
    }

    // FIXME: add support for SAP packet compression (using zlib)
    if (((data[0] & 0x01) >> 0) == 1) {
        mast_debug("Error: received SAP packet is compressed");
        return 1;
    }

    // Add on the authentication data length
    offset += (data[1] * 4);

    // Store the Message ID Hash
    sap->message_id_hash = (((uint16_t)data[2] << 8) | (uint16_t)data[3]);

    // Check the MIME type
    const char *mime_type = (char*)&data[offset];
    if (mime_type[0] == '\0') {
        offset += 1;
    } else if (strcmp(mime_type, MAST_SDP_MIME_TYPE) == 0) {
        offset += sizeof(MAST_SDP_MIME_TYPE);
    } else if (mime_type[0] == 'v' && mime_type[1] == '=' && mime_type[2] == '0') {
        // No MIME Type field
    } else {
        mast_debug("Error: unsupported MIME type in SAP packet: %s", mime_type);
        return 1;
    }

    // Copy over the SDP data
    memcpy(sap->sdp, &data[offset], data_len - offset);

    // Ensure that the SDP is null terminated
    // FIXME: do we need to worry about the buffer size?
    sap->sdp[data_len - offset] = '\0';

    return 0;
}

static uint16_t _crc16(const uint8_t* data, size_t data_len) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (data_len--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }

    return crc;
}

int mast_sap_generate(mast_socket_t *sock, const char* sdp, uint8_t message_type, uint8_t *buffer, size_t buffer_len)
{
    size_t sdp_len = strlen(sdp);
    uint16_t message_hash = _crc16((const uint8_t*)sdp, sdp_len);
    int pos = 0;

    if (sdp_len + 1 + MAST_SAP_MAX_HEADER > buffer_len) {
        // Buffer isn't big enough
        return -1;
    }

    buffer[pos++] = (0x1 << 5); // SAP Version 1
    if (message_type == MAST_SAP_MESSAGE_DELETE) {
        // SAP Flag: T=1
        buffer[0] |= (0x1 << 2);
    }

    buffer[pos++] = 0;  // Authentication Len
    buffer[pos++] = (message_hash & 0xFF00) >> 8;
    buffer[pos++] = (message_hash & 0x00FF);

    if (sock->src_addr.ss_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in*)&sock->src_addr;
        memcpy(&buffer[pos], &sin->sin_addr, sizeof(sin->sin_addr));
        pos += sizeof(sin->sin_addr);
    } else if (sock->src_addr.ss_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&sock->src_addr;
        memcpy(&buffer[pos], &sin6->sin6_addr, sizeof(sin6->sin6_addr));
        pos += sizeof(sin6->sin6_addr);
        buffer[0] |= (0x1 << 4);  // SAP Flag: A=1
    } else {
        return -1;
    }

    // Add the MIME type
    strcpy((char*)&buffer[pos], MAST_SDP_MIME_TYPE);
    pos += sizeof(MAST_SDP_MIME_TYPE);

    // Finally the SDP payload
    strcpy((char*)&buffer[pos], sdp);
    pos += sdp_len;

    return pos;
}

int mast_sap_send_sdp_string(mast_socket_t *sock, const char* sdp, uint8_t message_type)
{
    uint8_t packet[MAST_SAP_MAX_LEN];
    int len;

    len = mast_sap_generate(sock, sdp, message_type, packet, sizeof(packet));
    if (len > 0) {
        return mast_socket_send(sock, packet, len);
    } else {
        return len;
    }
}
