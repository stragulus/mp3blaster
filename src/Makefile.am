bin_PROGRAMS		= mp3blaster

mp3blaster_SOURCES	= debug.cc \
			  fileman.cc \
			  gstack.cc \
			  main.cc \
			  mp3play.cc \
			  mp3player.cc \
			  mp3stack.cc \
			  playwindow.cc \
			  scrollwin.cc \
			  windows.cc
mp3blaster_DEPENDENCIES = @LIBMPEGSOUND@ @LIBNMIXER@
INCLUDES		= -I../mpegsound -I/usr/include/ncurses -I../nmixer
LDADD			= @LIBMPEGSOUND@ @LIBNMIXER@
LIBS			= -L../mpegsound -L../nmixer @LIBS@
