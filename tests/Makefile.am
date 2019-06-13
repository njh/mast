check_PROGRAMS = 10_check_utils.cmd
TESTS = $(check_PROGRAMS)

.tc.c:
	checkmk $< > $@ || rm -f $@

10_check_utils_cmd_SOURCES = 10_check_utils.c
10_check_utils_cmd_CFLAGS = @CHECK_CFLAGS@
10_check_utils_cmd_LDADD = @CHECK_LIBS@

CLEANFILES = 10_check_utils.c
