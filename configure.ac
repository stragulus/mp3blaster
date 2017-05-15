dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(mp3blaster,3.2.6)
AC_CONFIG_SRCDIR([src/mp3blaster.h])
AM_INIT_AUTOMAKE

#Custom C++ flags - later options override earlier ones
if [ -z "$CXXFLAGS" ] ; then
	CXXFLAGS="-O2 -Wall -W -fno-strength-reduce"
fi

AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_RANLIB

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS(strdup strstr)

AC_LANG_CPLUSPLUS

#For FreeBSD, look in /usr/local/(lib,include) first, since ncurses4
#must be installed as an add-on on these systems in /usr/local
if test "`uname -s`" = "FreeBSD" ; then
  echo "FreeBSD detected."
  FREEBSD=1
  LDFLAGS="-L/usr/local/lib $LDFLAGS"
  CPPFLAGS="-I/usr/local/include $CPPFLAGS"
  AC_DEFINE(AUDIO_NONBLOCKING,1,
	[write audiodata to the sound device without blocking])
fi

#For NetBSD, look in /usr/pkg/(lib,include) first. Otherwise, the wrong
#ncurses package might be detected.

if test "`uname -s`" = "NetBSD" ; then
  echo "NetBSD detected."
  NETBSD=1
  LDFLAGS="-L/usr/pkg/lib $LDFLAGS"
  CPPFLAGS="-I/usr/pkg/include $CPPFLAGS"
fi

if test "`uname -s`" = "OpenBSD" ; then
	echo "OpenBSD detected"
	OPENBSD=1
	LDFLAGS="-L/usr/local/lib $LDFLAGS"
	CPPFLAGS="-I/usr/local/include $CPPFLAGS"
fi

if test "`uname -s`" = "Darwin" ; then
	dnl darwinports installs stuff in /opt/local, add those by default
	echo "Darwin OS detected"
	DARWINOS=1
	LDFLAGS="-L/opt/local/lib $LDFLAGS"
	CPPFLAGS="-I/opt/local/include $CPPFLAGS"
fi

dnl Check for additional paths to include-files and directories first 
AC_ARG_WITH(extra-includes,
  [  --with-extra-includes   Specify paths for additional headerfiles],
  [ CPPFLAGS="$CPPFLAGS $withval" ])
AC_ARG_WITH(extra-libs,
  [  --with-extra-libs       Specify location of extra libs],
  [ LDFLAGS="$LDFLAGS $withval" ])
AC_ARG_WITH(pthreads,
  [  --with-pthreads         Use pthreads [default=yes]],
  [ if test "$withval" = "yes" ; then INCLUDEPTHREAD=1
    else INCLUDEPTHREAD=0; fi ],
  [ INCLUDEPTHREAD=1 ])
dnl silently auto-disable pthreads if pth is chosen.

#Variable naming scheme:
#WANT_*: Whether a feature is wanted (can be implicit)
#REQ_*: User *requires* this feature, bail if we can't use it!

#optional packages
WANT_PTH=0
AC_ARG_WITH(pth,
  [  --with-pth              Experimental support for pth lib [default=no]],
  [ if test "$withval" = "yes" ; then WANT_PTH=1 ; REQ_PTH=1; INCLUDEPTHREAD=0 ; else WANT_PTH=0 ; fi ])

WANT_LIRC=0
AC_ARG_WITH(lirc,
  [  --with-lirc             Support for LIRC [default=no]],
  [ if test "$withval" = "yes" ; then WANT_LIRC=1; REQ_LIRC=1 ; else WANT_LIRC=0; fi ])

WANT_OGG=1
AC_ARG_WITH(oggvorbis,
  [  --with-oggvorbis        Beta support for ogg vorbis format [default=yes]],
  [ if test "$withval" = "yes" ; then WANT_OGG=1; REQ_OGG=1 ; else WANT_OGG=0; fi ])

WANT_NAS=0
AC_ARG_WITH(nas,
  [  --with-nas              Network Audio System [default=no]],
  [ if test "$withval" = "yes" ; then WANT_NAS=1 ; REQ_NAS=1 ; else WANT_NAS=0 ; fi ])

