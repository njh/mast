/*
  bytestoint.h

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2018-2019  Nicholas Humfrey
  License: MIT
*/

#ifndef BYTES_TO_INT_H
#define BYTES_TO_INT_H

// Convert big-endian 16-bit array of bytes to a signed integer
#define bytesToInt16(a) (int16_t)(((a)[0] << 8) | (a)[1])

// Convert big-endian 16-bit array of bytes to an unsigned integer
#define bytesToUInt16(a) (uint16_t)(((a)[0] << 8) | (a)[1])

// Convert big-endian 24-bit array of bytes to a signed integer
// Leaves the bottom 8-bits set to 0
#define bytesToInt24(a) (int32_t)(((a)[0] << 24) | \
                                  ((a)[1] << 16) | \
                                  ((a)[2] << 8))

// Convert big-endian 32-bit array of bytes to a signed integer
#define bytesToInt32(a) (int32_t)(((a)[0] << 24) | \
                                  ((a)[1] << 16) | \
                                  ((a)[2] << 8) | (a)[3])

// Convert big-endian 32-bit array of bytes to an unsigned integer
#define bytesToUInt32(a) (uint32_t)(((a)[0] << 24) | \
                                    ((a)[1] << 16) | \
                                    ((a)[2] << 8) | (a)[3])

#endif
