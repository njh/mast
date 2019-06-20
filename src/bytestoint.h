/*
  bytestoint.h

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2018-2019  Nicholas Humfrey
  License: MIT
*/

#ifndef BYTES_TO_INT_H
#define BYTES_TO_INT_H

// Convert big-endian 16-bit array of bytes to an integer
#define bytesToInt16(a) (((uint16_t)(a)[0] << 8) | (a)[1])

// Convert big-endian 24-bit array of bytes to an integer
// Leaves the bottom 8-bits set to 0
#define bytesToInt24(a) (((uint32_t)(a)[0] << 24) | \
                         ((uint32_t)(a)[1] << 16) | \
                         ((uint32_t)(a)[2] << 8))

// Convert big-endian 32-bit array of bytes to an integer
#define bytesToInt32(a) (((uint32_t)(a)[0] << 24) | \
                         ((uint32_t)(a)[1] << 16) | \
                         ((uint32_t)(a)[2] << 8) | (a)[3])

#endif