AC_ARG_WITH(curses,
  [  --with-curses           Force using curses instead of ncurses [default=no]],
  [ if test "$withval" = "yes" ; then FORCECURSES=1 ; else FORCECURSES=0; fi],
  [ FORCECURSES=0 ])

WANT_SID=1
AC_ARG_WITH(sidplay,
  [  --with-sidplay          Sidplayer support [default=yes]],
  [ if test "$withval" = "yes" ; then WANT_SID=1; REQ_SID=1 ; else WANT_SID=0 ; fi ])

WANT_ESD=0
AC_ARG_WITH(esd,
	[  --with-esd              Support ESD for audio playback [default=yes]],
	[ if test "$withval" = "yes" ; then WANT_ESD=1; REQ_ESD=1 ; else WANT_ESD=0 ; fi ])

WANT_SDL=1
AC_ARG_WITH(sdl,
	[  --with-sdl              Support SDL for audio playback [default=yes]],
	[ if test "$withval" = "yes" ; then WANT_SDL=1;REQ_SDL=1 ; else WANT_SDL=0 ; fi ])

WANT_OSS=1
AC_ARG_WITH(oss,
	[  --with-oss              Support OSS for audio playback [default=yes]],
	[ if test "$withval" = "yes" ; then WANT_OSS=1; REQ_OSS=1 ; else WANT_OSS=0 ; fi ])

#Features 
AC_ARG_ENABLE(static,
  [  --enable-static         Build static binaries instead [default=no]],
  [ if test "$enableval" = "yes" ; then
      LDFLAGS="$LDFLAGS -static" ; HAVE_STATIC="yes"
    else HAVE_STATIC="no" ; fi ],
  [ HAVE_STATIC="no" ])

AC_ARG_ENABLE(newthreads,
  [  --enable-newthreads     New (very experimental) buffering [default=no]],
	[ if test "$enableval" = "yes" ; then NEWTHREAD=1 ; else NEWTHREAD=0 ; fi ],
	[ NEWTHREAD=0 ])

dnl Does the user want to use the 'newthreads' buffering system?
dnl Perform this check *before* any pth/pthreads check!
if test "$NEWTHREAD" = "1" ; then
  echo "***********"
  echo "Enabling experimental 'newthreads' buffered playback support."
	echo "This auto-enables pth (instead of pthreads)."
  AC_DEFINE(NEWTHREAD,1,
		[Use alternative buffering mechanism when using OSS output driver])
	WANT_PTH=1
	REQ_PTH=1 #It's now required!
	INCLUDEPTHREAD=0
fi

dnl Checks for libraries.
AC_CHECK_LIB(m,main,,AC_MSG_ERROR([no math library found (libm)]))
AC_CHECK_LIB(ossaudio,main)

if test "$INCLUDEPTHREAD" = 1 ; then
	if test "$FREEBSD" = 1 -o "$OPENBSD" = 1 ; then
  	LDFLAGS="$LDFLAGS -pthread"
	else
  	AC_CHECK_LIB(pthread,main,,INCLUDEPTHREAD=0)
	fi
fi


AC_LANG(C)

HAVE_LIRC="no"
if test "$WANT_LIRC" = "1" -a "$WANT_PTH" = "0"; then
  AC_CHECK_LIB(lirc_client,lirc_init, [ HAVE_LIRC="yes" ], [ HAVE_LIRC="no" ])
  AC_CHECK_HEADER([lirc/lirc_client.h], [], [ HAVE_LIRC="no" ])

  if test "$HAVE_LIRC" = "yes" ; then
    LIRC_LIBS="-llirc_client"
		AC_SUBST(LIRC_LIBS)
    AC_DEFINE(INCLUDE_LIRC,1,[LIRC support])
	elif test "$REQ_LIRC" = "1" ; then
		AC_MSG_ERROR([lirc support requested, but libs/includes not found])
  fi
fi

AC_LANG(C++)

HAVE_OGG="no"
if test "$WANT_OGG" = "1" ; then
  AC_CHECK_LIB(vorbisfile,main,HAVE_OGG="yes",HAVE_OGG="no",-lvorbis)
  if test "$HAVE_OGG" = "yes" ; then
    OGG_LIBS="-lvorbisfile -lvorbis"
		AC_SUBST(OGG_LIBS)
    AC_DEFINE(INCLUDE_OGG,1,[OGG playback support])
	elif test "$REQ_OGG" = "1" ; then
		AC_MSG_ERROR([ogg support requested, but libs/includes not found])
  fi
