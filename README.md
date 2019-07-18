MAST - Multicast Audio Streaming Toolkit
========================================

[![Build Status](https://travis-ci.org/njh/mast.svg?branch=master)](https://travis-ci.org/njh/mast)


What is MAST ?
--------------

MAST is a set of command line tools for working with multicast Audio over IP streams that use the Real Time Protocol (RTP).
It can be used to interoperate with audio streams that are compatible with the [AES67] standard.

MAST is licenced under the permissive MIT License, see the file [LICENSE.md](/LICENSE.md) for details.



Tools in MAST
-------------

MAST aims to follow the [Unix Philosophy] of "_programs that do one thing and do it well_".

| Tool            | Description                                         |
|-----------------|-----------------------------------------------------|
| mast-info       | Display information about a RTP stream              |
| mast-recorder   | Record/archive audio stream to an audio file        |
| mast-meter      | Programme Peak Meter for checking audio levels      |
| mast-sap-client | Listen for [SAP] packets and write them to disk     |
| mast-sap-server | Periodically transmit [SAP] packets from SDP files  |


Supported Payload Types
-----------------------

MAST supports a number of different payload types from the [RTP audio video profile]:

* [L16] - 16-bit linear PCM
* [L24] - 24-bit linear PCM

It is intended that additional payload types will be added back in the future.


What happened to MAST v1?
-------------------------

MAST v2 is a re-write of [MAST v1](https://github.com/njh/mast/tree/v1), with a focus on high quality linear audio on local area networks, rather distribution over the Internet.


[AES67]: https://en.wikipedia.org/wiki/AES67
[SAP]:   https://en.wikipedia.org/wiki/Session_Announcement_Protocol
[L16]:   https://www.ietf.org/go/rfc4856
[L24]:   https://www.iana.org/go/rfc3190
[RTP audio video profile]: https://en.wikipedia.org/wiki/RTP_audio_video_profile
[Unix Philosophy]: https://en.wikipedia.org/wiki/Unix_philosophy
