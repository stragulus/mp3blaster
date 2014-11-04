CC=gcc
CPP=g++
LIBPATHS=-L./lib -L/usr/local/lib
INCS=-I./include -I/usr/include/ncurses
#CPPFLAGS=-Wall -ansi -pedantic $(INCS) $(LIBPATHS)
#use -g flag for debugging with gdb.
CPPFLAGS=-Wall -ansi -pedantic -g $(INCS) $(LIBPATHS)
OBJS=mp3play.o mp3core.o
LIBS=-lncurses -lpthread -lmpegsound -lm
RM=rm -f
INSTALL=/usr/bin/ginstall
#PREFIX is the installation path. Binaries will be added to PREFIX/bin.
PREFIX=/usr/local
TESTOBJS=test.o debug.o scrollwin.o gstack.o mp3stack.o windows.o mp3player.o mp3play.o playwindow.o
TSTOBJS=tst.o mp3player.o

#End of configurable part

test:			${TESTOBJS}
				$(CC) $(CPPFLAGS) -o test ${TESTOBJS} $(LIBS)

tst:			${TSTOBJS}
				$(CC) $(CPPFLAGS) -o tst ${TSTOBJS} $(LIBS)

%.o:			%.cc
				$(CPP) $(CPPFLAGS) -c $<

clean:
			$(RM) *.o test mp3blaster core tst

#install:
#			$(INSTALL) -g bin -o root -m 0755 -s linuxamp $(PREFIX)/bin
