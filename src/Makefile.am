
bin_PROGRAMS = mast-info mast-sap-client mast-sap-server mast-recorder

mast_info_SOURCES = \
	info.c \
	utils.c \
	rtp.c \
	socket.c \
	sdp.c \
	mast.h

mast_sap_client_SOURCES = \
	sap-client.c \
	utils.c \
	socket.c \
	sap.c \
	sdp.c \
	mast.h

mast_sap_server_SOURCES = \
	sap-server.c \
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
