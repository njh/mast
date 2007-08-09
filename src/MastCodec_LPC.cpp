/*
 *  MAST: Multicast Audio Streaming Toolkit
 *
 *  LPC designates an experimental linear predictive encoding contributed
 *  by Ron Frederick, Xerox PARC, which is based on an implementation
 *  written by Ron Zuckerman, Motorola, posted to the Usenet group
 *  comp.dsp on June 26, 1992.
 *
 *  This file is based on the sources from:
 *  ftp://parcftp.xerox.com/pub/net-research/lpc.tar.Z
 *
 *  Each LPC frame is 14 bytes / 160 samples long
 *
 *  $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>

#include "mast.h"
#include "MastCodec_LPC.h"


#define	LPC_DEFAULT_CHANNELS		(1)
#define	LPC_DEFAULT_SAMPLERATE		(8000)





#define MAXWINDOW	1000	/* Max analysis window length */
#define FS			8000.0	/* Sampling rate */

#define DOWN		5		/* Decimation for pitch analyzer */
#define PITCHORDER	4		/* Model order for pitch analyzer */
#define FC			600.0	/* Pitch analyzer filter cutoff */
#define MINPIT		50.0	/* Minimum pitch */
#define MAXPIT		300.0	/* Maximum pitch */

#define MINPER		(int)(FS/(DOWN*MAXPIT)+.5)	/* Minimum period */
#define MAXPER		(int)(FS/(DOWN*MINPIT)+.5)	/* Maximum period */

#define WSCALE		1.5863	/* Energy loss due to windowing */

#define	LPC_FRAMESIZE	(160)
#define LPC_BUFLEN		((LPC_FRAMESIZE * 3) / 2)



float s[MAXWINDOW];
float y[MAXWINDOW];
float h[MAXWINDOW];
float fa[6], u, u1, yp1, yp2;



static void auto_correl(float *w, int n, int p, float *r)
{
	int i, k, nk;

	for (k = 0; k <= p; k++) {
		nk = n - k;
		r[k] = 0.0f;
		for (i = 0; i < nk; i++)
			r[k] += w[i] * w[i + k];
	}
}

static void durbin(float *r, int p, float *k, float *g)
{
	int i, j;
	float a[LPC_FILTORDER + 1], at[LPC_FILTORDER + 1], e;

	for (i = 0; i <= p; i++)
		a[i] = at[i] = 0.0f;

	e = r[0];
	for (i = 1; i <= p; i++) {
		k[i] = -r[i];
		for (j = 1; j < i; j++) {
			at[j] = a[j];
			k[i] -= a[j] * r[i - j];
		}
		k[i] /= e;
		a[i] = k[i];
		for (j = 1; j < i; j++)
			a[j] = at[j] + k[i] * at[i - j];
		e *= 1.0f - k[i] * k[i];
	}

	*g = (float)sqrt(e);
}

static void inverse_filter(float *w, float *k)
{
	int i, j;
	float b[PITCHORDER + 1], bp[PITCHORDER + 1], f[PITCHORDER + 1];

	for (i = 0; i <= PITCHORDER; i++)
		b[i] = f[i] = bp[i] = 0.0;

	for (i = 0; i < LPC_BUFLEN / DOWN; i++) {
		f[0] = b[0] = w[i];
		for (j = 1; j <= PITCHORDER; j++) {
			f[j] = f[j - 1] + k[j] * bp[j - 1];
			b[j] = k[j] * f[j - 1] + bp[j - 1];
			bp[j - 1] = b[j - 1];
		}
		w[i] = f[PITCHORDER];
	}
}

