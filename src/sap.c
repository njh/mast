#include "mast.h"
#include <string.h>
#include <stdio.h>

int mast_sap_parse(const uint8_t* data, size_t data_len, mast_sap_t* sap)
{
  int offset = 4;
  memset(sap, 0, sizeof(mast_sap_t));

  if (data_len < 12) {
    // FIXME: pick a better minimum packet length
    printf("Packet is too short\n");
    return 1;
  }

  if (((data[0] & 0x20) >> 5) != 1) {
    printf("SAP Version is not 1\n");
    return 1;
  }

  if (((data[0] & 0x10) >> 4) == 0) {
    printf("Address type is IPv4\n");
    offset += 4;
  } else {
    printf("Address type is IPv6\n");
    offset += 16;
    // FIXME: add support for IPv6
    return 1;
  }

  // Store the message type (announce or delete)
  sap->message_type = ((data[0] & 0x04) >> 2);

  if (((data[0] & 0x02) >> 1) == 1) {
    printf("SAP packet is encrypted\n");
    return 1;
  }

  // FIXME: add support for compression
  if (((data[0] & 0x01) >> 0) == 1) {
    printf("SAP packet is compressed\n");
    return 1;
  }

  // Store the Message ID Hash
  sap->message_id_hash = (((uint16_t)data[2] << 8) | (uint16_t)data[3]);

  // Add on the authentication data length
  offset += (data[1] * 4);

  // Check the MIME type
  const char *mime_type = (char*)&data[offset];
  if (mime_type[0] == '\0') {
    offset += 1;
  } else if (strcmp(mime_type, MAST_SDP_MIME_TYPE) == 0) {
    offset += sizeof(MAST_SDP_MIME_TYPE);
  } else if (mime_type[0] == 'v' && mime_type[1] == '=' && mime_type[2] == '0') {
    // No MIME Type field
  } else {
    printf("Invalid Mime type: %s\n", mime_type);
    return 1;
  }

  // Copy over the SDP data
  memcpy(sap->sdp, &data[offset], data_len - offset);

  // Ensure that the SDP is null terminated
  // FIXME: do we need to worry about the buffer size?
  sap->sdp[data_len - offset] = '\0';

  // FIXME: now parse SDP?

  return 0;
}
