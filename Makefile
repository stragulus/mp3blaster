CC=gcc
CPP=g++
CFLAGS=-Wall -ansi -pedantic -g -I./include
CPPFLAGS=-Wall -g -I./include -L./lib
OBJS=mp3play.o mp3core.o
LIBS=-lncurses -lpthread -lmpegsound -lm

mp3play:	${OBJS}
			$(CPP) $(CPPFLAGS) -o mp3play ${OBJS} ${LIBS}

mp3play.o:	mp3play.cc
			$(CPP) $(CPPFLAGS) -c mp3play.cc

mp3core.o:	mp3core.cc
			$(CPP) $(CPPFLAGS) -DPTHREADEDMPEG -c mp3core.cc
