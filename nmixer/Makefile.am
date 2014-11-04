bin_PROGRAMS	= nmixer
nmixer_SOURCES	= main.cc
nmixer_DEPENDENCIES = libnmixer.a
LDADD           = @NCURSES_LIBS@ @NAS_LIBS@ @LIBNMIXER@
INCLUDES		= -I/usr/include/ncurses -I$(srcdir) -I$(includedir)
noinst_LIBRARIES = libnmixer.a
libnmixer_a_SOURCES = nmixer.cc mixers.cc ossmixer.cc nasmixer.cc nmixer.h \
                      getopt.c getopt1.c getopt_local.h nullmixer.cc
AM_CXXFLAGS = @NAS_CFLAGS@
