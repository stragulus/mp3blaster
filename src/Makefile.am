bin_PROGRAMS		= mp3blaster mp3tag
mp3blaster_SOURCES	= fileman.cc \
			  gstack.cc \
			  main.cc \
			  mp3play.cc \
			  mp3player.cc \
			  sidplayer.cc \
			  mp3stack.cc \
			  playwindow.cc \
			  scrollwin.cc \
			  windows.cc \
			  getopt.c \
			  getopt1.c \
			  fileman.h \
			  genretab.h \
			  getopt.h \
			  gstack.h \
			  id3parse.h \
			  mp3blaster.h \
			  mp3play.h \
			  mp3player.h \
			  sidplayer.h \
			  mp3stack.h \
			  playwindow.h \
			  global.cc \
			  global.h \
			  config.cc \
			  scrollwin.h \
			  @MP3B_MODULES@
mp3tag_SOURCES = id3parse.cc mp3tag.cc
mp3tag_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
mp3tag_LDADD = @LIBMPEGSOUND@ @LIBMYSQL@
mp3blaster_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
INCLUDES		= -I$(top_srcdir)/mpegsound -I/usr/include/ncurses -I$(top_srcdir)/nmixer -I$(includedir)
mp3blaster_LDADD	= @LIBMPEGSOUND@ @LIBNMIXER@ @NAS_LIBS@ @SID_LIBS@
mp3blaster_LIBS		= @LIBS@ @NAS_LIBS@ @SID_LIBS@
EXTRA_DIST = FILES
CXXFLAGS	= @CXXFLAGS@ @NAS_CFLAGS@
