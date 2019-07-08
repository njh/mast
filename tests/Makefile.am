AM_CFLAGS = @CHECK_CFLAGS@ \
            -DFIXTURE_DIR=\"$(srcdir)/fixtures/\" \
            -I$(top_srcdir)/src

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
10_check_utils_cmd_LDADD = @CHECK_LIBS@

20_check_rtp_cmd_SOURCES = \
  20_check_rtp.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/rtp.c \
  $(top_srcdir)/src/socket.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/bytestoint.h \
  $(top_srcdir)/src/mast.h
20_check_rtp_cmd_LDADD = @CHECK_LIBS@

20_check_sap_cmd_SOURCES = \
  20_check_sap.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/sap.c \
  $(top_srcdir)/src/socket.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
20_check_sap_cmd_LDADD = @CHECK_LIBS@

20_check_sdp_cmd_SOURCES = \
  20_check_sdp.c \
  hext.c \
  hext.h \
  $(top_srcdir)/src/sdp.c \
  $(top_srcdir)/src/utils.c \
  $(top_srcdir)/src/mast.h
20_check_sdp_cmd_LDADD = @CHECK_LIBS@

EXTRA_DIST = \
  fixtures/aes67-multicast-example.sdp \
  fixtures/dante-aes67-1.sdp \
  fixtures/livewire-stl.sdp \
  fixtures/rfc7273-example-4.8.1.sdp \
  fixtures/rtp_mini_packet.hext \
  fixtures/sap_minimal_compressed.hext \
  fixtures/sap_minimal_encrypted.hext \
  fixtures/sap_minimal_ipv6_origin.hext \
  fixtures/sap_minimal_no_mime.hext \
  fixtures/sap_minimal_unknown_mime.hext \
  fixtures/sap_minimal_valid.hext \
  fixtures/sap_too_short.hext \
  fixtures/sap_wrong_version.hext \
  fixtures/test.txt \
  fixtures/xnode-l24-48000-2.sdp

CLEANFILES = \
  $(check_PROGRAMS:%.cmd=%.c) \
  $(check_PROGRAMS:%.cmd=%.log) \
  $(check_PROGRAMS:%.cmd=%.trs)