fi

dnl Check PTH support
HAVE_PTH="no"
if test "$WANT_PTH" = "1" ; then
  if test -z "`pth-config --version 2>/dev/null`" ; then
		AC_MSG_ERROR([you want PTH support, but pth-config is not in your path])
  fi
  
  if ! test "$INCLUDEPTHREAD" = "0" ; then
    AC_MSG_ERROR([you cannot use pthread *and* pth, disable one of both.])
  fi

  LDFLAGS="$LDFLAGS `pth-config --ldflags`"
  LIBS="$LIBS `pth-config --libs`"
  CXXFLAGS="$CXXFLAGS -I`pth-config --includedir`"
  dnl set CPPFLAGS so AC_CHECK_HEADER can find pth.h
  CPPFLAGS="$CPPFLAGS -I`pth-config --includedir`"
  AC_DEFINE(LIBPTH,1,[Use PTH as posix threads library])
  HAVE_PTH="yes"
fi


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h malloc.h sys/ioctl.h unistd.h errno.h)
AC_CHECK_HEADERS(bool.h)
AC_CHECK_HEADERS(pthread.h pthread/mit/pthread.h pth.h)
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(sys/soundcard.h, [ SOUND_H="<sys/soundcard.h>" ])
AC_CHECK_HEADERS(machine/soundcard.h, [ SOUND_H="<machine/soundcard.h>" ])
AC_CHECK_HEADERS(soundcard.h, [ SOUND_H="<soundcard.h>" ])

dnl curses (required)
WANT_CURSES=1
REQ_CURSES=1

if test "$WANT_CURSES" = "1" ; then
	AC_CHECK_LIB(curses,main,HAVE_LIBCURSES=1)
	AC_CHECK_LIB(ncurses,main,HAVE_LIBNCURSES=1)
	AC_CHECK_HEADER(curses.h,HAVE_CURSES_H=1,HAVE_CURSES_H=0)
	AC_CHECK_HEADER(ncurses/ncurses.h,HAVE_NCURSES_NCURSES_H=1)
	AC_CHECK_HEADER(ncurses/curses.h,HAVE_NCURSES_CURSES_H=1)
	AC_CHECK_HEADER(ncurses.h,HAVE_NCURSES_H=1)

	if test "$FORCECURSES" = "1" ; then
		if test "$HAVE_LIBCURSES" = 0 -o "$HAVE_CURSES_H" = 0; then
			AC_MSG_ERROR([you forced using curses instead of ncurses, but lib/include cannot be found])
		fi
		AC_DEFINE_UNQUOTED(NCURSES_HEADER, [<curses.h>],
			[Which headerfile to include for ncurses support])
		NCURSES_LIBS="-lcurses"
		AC_SUBST(NCURSES_LIBS)
	else
		HAVE_CURSES="yes"
		if test "$HAVE_LIBNCURSES" = 1 ; then
			NCURSES_LIBS="-lncurses"
		elif test "$HAVE_LIBCURSES" = 1 ; then
			NCURSES_LIBS="-lcurses"
		else
			HAVE_CURSES="no"
		fi

		#header(s) found?
		HAVE_CURSES_HEADER=1
		if test "$HAVE_NCURSES_NCURSES_H" = "1" ; then
			NCURSES_HEADER="<ncurses/ncurses.h>"
		elif test "$HAVE_NCURSES_CURSES_H" = "1" ; then
			NCURSES_HEADER="<ncurses/curses.h>"
		elif test "$HAVE_NCURSES_H" = "1" ; then
			NCURSES_HEADER="<ncurses.h>"
		elif test "$HAVE_CURSES_H" = "1" ; then
			NCURSES_HEADER="<curses.h>"
		else
			HAVE_CURSES_HEADER=0
		fi

		if test "$HAVE_CURSES_HEADER" = "0" ; then HAVE_CURSES="no" ; fi

		if test "$HAVE_CURSES" = "yes" ; then
			AC_SUBST(NCURSES_LIBS)
			AC_DEFINE_UNQUOTED(NCURSES_HEADER, $NCURSES_HEADER,
				[Which headerfile to include for ncurses support])
		elif test "$REQ_CURSES" = "1" ; then
			AC_MSG_ERROR([could not find (n)curses libs or includes, which are required])
		fi
	fi #FORCECURSES
