check_PROGRAMS = \
  10_check_utils.cmd \
  20_check_rtp.cmd \
  20_check_sap.cmd \
  20_check_sdp.cmd

TESTS = $(check_PROGRAMS)

.tc.c:
	checkmk $< > $@ || rm -f $@

10_check_utils_cmd_SOURCES = \
  10_check_utils.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
10_check_utils_cmd_CFLAGS = @CHECK_CFLAGS@
10_check_utils_cmd_LDADD = @CHECK_LIBS@

20_check_rtp_cmd_SOURCES = \
  20_check_rtp.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/rtp.c \
  $(top_srcdir)/src/socket.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
20_check_rtp_cmd_CFLAGS = @CHECK_CFLAGS@
20_check_rtp_cmd_LDADD = @CHECK_LIBS@

20_check_sap_cmd_SOURCES = \
  20_check_sap.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/sap.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
20_check_sap_cmd_CFLAGS = @CHECK_CFLAGS@
20_check_sap_cmd_LDADD = @CHECK_LIBS@

20_check_sdp_cmd_SOURCES = \
  20_check_sdp.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/sdp.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
20_check_sdp_cmd_CFLAGS = @CHECK_CFLAGS@
20_check_sdp_cmd_LDADD = @CHECK_LIBS@

CLEANFILES = \
  $(check_PROGRAMS:%.cmd=%.c) \
  $(check_PROGRAMS:%.cmd=%.log) \
  $(check_PROGRAMS:%.cmd=%.trs)
