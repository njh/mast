
dnl Require autoconf version >= 2.71
AC_PREREQ([2.71])


dnl ############# Initialisation

AC_INIT([mast],[2.0.0],[njh@aelius.com])

AC_CONFIG_SRCDIR(src/mast.h)
AC_CONFIG_AUX_DIR(build) 
AC_CANONICAL_TARGET

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE(1.7)
AM_CONFIG_HEADER(src/config.h)



dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_C_BIGENDIAN
AC_C_INLINE


dnl ############# Configure Arguments

AC_ARG_ENABLE(debug,
              [--enable-debug=[no/yes] turn on debugging],
              [ Debugging="Enabled" 
                AC_DEFINE(DEBUGGING, 1, [ Define if debugging is enabled. ]) ],
              [ Debugging="Disabled" ]
)



dnl ############## Check for tools to build documentation

AC_PATH_PROG( [ASCIIDOC], [asciidoctor] )
AC_SUBST([ASCIIDOC])

AC_PATH_PROG( [XMLTO], [xmlto] )
AC_SUBST([XMLTO])

if test "x$ac_cv_path_ASCIIDOC" != "x" -a "x$ac_cv_path_XMLTO" != "x"; then
  BUILD_DOC="Yes"
else
  BUILD_DOC="No"
  AC_MSG_WARN([Not re-building documentation because tools are missing])
fi
AM_CONDITIONAL(BUILD_DOC, test x$BUILD_DOC = xYes)



dnl ############## Library Checks

AC_CHECK_LIB([m], [log10f])
AC_CHECK_LIB([mx], [log10f])

dnl Check for libsndfile (it is optional)
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.0,
  [ HAVE_SNDFILE="Yes" ],
  [ HAVE_SNDFILE="No" ]
)

PKG_CHECK_MODULES(CHECK, check >= 0.9.4, have_check="yes", have_check="no")
if test x"$have_check" = "xyes"; then
  AC_CHECK_PROG(have_checkmk, [checkmk], [yes], [no])
  if test x"$have_checkmk" = "xyes"; then
    AC_DEFINE([HAVE_CHECK], 1, [Define to 1 if check library is available])
	else
    AC_MSG_WARN([Command 'checkmk' not found.])
    AC_MSG_WARN([Download it here: http://micah.cowan.name/projects/checkmk/])
  fi
fi
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes" &&
                           test x"$have_checkmk" = "xyes")


dnl ############## Header Checks

AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h malloc.h])



dnl ############## Type checks

AC_CHECK_TYPE(u_int32_t, unsigned long)
AC_CHECK_TYPE(u_int16_t, unsigned short)
AC_CHECK_TYPE(u_int8_t, unsigned char)



dnl ############## Compiler and Linker Flags

if test x$Debugging = xEnabled; then
  CFLAGS="$CFLAGS -g -Wunused -Wall -Werror"
  CXXFLAGS="$CXXFLAGS -g -Wunused -Wall -Werror"
else
  CFLAGS="$CFLAGS -O2"
  CXXFLAGS="$CXXFLAGS -O2"
fi



dnl ############## Decide what to build

BUILD_PROGRAMS="mast_info mast_rawcast mast_rawrecord"

if test x$HAVE_JACK = "xYes" -a x$HAVE_LIBSRC = "xYes"; then
  BUILD_PROGRAMS="$BUILD_PROGRAMS mast_cast"
fi

if test x$HAVE_SNDFILE = "xYes" -a x$HAVE_LIBSRC = "xYes"; then
  BUILD_PROGRAMS="$BUILD_PROGRAMS mast_filecast mast_record"
fi

AC_SUBST(BUILD_PROGRAMS)



dnl ############## Final Output

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])

AC_OUTPUT



dnl ############## Display Message

echo "
  Multicast Audio Streaming Toolkit $VERSION

  Install path ........... $prefix
  Rebuilding Docs ........ $BUILD_DOC
  Debugging .............. $Debugging
  
Next type 'make' and then 'make install'."

