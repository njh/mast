
EXTRA_PROGRAMS = mast mastclient
bin_PROGRAMS = @BIN_TO_BUILD@


mastcast_SOURCES = \
	mastcast.c \
	mastcast.h \
	mast.h \
	udp6_socket.c \
	udp_socket.h \
	rtp.c \
	rtp.h \
	audio.c \
	audio.h \
	audio_file.c \
	audio_oss.c \
	audio_sdl.c \
	audio_stdio.c \
	codec_ulaw.c \
	codec_alaw.c \
	codec_l16.c \
	codecs.h \
	random32.c \
	random32.h \
	md5c.c \
	md5.h \
	global.h \
	sfifo.h \
	sfifo.c
	
mastcast_CFLAGS = @USE_SNDFILE@ @USE_OSS@ @SNDFILE_CFLAGS@
mastcast_LDFLAGS = @SNDFILE_LIBS@



mastclient_SOURCES = \
	mastclient.c \
	mastclient.h \
	mastcast.h \
	client.c \
	udp6_socket.c \
	udp_socket.h \
	rtp.c \
	rtp.h \
	audio.c \
	audio.h \
	audio_oss.c \
	audio_file.c \
	audio_sdl.c \
	audio_stdio.c \
	codec_ulaw.c \
	codec_alaw.c \
	codec_l16.c \
	codecs.h \
	random32.c \
	random32.h \
	md5c.c \
	md5.h \
	global.h \
	sfifo.h \
	sfifo.c

mastclient_CFLAGS = @USE_SNDFILE@ @USE_SDL@ @USE_OSS@ @SNDFILE_CFLAGS@ @SDL_CFLAGS@
mastclient_LDFLAGS = @SNDFILE_LIBS@ @SDL_LIBS@
