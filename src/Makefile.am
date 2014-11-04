bin_PROGRAMS		= mp3blaster mp3tag
mp3blaster_SOURCES	= fileman.cc \
			  main.cc \
			  scrollwin.cc \
			  windows.cc \
			  getopt.c \
			  getopt1.c \
			  fileman.h \
			  genretab.h \
			  getopt.h \
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
			  @MP3B_MODULES@
mp3tag_SOURCES = id3parse.cc mp3tag.cc
mp3tag_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
mp3tag_LDADD = @LIBMPEGSOUND@ @LIBMYSQL@
mp3blaster_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
INCLUDES		= -I$(top_srcdir)/mpegsound -I/usr/include/ncurses -I$(top_srcdir)/nmixer -I$(includedir)
mp3blaster_LDADD	= @LIBMPEGSOUND@ @LIBNMIXER@ @NAS_LIBS@ @SID_LIBS@
mp3blaster_LIBS		= @LIBS@ @NAS_LIBS@ @SID_LIBS@
EXTRA_DIST = FILES
CXXFLAGS	= @CXXFLAGS@ @NAS_CFLAGS@ -DMP3BLASTER_DOCDIR=\"${pkgdatadir}\"
