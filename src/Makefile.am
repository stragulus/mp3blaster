bin_PROGRAMS		= mp3blaster mp3tag splay
#bin_PROGRAMS		= mp3blaster mp3tag
mp3blaster_SOURCES	= fileman.cc \
			  main.cc \
			  scrollwin.cc \
			  getopt.c \
			  getopt1.c \
			  fileman.h \
			  genretab.h \
			  getopt_local.h \
				id3parse.cc \
			  id3parse.h \
			  mp3blaster.h \
			  global.cc \
			  global.h \
			  config.cc \
			  scrollwin.h \
			  winitem.cc \
			  winitem.h \
			  mp3win.cc \
			  mp3win.h \
			  mp3item.cc \
			  mp3item.h \
				keybindings.h \
				history.cc \
				history.h \
				getinput.cc \
				getinput.h \
				exceptions.h
			  @MP3B_MODULES@
splay_SOURCES = splay.cc splay.h splay_common.cc
splay_DEPENDENCIES = ../mpegsound/libmpegsound.a
splay_LDADD = @LIBMPEGSOUND@ @LIBESD@ @LIBSDL@
mp3tag_SOURCES = id3parse.cc mp3tag.cc id3parse.h
mp3tag_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
mp3tag_LDADD = @LIBMPEGSOUND@ @LIBESD@ @LIBSDL@
mp3blaster_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
INCLUDES		= -I$(top_srcdir)/mpegsound -I/usr/include/ncurses -I$(top_srcdir)/nmixer -I$(includedir)
mp3blaster_LDADD	= @LIBMPEGSOUND@ @LIBNMIXER@ @NAS_LIBS@ @SID_LIBS@ @OGG_LIBS@ @LIRC_LIBS@ @NCURSES_LIBS@ @LIBESD@ @LIBSDL@
EXTRA_DIST = FILES
AM_CXXFLAGS	= @NAS_CFLAGS@ -DMP3BLASTER_DOCDIR=\"${pkgdatadir}\"
