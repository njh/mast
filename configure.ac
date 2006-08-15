
dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mast], [0.2a], [njh@ecs.soton.ac.uk])

AC_CONFIG_SRCDIR(src/mastcast.c)
AC_CONFIG_AUX_DIR(build) 
AC_CANONICAL_SYSTEM

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE(1.7)
AM_CONFIG_HEADER(build/config.h)



dnl ############# Compiler and tools Checks

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_BIGENDIAN
AC_C_INLINE



dnl ############# Configure Arguments

AC_ARG_ENABLE(debug,
              [--enable-debug=[no/yes] turn on debugging],
              [ Debugging="Enabled" 
                AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ]) ],
              [ Debugging="Disabled" ]
)



dnl ############## Check for tools to build documentation

AC_PATH_PROG( [POD2MAN], [pod2man] )
AC_SUBST( POD2MAN )

AC_PATH_PROG( [POD2TEXT], [pod2text]  )
AC_SUBST( POD2TEXT )


dnl ############## Library Checks

dnl Check for libsndfile
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.0, ,
	[ AC_MSG_ERROR(
		[ Version 1.0.0 or greater of libsndfile is required.
		It can be downloaded from http://www.mega-nerd.com/libsndfile/ ])
	]
)

dnl Check for ortp
PKG_CHECK_MODULES(ORTP, ortp >= 0.9.1, ,
	[ AC_MSG_ERROR(
		[ Version 0.9.1 or greater of the oRTP library is required to build MAST.
		It can be downloaded from http://www.linphone.org/ortp/ ])
	]
)

dnl Check for JACK
dnl PKG_CHECK_MODULES(JACK, jack,
dnl 	[ JACKsupport="Enabled"
dnl 	  AC_DEFINE(HAVE_JACK, 1, [ Define if JACK audio server is available ]) ],
dnl 	[ JACKsupport="Disabled" ]
dnl )



dnl ############## Header Checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h])

dnl Check for Linux (OSS) sound support
AC_CHECK_HEADER(sys/soundcard.h, 
		[ OSSsupport="Enabled"
		  AC_DEFINE(HAVE_OSS, 1, [ Define if OSS sound support is available ]) ],
		[ OSSsupport="Disabled" ]
)



dnl ############## Type checks

AC_CHECK_TYPE(u_int32_t, unsigned long)
AC_CHECK_TYPE(u_int16_t, unsigned short)
AC_CHECK_TYPE(u_int8_t, unsigned char)



dnl ############## Compiler and Linker Flags
CFLAGS="$CFLAGS -std=c99 $ORTP_CFLAGS $LIBSNDFILE_CFLAGS"
LDFLAGS="$LDFLAGS $ORTP_LIBS $LIBSNDFILE_LIBS"

if test x$Debugging = xEnabled; then
	CFLAGS="$CFLAGS -g -Wunused -Wall -Werror"
else
	CFLAGS="$CFLAGS -O2"
fi



dnl ############## Final Output

AC_OUTPUT(Makefile src/Makefile doc/Makefile)



dnl ############## Display Message

echo "
  Multicast Audio Streaming Toolkit $VERSION
  Copyright (C) 2003-2006 University of Southampton

  Install path .......... $prefix
  Debugging ............. $Debugging
  OSS support ........... $OSSsupport
  JACK support .......... $JACKsupport
  
Next type 'make' and then 'make install'."

