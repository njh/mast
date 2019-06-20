/*
  writer.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2018-2019  Nicholas Humfrey
  License: MIT
*/

#include <stdlib.h>
#include <sndfile.h>

#include "mast.h"
#include "bytestoint.h"



SNDFILE * mast_writer_open(const char* path, mast_sdp_t *sdp)
{
    SF_INFO sfinfo;

    sfinfo.format = SF_FORMAT_WAV | SF_ENDIAN_FILE;
    sfinfo.samplerate = sdp->sample_rate;
    sfinfo.channels = sdp->channel_count;

    switch (sdp->sample_size) {
    case 16:
        sfinfo.format |= SF_FORMAT_PCM_16;
        break;
    case 24:
        sfinfo.format |= SF_FORMAT_PCM_24;
        break;
    case 32:
        sfinfo.format |= SF_FORMAT_PCM_32;
        break;
    default:
        mast_error("Unsupported sample size: %d", sdp->sample_size);
        return NULL;
        break;
    }

    // Check that the format is valid
    if (!sf_format_check(&sfinfo)) {
        mast_error( "Output format is not valid." );
        return NULL;
    }

    return sf_open(path, SFM_WRITE, &sfinfo);
}

void mast_writer_write(SNDFILE *file, uint8_t* payload, int payload_length)
{
    int s32[RTP_MAX_PAYLOAD / 3];
    sf_count_t written = 0;
    sf_count_t count = 0;
    int byte;

    if (payload_length > RTP_MAX_PAYLOAD) {
        mast_error("payload length is greater than maximum RTP payload size");
        return;
    }

    // FIXME: make this work for 16-bit samples

    if (payload_length % 3 != 0) {
        mast_warn("payload length is not a multiple of 3");
    }

    // Convert payload to an array of 32-bit integers
    for(byte=0; byte < payload_length; byte += 3) {
        s32[count++] = bytesToInt24(&payload[byte]);
    }

    written = sf_write_int(file, s32, count);
    if (written != count) {
        mast_error("Failed to write audio to disk: %s", sf_strerror(file));
    }
}
