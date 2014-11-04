# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.3 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DISTDIR =

pkgdatadir = $(datadir)/mp3blaster
pkglibdir = $(libdir)/mp3blaster
pkgincludedir = $(includedir)/mp3blaster

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
CC = gcc
CXX = c++
LIBMPEGSOUND = ../mpegsound/libmpegsound.a
LIBNMIXER = ../nmixer/libnmixer.a
LN_S = ln -s
MAKEINFO = makeinfo
PACKAGE = mp3blaster
RANLIB = ranlib
SRCDIRS = mpegsound nmixer src
VERSION = 2.0b6

bin_PROGRAMS		= mp3blaster mp3tag
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
mp3tag_SOURCES = id3parse.cc mp3tag.cc
mp3blaster_DEPENDENCIES = ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
INCLUDES		= -I../mpegsound -I/usr/include/ncurses -I../nmixer
LDADD			= ../mpegsound/libmpegsound.a ../nmixer/libnmixer.a
LIBS			= -L../mpegsound -L../nmixer -lm  -lncurses
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS =  -DPACKAGE=\"mp3blaster\" -DVERSION=\"2.0b6\" -DHAVE_LIBM=1 -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_UNISTD_H=1 -DHAVE_ERRNO_H=1 -DHAVE_PTHREAD_H=1 -DHAVE_NCURSES_H=1 -DHAVE_CURSES_H=1 -DHAVE_MACHINE_SOUNDCARD_H=1 -DHAVE_STRDUP=1 -DHAVE_STRSTR=1  -I. -I$(srcdir) 
CPPFLAGS = 
LDFLAGS = 
mp3blaster_OBJECTS =  debug.o fileman.o gstack.o main.o mp3play.o \
mp3player.o mp3stack.o playwindow.o scrollwin.o windows.o
mp3blaster_LDADD = $(LDADD)
mp3blaster_LDFLAGS = 
mp3tag_OBJECTS =  id3parse.o mp3tag.o
mp3tag_LDADD = $(LDADD)
mp3tag_DEPENDENCIES = 
mp3tag_LDFLAGS = 
CXXFLAGS = -g -O2 -Wall -Wstrict-prototypes -W -fno-strength-reduce 
CXXCOMPILE = $(CXX) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS)
CXXLINK = $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
DEP_FILES =  .deps/debug.P .deps/fileman.P .deps/gstack.P \
.deps/id3parse.P .deps/main.P .deps/mp3play.P .deps/mp3player.P \
.deps/mp3stack.P .deps/mp3tag.P .deps/playwindow.P .deps/scrollwin.P \
.deps/windows.P
CXXMKDEP = $(CXX) -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS)
SOURCES = $(mp3blaster_SOURCES) $(mp3tag_SOURCES)
OBJECTS = $(mp3blaster_OBJECTS) $(mp3tag_OBJECTS)

all: Makefile $(PROGRAMS)

.SUFFIXES:
.SUFFIXES: .S .c .cc .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && $(AUTOMAKE) --gnu src/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

mp3blaster: $(mp3blaster_OBJECTS) $(mp3blaster_DEPENDENCIES)
	@rm -f mp3blaster
	$(CXXLINK) $(mp3blaster_LDFLAGS) $(mp3blaster_OBJECTS) $(mp3blaster_LDADD) $(LIBS)

mp3tag: $(mp3tag_OBJECTS) $(mp3tag_DEPENDENCIES)
	@rm -f mp3tag
	$(CXXLINK) $(mp3tag_LDFLAGS) $(mp3tag_OBJECTS) $(mp3tag_LDADD) $(LIBS)
.cc.o:
	$(CXXCOMPILE) -c $<

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $(SOURCES) $(HEADERS) $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu src/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:

maintainer-clean-depend:
	-rm -rf .deps

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).P -c $<

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).p -c $<
	@-sed -e 's/^\([^:]*\)\.o:/\1.lo \1.o:/' \
	  < .deps/$(*F).p > .deps/$(*F).P
	@-rm -f .deps/$(*F).p

%.o: %.cc
	@echo '$(CXXCOMPILE) -c $<'; \
	$(CXXCOMPILE) -Wp,-MD,.deps/$(*F).P -c $<

%.lo: %.cc
	@echo '$(LTCXXCOMPILE) -c $<'; \
	$(LTCXXCOMPILE) -Wp,-MD,.deps/$(*F).p -c $<
	@-sed -e 's/^\([^:]*\)\.o:/\1.lo \1.o:/' \
	  < .deps/$(*F).p > .deps/$(*F).P
	@-rm -f .deps/$(*F).p
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS
	@$(NORMAL_INSTALL)

install-data: 
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(DATADIR)$(bindir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-depend mostlyclean-generic

clean:  clean-binPROGRAMS clean-compile clean-tags clean-depend \
		clean-generic mostlyclean

distclean:  distclean-binPROGRAMS distclean-compile distclean-tags \
		distclean-depend distclean-generic clean
	-rm -f config.status

maintainer-clean:  maintainer-clean-binPROGRAMS maintainer-clean-compile \
		maintainer-clean-tags maintainer-clean-depend \
		maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir mostlyclean-depend \
distclean-depend clean-depend maintainer-clean-depend info dvi \
installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
