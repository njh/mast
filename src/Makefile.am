
EXTRA_PROGRAMS = pcm6cast pcm6client
bin_PROGRAMS = @BIN_TO_BUILD@


pcm6cast_SOURCES = \
	pcm6cast.c \
	pcm6cast.h \
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
	
pcm6cast_CFLAGS = @USE_SNDFILE@ @USE_OSS@ @SNDFILE_CFLAGS@
pcm6cast_LDFLAGS = @SNDFILE_LIBS@



pcm6client_SOURCES = \
	pcm6client.c \
	pcm6cast.h \
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

pcm6client_CFLAGS = @USE_SNDFILE@ @USE_SDL@ @USE_OSS@ @SNDFILE_CFLAGS@ @SDL_CFLAGS@
pcm6client_LDFLAGS = @SNDFILE_LIBS@ @SDL_LIBS@
