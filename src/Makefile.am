
bin_PROGRAMS = @BUILD_PROGRAMS@

EXTRA_PROGRAMS = \
	mast_info \
	mast_rawcast \
	mast_rawrecord \
	mast_cast \
	mast_filecast \
	mast_record


noinst_LIBRARIES = libmastcodecs.a
libmastcodecs_a_SOURCES = \
	codecs.h \
	codecs.c \
	codec_gsm.c \
	codec_lpc.c \
	codec_l16.c \
	codec_mpa.c \
	codec_pcma.c \
	codec_pcmu.c

MAST_CODEC_LIBS = \
	libmastcodecs.a \
	-ltwolame \
	../libgsm/libgsm.a


mast_info_SOURCES = \
	mast_info.c \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c


mast_rawcast_SOURCES = \
	mast_rawcast.c \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c


mast_rawrecord_SOURCES = \
	mpa_header.c \
	mpa_header.h \
	mast_rawrecord.c \
	mast.h \
	util.c


mast_cast_CFLAGS = @JACK_CFLAGS@
mast_cast_LDADD = $(MAST_CODEC_LIBS) @JACK_LIBS@
mast_cast_SOURCES = \
	mast_cast.c \
	mast_cast-jack.c \
	mast_cast-jack.h \
	mime_type.c \
	mime_type.h \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c


mast_filecast_CFLAGS = @SNDFILE_CFLAGS@
mast_filecast_LDADD = $(MAST_CODEC_LIBS) @SNDFILE_LIBS@
mast_filecast_SOURCES = \
	mast_filecast.c \
	mime_type.c \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c


mast_record_CFLAGS = @SNDFILE_CFLAGS@
mast_record_LDADD = $(MAST_CODEC_LIBS) @SNDFILE_LIBS@
mast_record_SOURCES = \
	mast_record.c \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c

