MAST - Multicast Audio Streaming Toolkit
========================================

[![Build Status](https://travis-ci.org/njh/mast.svg?branch=master)](https://travis-ci.org/njh/mast)


What is MAST ?
--------------

MAST is an audio streaming broadcaster and client using RTP over
IPv4 and IPv6 Multicast/Unicast.

Unlike VAT and RAT, which are designed primerily for audio conferencing, MAST
is designed to be used for audio distribution and broadcast. It is currently limited 
to recieving a single audio source, unlike RAT which can mix serveral sources.

It supports many of the audio payload types in the Audio-visual Profile (RTP/AVP).

MAST is licenced under the GNU General Public License, see the file COPYING for details.



Tools in MAST
-------------

| Tool           | Description                                         |
|----------------|-----------------------------------------------------|
| mast_info      | Display information about first RTP packet received |
| mast_cast      | Live Audio broadcaster that sources audio from JACK |
| mast_filecast  | Audio file broadcaster                              |
| mast_record    | Record/archive audio stream to audio file           |
| mast_rawcast   | Directly broadcast a previously encoded audio file  |
| mast_rawrecord | Record/archive raw streams directly to disk         |


Supported Payloads
------------------

| PT    | Clock Rate | Channels | Encoding Name             |
|-------|------------|----------|---------------------------|
| 0     |  8000Hz    | Mono     | u-law encoded             |
| 3     |  8000Hz    | Mono     | GSM                       |
| 7     |  8000Hz    | Mono     | LPC                       |
| 8     |  8000Hz    | Mono     | A-law encoded             |
| 10    | 44.1kHz    | Stereo   | Raw 16-bit linear audio   |
| 11    | 44.1kHz    | Mono     | Raw 16-bit linear audio   |
| 14    | -          | -        | MPEG Audio (encoder only) |


Details are here:
http://www.iana.org/assignments/rtp-parameters




Current Limitations
-------------------

- No mixer support (can only recieve single source).
- No sample rate conversion support.
- No upmixing/downmixing support.
- No clock skew error correction.
- mast_rawcast currently only supports GSM and MPEG Audio.
- due to problems with oRTP, only 8000Hz streams can be recieved.


