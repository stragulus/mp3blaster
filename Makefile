CC=gcc
CPP=g++
CFLAGS=-Wall -ansi -pedantic -g
CPPFLAGS=-Wall -ansi -pedantic -g
OBJS=mp3play.o mp3core.o
LIBS=-lncurses -lpthread -L. -lmpegsound -lm

mp3play:	${OBJS}
			$(CPP) $(CPPFLAGS) -D__USE_BSD -o mp3play ${OBJS} ${LIBS}

mp3play.o:	mp3play.cc
			$(CPP) $(CPPFLAGS) -c mp3play.cc

mp3core.o:	mp3core.cc
			$(CPP) $(CPPFLAGS) -DPTHREADEDMPEG -c mp3core.cc
