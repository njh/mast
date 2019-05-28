
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
	MastAudioBuffer.cpp \
	MastAudioBuffer.h \
	MastMimeType.cpp \
	MastMimeType.h \
	MastCodec.cpp \
	MastCodec.h \
	MastCodec_GSM.cpp \
	MastCodec_GSM.h \
	MastCodec_L16.cpp \
	MastCodec_L16.h \
	MastCodec_LPC.cpp \
	MastCodec_LPC.h \
	MastCodec_MPA.cpp \
	MastCodec_MPA.h \
	MastCodec_PCMU.cpp \
	MastCodec_PCMU.h \
	MastCodec_PCMA.cpp \
	MastCodec_PCMA.h

MAST_CODEC_LIBS = \
	libmastcodecs.a \
	-lsamplerate

if HAVE_TWOLAME
MAST_CODEC_LIBS += -ltwolame
endif

if HAVE_GSM
MAST_CODEC_LIBS += -lgsm
endif


mast_info_SOURCES = \
	mast_info.cpp \
	MastTool.cpp \
	MastTool.h \
	MastMimeType.cpp \
	MastMimeType.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp


mast_rawcast_SOURCES = \
	mast_rawcast.cpp \
	MastTool.cpp \
	MastTool.h \
	MastMimeType.cpp \
	MastMimeType.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp


mast_rawrecord_SOURCES = \
	mast_rawrecord.cpp \
	MastTool.cpp \
	MastTool.h \
	MastMimeType.cpp \
	MastMimeType.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp


mast_cast_CFLAGS = @JACK_CFLAGS@
mast_cast_LDADD = $(MAST_CODEC_LIBS) @JACK_LIBS@
mast_cast_SOURCES = \
	mast_cast.cpp \
	MastTool.cpp \
	MastTool.h \
	MastSendTool.cpp \
	MastSendTool.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp


mast_filecast_CFLAGS = @SNDFILE_CFLAGS@
mast_filecast_LDADD = $(MAST_CODEC_LIBS) @SNDFILE_LIBS@
mast_filecast_SOURCES = \
	mast_filecast.cpp \
	MastTool.cpp \
	MastTool.h \
	MastSendTool.cpp \
	MastSendTool.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp


mast_record_CFLAGS = @SNDFILE_CFLAGS@
mast_record_LDADD = $(MAST_CODEC_LIBS) @SNDFILE_LIBS@
mast_record_SOURCES = \
	mast_record.cpp \
	MastTool.cpp \
	MastTool.h \
	MPA_Header.cpp \
	MPA_Header.h \
	mast.h \
	util.cpp

