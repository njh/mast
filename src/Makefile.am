
bin_PROGRAMS = mast-sap-client mast-recorder

mast_sap_client_SOURCES = \
	sap-client.c \
	utils.c \
	socket.c \
	sap.c \
	sdp.c \
	mast.h

mast_recorder_SOURCES = \
	recorder.c \
	utils.c \
	rtp.c \
	socket.c \
	sdp.c \
	writer.c \
	bytestoint.h \
	mast.h

mast_recorder_CFLAGS = @SNDFILE_CFLAGS@
mast_recorder_LDADD = @SNDFILE_LIBS@
