/*
 *  pcm6cast: Multicast PCM audio over IPv6
 *
 *  By Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
 *
 *  Copyright (C) 2003 University of Southampton
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 */
 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include <shout/shout.h>


#include "config.h"
#include "audio.h"
#include "pcm6cast.h"
#include "pcm6shout.h"

#define ENCODED_BUFFER_SIZE		LAME_MAXMP3BUFFER


/* Extenal Globals */
extern config_t config;
extern shout_t* shout;



/* Globals */
char* encoded_buffer = NULL;


#ifdef USE_LAME
#include <lame/lame.h>
lame_global_flags *lame;
#endif


#ifdef USE_VORBIS
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

struct {

	ogg_sync_state    oy;  // sync and verify incoming physical bitstream
	ogg_stream_state  os;  // take physical pages, weld into a logical stream of packets
	ogg_page          og;  // one Ogg bitstream page. Vorbis packets are inside
	ogg_packet        op;  // one raw packet of data for decode
	vorbis_info       vi;  // struct that stores all the static vorbis bitstream settings
	vorbis_comment    vc;  // struct that stores all the bitstream user comments
	vorbis_dsp_state  vd;  // central working state for the packet->PCM decoder
	vorbis_block      vb;  // local working space for packet->PCM decode
  
  
	float quality;
	int in_header;
	int downmix;
	
} vorbis;

#endif






void shout_display_config( )
{
#ifdef DEBUG
	
	printf( "config.ttl=%d\n", config.ttl);
	printf( "config.port=%d\n", config.port);
	printf( "config.ip=%s\n", config.ip);
	printf( "config.timeout=%d\n", config.timeout);

	
	printf( "shout_version %s\n\n", shout_version(NULL, NULL, NULL) );
	printf( "shout_ip '%s'\n", shout_get_host(shout) );
	printf( "shout_port '%i'\n", shout_get_port(shout) );
	printf( "shout_password '%s'\n", shout_get_password(shout) );
	printf( "shout_mount '%s'\n", shout_get_mount(shout) );
	
	printf( "shout_name '%s'\n", shout_get_name(shout) );
	printf( "shout_url '%s'\n", shout_get_url(shout) );
	printf( "shout_genre '%s'\n", shout_get_genre(shout) );
	printf( "shout_user '%s'\n", shout_get_user(shout) );
	printf( "shout_agent '%s'\n", shout_get_agent(shout) );
	printf( "shout_description '%s'\n", shout_get_description(shout) );
	
	printf( "shout_dumpfile '%s'\n", shout_get_dumpfile(shout) );
	printf( "shout_audioinfo - bitrate - '%s'\n", shout_get_audio_info(shout, SHOUT_AI_BITRATE) );
	printf( "shout_audioinfo - samplerate - '%s'\n", shout_get_audio_info(shout, SHOUT_AI_SAMPLERATE) );
	printf( "shout_audioinfo - channels - '%s'\n", shout_get_audio_info(shout, SHOUT_AI_CHANNELS) );
	printf( "shout_audioinfo - quality - '%s'\n", shout_get_audio_info(shout, SHOUT_AI_QUALITY) );
	printf( "shout_is_public '%d'\n", shout_get_public(shout) );
	
	printf( "shout_format " );
	switch(shout_get_format(shout)) {
		case SHOUT_FORMAT_VORBIS: printf("VORBIS\n"); break;
		case SHOUT_FORMAT_MP3: printf("MP3\n"); break;
		default: printf("%d\n", shout_get_format(shout)); break;
	}

	printf( "shout_protocol " );
	switch(shout_get_protocol(shout)) {
		case SHOUT_PROTOCOL_HTTP: printf("HTTP\n"); break;
		case SHOUT_PROTOCOL_XAUDIOCAST: printf("XAUDIOCAST\n"); break;
		case SHOUT_PROTOCOL_ICY: printf("ICY\n"); break;
		default: printf("%d\n", shout_get_protocol(shout)); break;
	}
	
#ifdef USE_LAME
	printf( "lame_version %s\n", get_lame_version() );
	printf( "lame_samplerate %i\n", lame_get_in_samplerate(lame) );
	printf( "lame_channels %i\n", lame_get_num_channels(lame) );
	printf( "lame_bitrate %i\n", lame_get_brate(lame) );
	printf( "lame_mode %i\n", lame_get_mode(lame) );
	printf( "lame_quality %i\n", lame_get_quality(lame) );
#endif


#ifdef USE_VORBIS
	printf( "vorbis_quality %2.2f\n", vorbis.quality );
	printf( "vorbis_downmix %d\n", vorbis.downmix );
#endif

	
#endif
}