fi

#Sound output devices.
HAVE_SOUNDOUTPUT=0

#Use ESD ?
HAVE_ESD="no"
if test "$WANT_ESD" = "1" ; then
	AC_CHECK_LIB(esd,esd_open_sound, [ HAVE_ESD="yes" ], [ HAVE_ESD="no" ])
	AC_CHECK_HEADER(esd.h, [], [ HAVE_ESD="no" ])

	if test "$HAVE_ESD" = "yes"
	then
		AC_DEFINE(WANT_ESD,1,[ESD audio output support])
		LIBESD="-lesd"
		AC_SUBST(LIBESD)
		#We have an alternative to OSS!
		HAVE_SOUNDOUTPUT=1
	elif test "$REQ_ESD" = "1" ; then
		AC_MSG_ERROR([you requested ESD support, but ESD library or includes cannot be found in your library path])
	fi
fi

dnl Use SDL ?
HAVE_SDL="no"

dnl SDL and NEWTHREADS don't mix (pthreads vs pth)
if test "$HAVE_PTH" = "yes" ; then
	WANT_SDL=0

	if test "$REQ_SDL" = "1" ; then
		AC_MSG_ERROR([you requested SDL support, but it conflicts with having PTH enabled (e.g. when enabling newthreads). Disable one or the other])
	fi
fi

if test "$WANT_SDL" = "1" ; then
	OLD_CXXFLAGS="$CXXFLAGS"
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"

	dnl There's plenty sdl*config's about, find the right one
	AC_PATH_PROG(SDL12_CONFIG, sdl12-config, no)
	SDL_CONFIG="$SDL12_CONFIG"
	SDLLIB="SDL-1.2"
	if test "$SDL_CONFIG" = "no" ; then
		AC_PATH_PROG(SDL11_CONFIG, sdl11-config, no)
		SDL_CONFIG="$SDL11_CONFIG"
		SDLLIB="SDL-1.1"
	fi
	if test "$SDL_CONFIG" = "no" ; then
		AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
		SDLLIB="SDL"
	fi

	if test "$SDL_CONFIG" = "no" ; then
		if test "$REQ_SDL" = "1" ; then
			AC_MSG_ERROR([you requested sdl support, but sdl-config / sdl11-config / sdl12-config cannot be found in your PATH])
		fi
	else
		SDL_CFLAGS="`$SDL_CONFIG --cflags`"
		SDL_LIBS="`$SDL_CONFIG --libs|sed s+'-lSDLmain'++`"

		CXXFLAGS="$SDL_CFLAGS $CXXFLAGS"
		CPPFLAGS="$SDL_CFLAGS $CPPFLAGS"
		LDFLAGS="$SDL_LIBS $LDFLAGS"
		AC_CHECK_LIB($SDLLIB,SDL_OpenAudio, [ HAVE_SDL="yes" ], [ HAVE_SDL="no" ])
		AC_CHECK_HEADER(SDL.h, [], [ HAVE_SDL="no" ])
		AC_CHECK_HEADER(SDL_audio.h, [], [ HAVE_SDL="no" ])
	fi

	if test "$HAVE_SDL" = "no" -a "$REQ_SDL" = "1" ; then
		AC_MSG_ERROR([you requested SDL support, but SDL library or includes cannot be found])
	fi

	LDFLAGS="$OLD_LDFLAGS"

	if test "$HAVE_SDL" = "yes"
	then
		AC_DEFINE(WANT_SDL,1,[SDL audio output support])
		LIBSDL="$SDL_LIBS"
		AC_SUBST(LIBSDL)
		#We have an alternative to OSS!
		HAVE_SOUNDOUTPUT=1
	else
		CXXFLAGS="$OLD_CXXFLAGS"
		CPPFLAGS="$OLD_CPPFLAGS"
	fi
fi #WANT_SDL