static void calc_pitch(float *w, float *per)
{
	int i, j, rpos;
	float d[MAXWINDOW / DOWN], k[PITCHORDER + 1], r[MAXPER + 1], g, rmax;
	float rval, rm, rp;
	float a, b, c, x, y;
	static int vuv = 0;

	for (i = 0, j = 0; i < LPC_BUFLEN; i += DOWN)
		d[j++] = w[i];
	auto_correl(d, LPC_BUFLEN / DOWN, PITCHORDER, r);
	durbin(r, PITCHORDER, k, &g);
	inverse_filter(d, k);
	auto_correl(d, LPC_BUFLEN / DOWN, MAXPER + 1, r);
	rpos = 0;
	rmax = 0.0;
	for (i = MINPER; i <= MAXPER; i++) {
		if (r[i] > rmax) {
			rmax = r[i];
			rpos = i;
		}
	}

	rm = r[rpos - 1];
	rp = r[rpos + 1];
	rval = rmax / r[0];

	a = 0.5 * rm - rmax + 0.5 * rp;
	b = -0.5 * rm * (2.0 * rpos + 1.0) + 
	    2.0 * rpos * rmax + 0.5 * rp * (1.0 - 2.0 * rpos);
	c = 0.5 * rm * (rpos * rpos + rpos) +
	    rmax * (1.0 - rpos * rpos) + 0.5 * rp * (rpos * rpos - rpos);

	x = -b / (2.0 * a);
	y = a * x * x + b * x + c;
	x *= DOWN;

	rmax = y;
	rval = rmax / r[0];
	if (rval >= 0.4 || (vuv == 3 && rval >= 0.3)) {
		*per = x;
		vuv = (vuv & 1) * 2 + 1;
	} else {
		*per = 0.0;
		vuv = (vuv & 1) * 2;
	}
}

static void lpc_initialise( lpcstate_t* state )
{
	float   r, v, w, wcT;
	int     i;

	for (i = 0; i < LPC_BUFLEN; i++) {
		s[i] = 0.0;
		h[i] = WSCALE * (0.54 - 0.46 *
		       cos(2 * M_PI * i / (LPC_BUFLEN - 1.0)));
	}
	wcT = 2 * M_PI * FC / FS;
	r = 0.36891079f * wcT;
	v = 0.18445539f * wcT;
	w = 0.92307712f * wcT;
	fa[1] = -exp(-r);
	fa[2] = 1.0 + fa[1];
	fa[3] = -2.0 * exp(-v) * cos(w);
	fa[4] = exp(-2.0 * v);
	fa[5] = 1.0 + fa[3] + fa[4];

	u1 = 0.0f;
	yp1 = 0.0f;
	yp2 = 0.0f;

	state->Oldper = 0.0f;
	state->OldG = 0.0f;
	for (i = 0; i <= LPC_FILTORDER; i++) {
		state->Oldk[i] = 0.0f;
		state->bp[i] = 0.0f;
	}
	state->pitchctr = 0;

}


static void lpc_analyze(const float *input, lpcparams_t *output, lpcstate_t* state)
{
	int     i, j;
	float   w[MAXWINDOW], r[LPC_FILTORDER + 1];
	float   per, G, k[LPC_FILTORDER + 1];

	for (i = 0, j = LPC_BUFLEN - LPC_FRAMESIZE; i < LPC_FRAMESIZE; i++, j++) {
		s[j] = *input++;
		u = fa[2] * s[j] - fa[1] * u1;
		y[j] = fa[5] * u1 - fa[3] * yp1 - fa[4] * yp2;
		u1 = u;
		yp2 = yp1;
		yp1 = y[j];
	}

	calc_pitch(y, &per);

	for (i = 0; i < LPC_BUFLEN; i++)
		w[i] = s[i] * h[i];
	auto_correl(w, LPC_BUFLEN, LPC_FILTORDER, r);
	durbin(r, LPC_FILTORDER, k, &G);

	output->period = (u_short)(per * 256.f);
	output->gain = (u_short)(G * 256.f);
	for (i = 0; i < LPC_FILTORDER; i++)
		output->k[i] = (char)(k[i + 1] * 128.f);

	memcpy(s, s + LPC_FRAMESIZE, (LPC_BUFLEN - LPC_FRAMESIZE) * sizeof(s[0]));
	memcpy(y, y + LPC_FRAMESIZE, (LPC_BUFLEN - LPC_FRAMESIZE) * sizeof(y[0]));
}


