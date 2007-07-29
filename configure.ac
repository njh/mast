
dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mast], [0.2.3], [njh@ecs.soton.ac.uk])

AC_CONFIG_SRCDIR(src/mast.h)
AC_CONFIG_AUX_DIR(build) 
AC_CANONICAL_SYSTEM

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE(1.7)
AM_CONFIG_HEADER(src/config.h)



dnl ############# Compiler and tools Checks

AC_PROG_CC
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

AC_PATH_PROG( [ASCIIDOC], [asciidoc] )
AC_SUBST( POD2MAN )

AC_PATH_PROG( [XMLTO], [xmlto] )
AC_SUBST( XMLTO )

if test "x$ac_cv_path_ASCIIDOC" != "x" -a "x$ac_cv_path_XMLTO" != "x"; then
	BUILD_DOC="Yes"
else
	BUILD_DOC="No"
	AC_MSG_WARN([Not re-building documentation because tools are missing])
fi
AM_CONDITIONAL(BUILD_DOC, test x$BUILD_DOC = xYes)



dnl ############## Library Checks

dnl Check for ortp (required)
PKG_CHECK_MODULES(oRTP, ortp >= 0.13.1, ,
	[ AC_MSG_ERROR(
		[ Version 0.13.1 or greater of the oRTP library is required to build MAST.
		It can be downloaded from http://www.linphone.org/ortp/ ])
	]
)

dnl Check for Secret Rabbit Code (it is optional)
PKG_CHECK_MODULES(libsamplerate, samplerate >= 0.1.2,
	[ HAVE_LIBSRC="Yes" ],
	[ HAVE_LIBSRC="No" ]
)

dnl Check for libsndfile (it is optional)
PKG_CHECK_MODULES(sndfile, sndfile >= 1.0.0,
	[ HAVE_SNDFILE="Yes" ],
	[ HAVE_SNDFILE="No" ]
)

dnl Check for JACK (it is optional)
PKG_CHECK_MODULES(JACK, jack,
	[ HAVE_JACK="Yes" ],
	[ HAVE_JACK="No" ]
)

dnl Check for twolame (it is optional)
PKG_CHECK_MODULES(TwoLAME, twolame >= 0.3.10,
	[
		HAVE_TWOLAME="Yes"
		AC_DEFINE_UNQUOTED( HAVE_TWOLAME, 1, [ Define if libtwolame is available ] )
	],
	[ HAVE_TWOLAME="No" ]
)
AM_CONDITIONAL([HAVE_TWOLAME], [test x$HAVE_TWOLAME = xYes])




dnl ############## Header Checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h malloc.h])



dnl ############## Type checks

AC_CHECK_TYPE(u_int32_t, unsigned long)
AC_CHECK_TYPE(u_int16_t, unsigned short)
AC_CHECK_TYPE(u_int8_t, unsigned char)



dnl ############## Compiler and Linker Flags
CFLAGS="$CFLAGS $ORTP_CFLAGS"
LDFLAGS="$LDFLAGS $ORTP_LIBS"

if test x$Debugging = xEnabled; then
	CFLAGS="$CFLAGS -g -Wunused -Wall -Werror"
else
	CFLAGS="$CFLAGS -O2"
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

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile libgsm/Makefile])

AC_OUTPUT



dnl ############## Display Message

echo "
  Multicast Audio Streaming Toolkit $VERSION

  Install path ........... $prefix
  Rebuilding Docs ........ $BUILD_DOC
  Debugging .............. $Debugging
  Jack Support ........... $HAVE_JACK
  Libsamplerate support .. $HAVE_LIBSRC
  Libsndfile support ..... $HAVE_SNDFILE
  MPEG Audio Support ..... $HAVE_TWOLAME
  
Next type 'make' and then 'make install'."

