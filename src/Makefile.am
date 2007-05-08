
bin_PROGRAMS = @BUILD_PROGRAMS@

EXTRA_PROGRAMS = \
	mast_listen \
	mast_cast \
	mast_filecast \
	mast_rawcast \
	mast_record \
	mast_rawrecord


mast_filecast_CFLAGS = @SNDFILE_CFLAGS@
mast_filecast_LDADD = @SNDFILE_LIBS@
mast_filecast_SOURCES = \
	mast_filecast.c \
	avprofile_extras.c \
	payloadtype.c \
	util.c



mast_rawrecord_SOURCES = \
	gethostname.c \
	mpa_header.c \
	mast_rawrecord.c

mast_record_SOURCES = \
	gethostname.c \
	mast_record.c

mast_rawcast_SOURCES = \
	gethostname.c \
	mast_rawcast.c \
	mpa_header.c \
	mpa_header.h \
	util.c

mast_cast_CFLAGS = @JACK_CFLAGS@
mast_cast_LDADD = @JACK_LIBS@
mast_cast_SOURCES = \
	mastcast.c \
	mast_config.h \
	mast_config.c \
	rtp.c \
	rtp.h \
	audio.c \
	audio.h \
	audio_file.c \
	audio_oss.c \
	audio_stdio.c \
	codec_ulaw.c \
	codec_alaw.c \
	codec_l16.c \
	codecs.h \
	random32.c \
	random32.h \
	md5c.c \
	md5.h \
	global.h
	
mast_listen_SOURCES = \
	mastclient.c \
	mast_config.h \
	mast_config.c \
	rtp.c \
	rtp.h \
	audio.c \
	audio.h \
	audio_oss.c \
	audio_file.c \
	audio_stdio.c \
	codec_ulaw.c \
	codec_alaw.c \
	codec_l16.c \
	codecs.h \
	random32.c \
	random32.h \
	md5c.c \
	md5.h \
	global.h