void shout_config_audio_format( char* value )
{
	if (!strcasecmp(value, "mp3"))
		shout_set_format(shout, SHOUT_FORMAT_MP3);
	else if (!strcasecmp(value, "vorbis"))
		shout_set_format(shout, SHOUT_FORMAT_VORBIS);
	else {
		fprintf(stderr, "audio_format: unknown format: %s [mp3, vorbis]\n", value);
		exit(1);
	}
}


void shout_config_mp3_bitrate( char* value )
{

#ifdef USE_LAME
	lame_set_brate(lame,atoi(value));
#endif

}


void shout_config_mp3_mode( char* value )
{

#ifdef USE_LAME
	if (!strcasecmp(value, "mono"))
		lame_set_mode(lame, MONO);
	else if (!strcasecmp(value, "stereo"))
		lame_set_mode(lame, STEREO);
	else if (!strcasecmp(value, "joint"))
		lame_set_mode(lame, JOINT_STEREO);
	else {
		fprintf(stderr, "audio_mode: unknown mode for MP3: %s [joint, stereo, mono]\n", value);
		exit(1);
	}
#endif

}


void shout_config_mp3_quality( char* value )
{

#ifdef USE_LAME
	lame_set_quality(lame, atoi(value));
#endif

}


void shout_config_vorbis_quality( char* value )
{

#ifdef USE_VORBIS
	vorbis.quality = (float)atof(value);
#endif

}

void shout_config_vorbis_downmix( int value )
{

#ifdef USE_VORBIS
	vorbis.downmix = value;
#endif
}


void shout_set_vorbis_defaults( )
{
#ifdef USE_VORBIS

	// Default quality - about 128 kbps VBR
	vorbis.quality = 4.0;

	// Don't Downmix by default
	vorbis.downmix = 0;
#endif
}


void shout_send_ogg_packet()
{
#ifdef USE_VORBIS
	int shout_code;
	
	// Send Ogg packet header
	shout_code = shout_send(shout, vorbis.og.header, vorbis.og.header_len);
	if (shout_code != SHOUTERR_SUCCESS) {
		fprintf(stderr,"Failed to send Ogg Header to libshout's server.\n");
	}
	
	// Send Ogg packet body
	shout_code = shout_send(shout, vorbis.og.body, vorbis.og.body_len);
	if (shout_code != SHOUTERR_SUCCESS) {
		fprintf(stderr,"Failed to send Ogg Body to libshout's server.\n");
	}
	
	
	/* Verify we are still connected */
	if (shout_get_connected( shout ) != SHOUTERR_CONNECTED) {
		fprintf(stderr,"Lost connection to libshout's server: %s\n", shout_get_error(shout));
		exit(1);
	}
	
#endif
}


