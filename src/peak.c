/*
  peak.c

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT
*/

#include <stdlib.h>
#include <math.h>

#include "mast.h"
#include "bytestoint.h"


// Globals
static int channel_count = 0;

// Peak value for each channel in dB
static float peaks[MAST_MAX_CHANNEL_COUNT];


void mast_peak_init(int channels)
{
    int channel;

    channel_count = channels;
    for(channel=0; channel<MAST_MAX_CHANNEL_COUNT; channel++) {
        peaks[channel] = -INFINITY;
    }
}

float mast_peak_read_and_reset(int channel)
{
    float value = peaks[channel];
    peaks[channel] = -INFINITY;

    return value;
}

float mast_peak_read_and_reset_all()
{
    float peak = -INFINITY;
    int channel;

    for(channel=0; channel<channel_count; channel++) {
        if (peak < peaks[channel]) {
            peak = peaks[channel];
        }
        peaks[channel] = -INFINITY;
    }

    return peak;
}

void mast_peak_process_l16(uint8_t* payload, int payload_length)
{
    int16_t peaks_int[MAST_MAX_CHANNEL_COUNT];
    int channel = 0;
    int byte;

    if (payload_length % 2 != 0) {
        mast_warn("payload length is not a multiple of 2");
    }

    for(channel=0; channel < channel_count; channel++) {
        peaks_int[channel] = 0;
    }

    // Convert samples to 16-bit integers and find highest value for each channel
    channel = 0;
    for(byte=0; byte < payload_length; byte += 2) {
        int16_t i = abs(bytesToInt16(&payload[byte]));
        if (i > peaks_int[channel]) {
            peaks_int[channel] = i;
        }

        // Move on to the next channel
        channel++;
        if (channel >= channel_count)
            channel = 0;
    }

    // Convert peak integers to floating-point decibels
    for(channel=0; channel<channel_count; channel++) {
        float db = MAST_POWER_TO_DB((float)peaks_int[channel] / 0x8000);
        if (db > peaks[channel]) {
            peaks[channel] = db;
        }
    }
}

void mast_peak_process_l24(uint8_t* payload, int payload_length)
{
    int peaks_int[MAST_MAX_CHANNEL_COUNT];
    int channel = 0;
    int byte;

    if (payload_length % 3 != 0) {
        mast_warn("payload length is not a multiple of 3");
    }

    for(channel=0; channel < channel_count; channel++) {
        peaks_int[channel] = 0;
    }

    // Convert samples to 32-bit integers and find highest value for each channel
    channel = 0;
    for(byte=0; byte < payload_length; byte += 3) {
        int i = abs(bytesToInt24(&payload[byte]));
        if (i > peaks_int[channel]) {
            peaks_int[channel] = i;
        }

        // Move on to the next channel
        channel++;
        if (channel >= channel_count)
            channel = 0;
    }

    // Convert peak integers to floating-point decibels
    for(channel=0; channel<channel_count; channel++) {
        float db = MAST_POWER_TO_DB((float)peaks_int[channel] / 0x80000000);
        if (db > peaks[channel]) {
            peaks[channel] = db;
        }
    }
}