static void lpc_synthesize(lpcparams_t *input, float *output, lpcstate_t* state)
{
	int i, j;
	register double u, f, per, G, NewG, Ginc, Newper, perinc;
	double k[LPC_FILTORDER + 1], Newk[LPC_FILTORDER + 1],
	       kinc[LPC_FILTORDER + 1];

	per = (double) input->period / 256.;
	G = (double) input->gain / 256.;
	k[0] = 0.0;
	for (i = 0; i < LPC_FILTORDER; i++)
		k[i + 1] = (double) (input->k[i]) / 128.;

	G /= sqrt(LPC_BUFLEN / (per == 0.0? 3.0 : per));
	Newper = state->Oldper;
	NewG = state->OldG;
	for (i = 1; i <= LPC_FILTORDER; i++)
		Newk[i] = state->Oldk[i];

	if (state->Oldper != 0 && per != 0) {
		perinc = (per - state->Oldper) / (double)LPC_FRAMESIZE;
		Ginc = (G - state->OldG) / (double)LPC_FRAMESIZE;
		for (i = 1; i <= LPC_FILTORDER; i++)
			kinc[i] = (k[i] - state->Oldk[i]) / (double)LPC_FRAMESIZE;
	} else {
		perinc = 0.0;
		Ginc = 0.0;
		for (i = 1; i <= LPC_FILTORDER; i++)
			kinc[i] = 0.0;
	}

	if (Newper == 0)
		state->pitchctr = 0;

	for (i = 0; i < LPC_FRAMESIZE; i++) {
		if (Newper == 0) {
			u = ((double)random() / 2147483648.0) * NewG;
		} else {
			if (state->pitchctr == 0) {
				u = NewG;
				state->pitchctr = (int) Newper;
			} else {
				u = 0.0;
				state->pitchctr--;
			}
		}

		f = u;
		for (j = LPC_FILTORDER; j >= 1; j--) {
			register double b = state->bp[j - 1];
			register double kj = Newk[j];
			Newk[j] = kj + kinc[j];
			f -= b * kj;
			b += f * kj;
			state->bp[j] = b;
		}
		state->bp[0] = f;

		*output++ = f;

		Newper += perinc;
		NewG += Ginc;
	}

	state->Oldper = per;
	state->OldG = G;
	for (i = 1; i <= LPC_FILTORDER; i++)
		state->Oldk[i] = k[i];
}









// Calculate the number of samples per packet
size_t MastCodec_LPC::frames_per_packet_internal( size_t max_bytes )
{
	int frames_per_packet = max_bytes/LPC_BYTES_PER_FRAME;
	MAST_DEBUG("LPC frames per packet = %d",frames_per_packet);
	return frames_per_packet * LPC_FRAMESIZE;
}


// Encode a packet's payload
size_t MastCodec_LPC::encode_packet_internal(
		size_t inputsize, 	/* input size in frames */
		mast_sample_t *input,
		size_t outputsize,	/* output size in bytes */
		u_int8_t *output)
{
	size_t frames = (inputsize/LPC_FRAMESIZE);
	size_t f = 0;
	
	if (inputsize % LPC_FRAMESIZE) {
		MAST_DEBUG("encode_lpc: number of input samples (%d) isn't a multiple of %d", inputsize, LPC_FRAMESIZE);
	}

	if (outputsize < (frames*LPC_BYTES_PER_FRAME)) {
		MAST_ERROR("encode_lpc: output buffer isn't big enough");
		return -1;
	}

	// Encode frame by frame
	for(f=0; f<frames; f++) {
		float* in = &input[LPC_FRAMESIZE*f];
		lpcparams_t* out = (lpcparams_t*)output+(LPC_BYTES_PER_FRAME*f);
		lpc_analyze( in, out, &lpc_state );
		out->period = htons( out->period );
	}
	
	return frames*LPC_BYTES_PER_FRAME;
}


// Decode a packet's payload
size_t MastCodec_LPC::decode_packet_internal(
		size_t inputsize,	/* input size in bytes */
		u_int8_t  *input,
		size_t outputsize, 	/* output size in frames */
		mast_sample_t  *output)
{
	size_t frames = (inputsize/LPC_BYTES_PER_FRAME);
	size_t f = 0;
	
	if (outputsize < (frames*LPC_FRAMESIZE)) {
		MAST_ERROR("decode_lpc: output buffer isn't big enough");
		return -1;
	}

	// Decode frame by frame
	for(f=0; f<frames; f++) {
		lpcparams_t* in = (lpcparams_t*)input+(LPC_BYTES_PER_FRAME*f);
		float* out = &output[LPC_FRAMESIZE*f];
		in->period = ntohs( in->period );
		lpc_synthesize(in, out, &lpc_state);
	}

	return frames*LPC_FRAMESIZE;
}




// Initialise the LPC codec
MastCodec_LPC::MastCodec_LPC( MastMimeType *type)
	: MastCodec(type)
{
	
	// Set default values
	this->samplerate = LPC_DEFAULT_SAMPLERATE;
	this->channels = LPC_DEFAULT_CHANNELS;

	
	// Initialise the codec state
	lpc_initialise( &lpc_state );

	// Apply MIME type parameters to the codec
	this->apply_mime_type_params( type );

}
