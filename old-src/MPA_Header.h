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

#ifndef MAST_MPA_HEADER_H
#define MAST_MPA_HEADER_H


class MPA_Header {

public:

    // Constructors
    MPA_Header();
    MPA_Header( const u_int8_t* buff );

    // Get parse the header of a frame of mpeg audio
    int parse( const u_int8_t* buff );

    // Print out details of very field for debugging
    void debug( FILE* out = stderr );

    // Summarise audio format in single line
    void print( FILE* out = stderr );


    // Accessors
    u_int32_t get_layer() {
        return this->layer;
    };
    u_int32_t get_version() {
        return this->version;
    };
    u_int32_t get_error_protection() {
        return this->error_protection;
    };
    u_int32_t get_padding() {
        return this->padding;
    };
    u_int32_t get_extension() {
        return this->extension;
    };
    u_int32_t get_mode() {
        return this->mode;
    };
    u_int32_t get_mode_ext() {
        return this->mode_ext;
    };
    u_int32_t get_copyright() {
        return this->copyright;
    };
    u_int32_t get_original() {
        return this->original;
    };
    u_int32_t get_emphasis() {
        return this->emphasis;
    };

    u_int32_t get_channels();
    u_int32_t get_bitrate();
    u_int32_t get_samplerate();
    u_int32_t get_samples();
    u_int32_t get_framesize();



private:

    u_int32_t syncword;
    u_int32_t layer;
    u_int32_t version;
    u_int32_t error_protection;
    u_int32_t bitrate_index;
    u_int32_t samplerate_index;
    u_int32_t padding;
    u_int32_t extension;
    u_int32_t mode;
    u_int32_t mode_ext;
    u_int32_t copyright;
    u_int32_t original;
    u_int32_t emphasis;

};



#endif // _MAST_MPA_HEADER_H_