HAVE_OSS=no
if test "$WANT_OSS" = "1" ; then
	HAVE_OSS="yes"
	if test -n "$SOUND_H" ; then
		AC_DEFINE_UNQUOTED(SOUNDCARD_HEADERFILE, $SOUND_H,
			[OSS driver include file])
	else
		HAVE_OSS="no"
	fi

	if test "$HAVE_OSS" = "yes"
	then
		HAVE_SOUNDOUTPUT=1
		AC_DEFINE(WANT_OSS,1,[OSS audio output support])
	elif test "$REQ_OSS" = "1" ; then
		AC_MSG_ERROR([you requested OSS audio, but includefiles cannot be found])
	fi
fi

if test "$HAVE_SOUNDOUTPUT" = "0" ; then
		AC_MSG_ERROR([no sound output device support found. Install SDL if you don't have an OSS sound driver])
fi

dnl Check SID support
HAVE_SID="no"
if test "$WANT_SID" = 1 ; then
  AC_CHECK_LIB(sidplay, main,HAVE_SID="yes",HAVE_SID="no")
  if test "$HAVE_SID" = "yes" ; then
    SID_LIBS="-lsidplay"
    AC_DEFINE(HAVE_SIDPLAYER,1,[SID Playback support])
    AC_SUBST(SID_LIBS)
	elif test "$REQ_SID" = "1" ; then
		AC_MSG_ERROR([you rquested SID playback, but libs/includes cannot be found])
  fi
fi

dnl Check NAS support
AC_LANG(C)

HAVE_NAS="no"
if test "$WANT_NAS" = "1" ; then
  AC_PATH_XTRA
  dnl X_CFLAGS are needed to find audio/audiolib.h on many systems
  tmp_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $X_CFLAGS"
  
  dnl AC_CHECK_LIB(audio, AuOpenServer,INCLUDENAS=1,,-lXt -lX11 -lm)
  AC_CHECK_LIB(audio, AuOpenServer,INCLUDENAS=1,,$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS -lXt -lX11 -lm)
  AC_CHECK_HEADER(audio/audiolib.h,INCLUDENASH=1)
  dnl LIBS=$tmp_LIBS
  dnl CXXFLAGS is not checked during configure, CPPFLAGS is though.
  CPPFLAGS=$tmp_CPPFLAGS
  
  if test "$INCLUDENAS" = 1 -a "$INCLUDENASH" = 1; then
    NAS_LIBS="$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS -laudio -lXt -lX11 -lm"
    NAS_CFLAGS=$X_CFLAGS
    AC_DEFINE(WANT_NAS,1,[NAS audio output support])
    AC_SUBST(NAS_LIBS)
    AC_SUBST(NAS_CFLAGS)
    HAVE_NAS="yes"
  fi

	if test "$HAVE_NAS" = "no" -a "$REQ_NAS" = "1" ; then
		AC_MSG_ERROR([you requested NAS support, but libs/includes cannot be found])
	fi
fi

AC_LANG(C++)

if test "$OPENBSD" = 1 -o "$NETBSD" = 1 ; then
  SOUNDDEV="/dev/audio"
else
  SOUNDDEV="/dev/dsp"
fi

AC_DEFINE_UNQUOTED(SOUND_DEVICE, "$SOUNDDEV",
	[Default OSS sound device to use])

if test "$INCLUDEPTHREAD" = 1 ; then
  if test "$FREEBSD" = 1 -o "$OPENBSD" = 1 ; then
    LDFLAGS="$LDFLAGS -pthread"
    AC_DEFINE(_THREAD_SAFE,1,[Pthreads-required symbol on some systems])
    dnl AC_DEFINE(_REENTRANT,1,[Pthreads-required symbol on some systems])
  else
    LIBS="$LIBS -lpthread"
    #Is _REENTRANT only needed for Linux? Is it still needed? Where is the
    #documentation about _REENTRANT???? I've read in a newsgroup _THREAD_SAFE
    #should be used for FreeBSD, _REENTRANT for linux....dunno why.
    AC_DEFINE(_REENTRANT,1,[Pthreads-required symbol on some systems])
  fi
  AC_DEFINE(PTHREADEDMPEG,1,
		[Use pthreads for buffered audio playback (OSS only)])
  dnl AC_DEFINE(THREAD_2)
elif test "$HAVE_PTH" = "no" ; then
  echo "Sorry, either libpthread OR libpth is required."
  exit 1;
fi

dnl A check for a particular __math.h file that causes sigsegv's in combination
dnl with gcc 2.7.*  / egcs-1.1.1 or older versions
if test -f /usr/include/__math.h -a "`grep 'long long int __p = (long long int) __y' /usr/include/__math.h 2>/dev/null`" != "" -a \( "`$CXX -v 2>&1|grep 'gcc version 2.[[567]]'`" != "" -o "`$CXX -v 2>&1|tail -1|grep egcs|cut -d'(' -f2|egrep 'egcs-(0|1.(0|1.[[01]]))'`" != "" \) ; then
    CXXFLAGS="$CXXFLAGS -D__NO_MATH_INLINES"
fi
dnl Even the script above is not foolproof. Since I can't tell the difference
dnl in speed with or without this define, just define it.
dnl 2002/03/16: removed this, if you still use an ancient gcc-version and the
dnl check doesn't work, enable this yourself..
dnl CXXFLAGS="$CXXFLAGS -D__NO_MATH_INLINES"

LDFLAGS="$LDFLAGS -L../mpegsound -L../nmixer"
LIBMPEGSOUND="-lmpegsound"
LIBNMIXER="-lnmixer"
AC_SUBST(LIBMPEGSOUND)
AC_SUBST(LIBNMIXER)

dnl Produce output summary

PTHREADS_OR_PTH="pthreads"
if test "$HAVE_PTH" = "yes" ; then PTHREADS_OR_PTH="pth" ; fi

AUDIO_FORMATS="mp3 wav"
if test "$HAVE_OGG" = "yes" ; then AUDIO_FORMATS="$AUDIO_FORMATS ogg" ; fi
if test "$HAVE_SID" = "yes" ; then AUDIO_FORMATS="$AUDIO_FORMATS sid" ; fi
AUDIO_DRIVERS=""
if test "$HAVE_OSS" = "yes" ; then AUDIO_DRIVERS="${AUDIO_DRIVERS}oss " ; fi
if test "$HAVE_SDL" = "yes" ; then AUDIO_DRIVERS="${AUDIO_DRIVERS}sdl " ; fi
if test "$HAVE_ESD" = "yes" ; then AUDIO_DRIVERS="${AUDIO_DRIVERS}esd " ; fi
if test "$HAVE_NAS" = "yes" ; then AUDIO_DRIVERS="${AUDIO_DRIVERS}nas " ; fi
BUILDOPTS=""
if test "$HAVE_LIRC" = "yes"; then BUILDOPTS="${BUILDOPTS}lirc "; fi
if test "$HAVE_PTH" = "yes"; then BUILDOPTS="${BUILDOPTS}pth "; fi
if test "$INCLUDEPTHREAD" = "1"; then BUILDOPTS="${BUILDOPTS}pthreads "; fi
if test "$HAVE_STATIC" = "yes"; then BUILDOPTS="${BUILDOPTS}static "; fi
if test "$NEWTHREAD" = "1"; then BUILDOPTS="${BUILDOPTS}newthreads "; fi

AC_DEFINE_UNQUOTED(BUILDOPTS_AUDIOFORMATS, ["$AUDIO_FORMATS"],
	[Compile-time audio foramt support info])
AC_DEFINE_UNQUOTED(BUILDOPTS_AUDIODRIVERS, ["$AUDIO_DRIVERS"],
	[Compile-time audio output driver info])
AC_DEFINE_UNQUOTED(BUILDOPTS_FEATURES, ["$BUILDOPTS"],
	[Compile-time features info])

SRCDIRS="mpegsound nmixer src doc doc/charmap"
AC_SUBST(SRCDIRS)
AC_OUTPUT(src/Makefile mpegsound/Makefile Makefile nmixer/Makefile doc/Makefile doc/charmap/Makefile)


AC_MSG_RESULT([
Summary:

Support audio formats: $AUDIO_FORMATS
Audio output drivers : $AUDIO_DRIVERS

Misc:
static binaries        $HAVE_STATIC
lirc                   $HAVE_LIRC
libpth                 $HAVE_PTH
pthreads or pth        $PTHREADS_OR_PTH

*****************************************************************************
* Questions? Bug reports? Read ./FAQ or go to http://mp3blaster.sf.net/     *
*****************************************************************************
])
