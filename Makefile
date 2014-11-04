CC=gcc
CPP=g++
LIBPATHS=-L./lib -L/usr/local/lib
INCS=-I./include -I/usr/include/ncurses
CPPFLAGS=-O2 -Wall -ansi -pedantic $(INCS) $(LIBPATHS)
#use -g flag for debugging with gdb.
#CPPFLAGS=-Wall -g -ansi -pedantic $(INCS) $(LIBPATHS)
LIBS=-lncurses -lpthread -lmpegsound -lm
RM=rm -f
INSTALL=/usr/bin/ginstall
#PREFIX is the installation path. Binaries will be added to PREFIX/bin.
PREFIX=/usr/local

#End of configurable part

OBJS=scrollwin.o gstack.o mp3stack.o mp3player.o mp3play.o playwindow.o fileman.o main.o windows.o


mp3blaster:	${OBJS}
		$(CC) $(CPPFLAGS) -o mp3blaster ${OBJS} $(LIBS) -s

%.o:		%.cc
		$(CPP) $(CPPFLAGS) -c $<

clean:
		$(RM) *.o mp3blaster core

install:	
		strip mp3blaster
		$(INSTALL) -g bin -o root -m 0755 -s mp3blaster $(PREFIX)/bin
