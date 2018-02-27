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

#ifndef _MAST_CODEC_LPC_H_
#define _MAST_CODEC_LPC_H_

#include "MastCodec.h"




/*
 * LPC stucture declarations
 */
#define LPC_FILTORDER            (10)

typedef struct {
    unsigned short period;
    unsigned char gain;
    char k[LPC_FILTORDER];
    char pad;
} lpcparams_t;

/*
 * we can't use 'sizeof(lpcparams_t)' because some compilers
 * add random padding so define size of record that goes over net.
 */
#define LPC_BYTES_PER_FRAME        (14)

typedef struct {
    double Oldper;
    double OldG;
    double Oldk[LPC_FILTORDER + 1];
    double bp[LPC_FILTORDER + 1];
    int pitchctr;
} lpcstate_t;


class MastCodec_LPC : public MastCodec {

public:

    // Constructor
    MastCodec_LPC( MastMimeType* type );


protected:

    // Internal: encode a packet of audio - returns number of bytes encoded, or -1 on failure
    virtual size_t encode_packet_internal( size_t num_frames, mast_sample_t *input, size_t out_size, u_int8_t *output );

    // Internal: decode a packet of audio - returns number of samples decoded, or -1 on failure
    virtual size_t decode_packet_internal( size_t inputsize, u_int8_t *input, size_t outputsize, mast_sample_t *output );

    // Internal: return the number of frames per packet for the current parameters
    virtual size_t frames_per_packet_internal( size_t max_bytes );

private:

    // LPC codec state storage
    lpcstate_t lpc_state;

};




#endif // _MAST_CODEC_GSM_H_