void shout_initialise_vorbis( audio_t* audio )
{
#ifdef USE_VORBIS
	char audio_info[MAX_STR_LEN];
	int channels;

	audio->write_callback = shout_encode_vorbis_callback;
	audio->close_callback = shout_close_vorbis_callback;


	/* Setup encoder */
	vorbis_info_init(&vorbis.vi);

	if (vorbis.downmix) channels = 1;
	else channels = audio->channels;
	
	if (vorbis_encode_init_vbr(&vorbis.vi, channels,
								audio->samplerate, vorbis.quality*0.1)) {
		fprintf(stderr, "Failed to initialise VBR encoding:\n"); 
		fprintf(stderr, "  channels=%d\n  samplerate=%d\n  quality=%f\n",
			channels, audio->samplerate, vorbis.quality); 
		exit(1);
	}
	
	
	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&vorbis.vd, &vorbis.vi);
    vorbis_block_init(&vorbis.vd, &vorbis.vb);
	
	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build
	 chained streams just by concatenation */
	srand(time(NULL));
	ogg_stream_init(&vorbis.os, rand());
	
	
	/* Vorbis streams begin with three headers; the initial header (with
	   most of the codec setup parameters) which is mandated by the Ogg
	   bitstream spec.  The second header holds any comment fields.  The
	   third header holds the bitstream codebook.  We merely need to
	   make the headers, then pass them to libvorbis one at a time;
	   libvorbis handles the additional Ogg bitstream constraints */
	
	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;
		
		vorbis_analysis_headerout(&vorbis.vd,&vorbis.vc,&header,&header_comm,&header_code);
		ogg_stream_packetin(&vorbis.os,&header); /* automatically placed in its own page */
		ogg_stream_packetin(&vorbis.os,&header_comm);
		ogg_stream_packetin(&vorbis.os,&header_code);
		
		vorbis.in_header = 1;
	}

	
	/* tell libshout about audio settings too */
	snprintf( audio_info, MAX_STR_LEN, "%2.2f", vorbis.quality );
	shout_set_audio_info( shout, SHOUT_AI_QUALITY, audio_info );
	
	snprintf( audio_info, MAX_STR_LEN, "%d", audio->samplerate );
	shout_set_audio_info( shout, SHOUT_AI_SAMPLERATE, audio_info );
	
	snprintf( audio_info, MAX_STR_LEN, "%d", channels );
	shout_set_audio_info( shout, SHOUT_AI_CHANNELS, audio_info );
	

	fprintf(stderr, "shout_initialise_vorbis\n");
#endif
}



void shout_encode_vorbis_callback( audio_t* audio, int raw_bytes )
{
#ifdef USE_VORBIS
	int samples = raw_bytes / audio->channels / 2;
	short *int_buf = (short*)audio->buffer;
	float **buffer;
	int s,c, result;
    ogg_packet op;
	
	
	/* Send out Vorbis headers to server */
	if (vorbis.in_header) {
		
		while(ogg_stream_flush(&vorbis.os, &vorbis.og)) {
			shout_send_ogg_packet();
		}
		vorbis.in_header = 0;
	}
	

	/* expose the buffer to submit data */
	buffer = vorbis_analysis_buffer(&vorbis.vd, samples);

	if (vorbis.downmix && audio->channels == 2) {	
		// Downmix from Stero to Mono (average the channels)
		for(s=0; s < samples; s++)
		{
			buffer[0][s] = ((int)( int_buf[s*2] + int_buf[s*2+1])/2)
					/ 32768.f;
		}
	} else {
	for(s=0; s < samples; s++) 
	for(c=0; c < audio->channels; c++)
	{
		buffer[c][s] = int_buf[s*audio->channels + c] / 32768.f;
	}
	}
	
	vorbis_analysis_wrote(&vorbis.vd, samples);


	do {
        while(vorbis_analysis_blockout(&vorbis.vd, &vorbis.vb)==1)
        {
            vorbis_analysis(&vorbis.vb, NULL);
            vorbis_bitrate_addblock(&vorbis.vb);

            while(vorbis_bitrate_flushpacket(&vorbis.vd, &op)) 
                ogg_stream_packetin(&vorbis.os, &op);
        }

		result = ogg_stream_pageout(&vorbis.os, &vorbis.og);
		if (result) shout_send_ogg_packet();
		
	} while(result > 0);

#endif
}


void shout_close_vorbis_callback( audio_t* audio )
{
#ifdef USE_VORBIS
	ogg_packet op;
    vorbis_analysis_wrote(&vorbis.vd, 0);

    while(vorbis_analysis_blockout(&vorbis.vd, &vorbis.vb)==1)
    {
        vorbis_analysis(&vorbis.vb, NULL);
        vorbis_bitrate_addblock(&vorbis.vb);
        while(vorbis_bitrate_flushpacket(&vorbis.vd, &op))
            ogg_stream_packetin(&vorbis.os, &op);
    }
    
	
	ogg_stream_clear(&vorbis.os);
	vorbis_block_clear(&vorbis.vb);
	vorbis_dsp_clear(&vorbis.vd);
	vorbis_info_clear(&vorbis.vi);
#endif
}




