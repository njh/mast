/*
  rtp.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2018-2019  Nicholas Humfrey
  License: MIT
*/

#include <stdlib.h>

#include "config.h"
#include "bytestoint.h"
#include "mast.h"


#define bitMask(byte, mask, shift) ((byte & (mask << shift)) >> shift)

int mast_rtp_parse( mast_rtp_packet_t* packet )
{
    size_t header_len = RTP_HEADER_LENGTH;

    // Byte 1
    packet->version = bitMask(packet->buffer[0], 0x02, 6);
    packet->padding = bitMask(packet->buffer[0], 0x01, 5);
    packet->extension = bitMask(packet->buffer[0], 0x01, 4);
    packet->csrc_count = bitMask(packet->buffer[0], 0x0F, 0);

    // Byte 2
    packet->marker = bitMask(packet->buffer[1], 0x01, 7);
    packet->payload_type = bitMask(packet->buffer[1], 0x7F, 0);

    // Bytes 3 and 4
    packet->sequence = bytesToUInt16(&packet->buffer[2]);

    // Bytes 5-8
    packet->timestamp = bytesToUInt32(&packet->buffer[4]);

    // Bytes 9-12
    packet->ssrc = bytesToUInt32(&packet->buffer[8]);

    // Calculate the size of the payload
    header_len += (packet->csrc_count * 4);

    if (packet->extension) {
	uint16_t ext_header_len;

	if (packet->length < header_len + 4)
	    return -1;

	/* ignore extension header ID at buffer + header_len */
	ext_header_len = bytesToUInt16(&packet->buffer[header_len + 2]);
	header_len += 4 + (ext_header_len * 4);
    }

    if (packet->length < header_len)
	return -1;

    packet->payload_length = packet->length - header_len;
    packet->payload = packet->buffer + header_len;

    if (packet->padding) {
	uint8_t padding_len;

	if (packet->payload_length < 1)
	    return -1;

	padding_len = packet->payload[packet->payload_length - 1];
	if (padding_len < 1 || padding_len > packet->payload_length)
	    return -1;

	packet->payload_length -= padding_len;
    }

    // Success
    return 0;
}


int mast_rtp_recv( mast_socket_t* socket, mast_rtp_packet_t* packet )
{
    int len = mast_socket_recv(socket, &packet->buffer, sizeof(packet->buffer));

    // Failure or too short to be an RTP packet?
    if (len <= RTP_HEADER_LENGTH) return -1;

    packet->length = len;

    return mast_rtp_parse(packet);
}

int mast_rtp_packet_duration(mast_rtp_packet_t* packet, mast_sdp_t* sdp)
{
    int frames = ((packet->payload_length / (sdp->sample_size / 8)) / sdp->channel_count);
    return (frames * 1000000) / sdp->sample_rate;
}
