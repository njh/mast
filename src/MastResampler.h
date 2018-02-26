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
 *  $Id$
 *
 */


#ifndef _MASTRESAMPLER_H_
#define _MASTRESAMPLER_H_

#include <sys/types.h>
#include "MastAudioBuffer.h"
#include "mast.h"


class MastResampler {

// Constructors
public:
    MastResampler( int in_channels, int quality = SRC_SINC_MEDIUM_QUALITY);
    ~MastResampler();


// Public methods
    void resample(MastAudioBuffer *input, MastAudioBuffer *output);



private:
    SRC_STATE* src_state;
    int channels;
    double ratio;

};


#endif // _MASTRESAMPLER_H_
