
bin_PROGRAMS = @BUILD_PROGRAMS@

EXTRA_PROGRAMS = \
	mast_rawcast \
	mast_rawrecord \
	mast_cast \
	mast_listen \
	mast_filecast \
	mast_record


mast_rawcast_SOURCES = \
	mast_rawcast.c \
	mpa_header.c \
	mpa_header.h \
	mast.h \
	util.c


mast_rawrecord_SOURCES = \
	mpa_header.c \
	mast_rawrecord.c \
	mast.h \
	util.c


mast_cast_CFLAGS = @JACK_CFLAGS@
mast_cast_LDADD = @JACK_LIBS@
mast_cast_SOURCES = \
	mast_cast.c \
	avprofile_extras.c \
	payloadtype.c \
	mast.h \
	util.c


mast_listen_CFLAGS = @JACK_CFLAGS@
mast_listen_LDADD = @JACK_LIBS@
mast_listen_SOURCES = \
	mast_listen.c \
	mast.h \
	util.c


mast_filecast_CFLAGS = @SNDFILE_CFLAGS@
mast_filecast_LDADD = @SNDFILE_LIBS@
mast_filecast_SOURCES = \
	mast_filecast.c \
	avprofile_extras.c \
	payloadtype.c \
	mast.h \
	util.c


mast_record_CFLAGS = @SNDFILE_CFLAGS@
mast_record_LDADD = @SNDFILE_LIBS@
mast_record_SOURCES = \
	mast_record.c \
	mast.h \
	util.c
