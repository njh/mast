AM_CFLAGS = -I$(top_srcdir)/src $(CHECK_CFLAGS)
AM_LDFLAGS = $(CHECK_LIBS)

check_PROGRAMS = 10_check_utils.cmd
TESTS = $(check_PROGRAMS)

.tc.c:
	checkmk $< > $@ || rm -f $@

10_check_utils_cmd_SOURCES = 10_check_utils.c

CLEANFILES = 10_check_utils.c
