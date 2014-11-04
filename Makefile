CC=gcc
CPP=g++
LIBPATHS=-L../splay-0.5/mpegsound -L/usr/local/lib
INCS=-I../splay-0.5/mpegsound
CPPFLAGS=-Wall -ansi -pedantic -g $(INCS) $(LIBPATHS)
OBJS=mp3play.o mp3core.o
LIBS=-lncurses -lpthread -lmpegsound -lm
RM=rm -f
INSTALL=/usr/bin/ginstall
#PREFIX is the installation path. Binaries will be added to PREFIX/bin.
PREFIX=/usr/local

#End of configurable part

mp3blaster: ${OBJS}
			$(CPP) $(CPPFLAGS) -o mp3blaster ${OBJS} ${LIBS}

mp3play.o:  mp3play.cc
			$(CPP) $(CPPFLAGS) -c mp3play.cc

mp3core.o:  mp3core.cc
			$(CPP) $(CPPFLAGS) -DPTHREADEDMPEG -c mp3core.cc

clean:
			$(RM) *.o mp3blaster core 

install:
			$(INSTALL) -g bin -o root -m 0755 -s mp3blaster $(PREFIX)/bin