void shout_set_lame_defaults( )
{
#ifdef USE_LAME
	// Create a LAME instance 
	if (!(lame = lame_init())) {
		fprintf(stderr, "pcm6shout: Failed to initalise LAME.\n");
		exit(1);
	}
	
	/* Lame Defaults */
	lame_set_brate(lame, 128);
	lame_set_in_samplerate(lame, 44100);
	lame_set_num_channels(lame, 2);
	lame_set_quality(lame, 3);
	lame_set_mode(lame, JOINT_STEREO);
#endif
}


void shout_initialise_lame( audio_t* audio )
{
#ifdef USE_LAME
	char audio_info[MAX_STR_LEN];

	audio->write_callback = shout_encode_lame_callback;
	audio->close_callback = shout_close_lame_callback;


	/* Sanity Fix */
	if (audio->channels == 1) lame_set_mode(lame, MONO);

	/* transfer audio settings to lame */
	lame_set_in_samplerate(lame, audio->samplerate);
	lame_set_num_channels(lame, audio->channels);


	/* Prepare lame */
	if (lame_init_params(lame)) {
		fprintf(stderr, "pcm6shout: Failed to prepare LAME.\n");
		exit(1);
	}



	/* tell libshout about audio settings too */
	snprintf( audio_info, MAX_STR_LEN, "%d", lame_get_brate(lame) );
	shout_set_audio_info( shout, SHOUT_AI_BITRATE, audio_info );
	
	snprintf( audio_info, MAX_STR_LEN, "%d", lame_get_out_samplerate(lame) );
	shout_set_audio_info( shout, SHOUT_AI_SAMPLERATE, audio_info );
	
	/* Send the number of channels on the encoded stream */
	if (lame_get_mode(lame) == MONO) 	snprintf( audio_info, MAX_STR_LEN, "1" );
	else 								snprintf( audio_info, MAX_STR_LEN, "2" );
	shout_set_audio_info( shout, SHOUT_AI_CHANNELS, audio_info );




	// Allocate memory for buffer
	encoded_buffer = malloc( ENCODED_BUFFER_SIZE );
	if (!encoded_buffer) {
		fprintf(stderr, "pcm6shout: Failed to allocate Encoded Audio memory buffer.\n");
		exit(1);
	}
#endif
}




void shout_encode_lame_callback( audio_t* audio, int raw_bytes )
{
#ifdef USE_LAME
	int encoded_size=0, shout_code=0;
	int raw_samples = raw_bytes / audio->channels / 2;
	
	// Encode the audio
	if (audio->channels == 1) {
		encoded_size = lame_encode_buffer(
			lame, (short*)audio->buffer, (short*)audio->buffer,
			raw_samples, encoded_buffer, ENCODED_BUFFER_SIZE );
	} else {
		encoded_size = lame_encode_buffer_interleaved(
			lame, (short*)audio->buffer,
			raw_samples, encoded_buffer, ENCODED_BUFFER_SIZE );
	}
	
	if (encoded_size < 0) {
		fprintf(stderr, "pcm6shout: LAME Failed (%d) buffersize=%d.\n", encoded_size, ENCODED_BUFFER_SIZE);
		exit(-2);
	}
	
	/* Do we have some MP3 ? */
	if (encoded_size > 0) {

		// Send the encoded data to the Shoutcast server 
		shout_code = shout_send(shout, encoded_buffer, encoded_size);
		if (shout_code != SHOUTERR_SUCCESS) {
			fprintf(stderr,"Failed to send encoded audio to libshout's server.\n");
		}
			
		/* Not sure if this is needed ? 
		   (live audio should be being read in at correct rate) */
		shout_sync(shout);
	}
	
	
	/* Verify we are still connected */
	if (shout_get_connected( shout ) != SHOUTERR_CONNECTED) {
		fprintf(stderr,"Lost connection to libshout's server: %s\n", shout_get_error(shout));
		exit(1);
	}
#endif
}



void shout_close_lame_callback( audio_t* audio )
{
#ifdef USE_LAME
	if (encoded_buffer) free(encoded_buffer);
#endif
}

