bin_PROGRAMS	= nmixer
nmixer_SOURCES	= main.cc
nmixer_DEPENDENCIES = @LIBNMIXER@
LDADD           = @LIBNMIXER@ @NCURSES_LIBS@
INCLUDES		= -I/usr/include/ncurses -I$(srcdir) -I$(includedir)
noinst_LIBRARIES = libnmixer.a
libnmixer_a_SOURCES = nmixer.cc mixers.cc ossmixer.cc nasmixer.cc nmixer.h \
                      getopt.c getopt1.c getopt.h
CXXFLAGS = @NAS_CFLAGS@
