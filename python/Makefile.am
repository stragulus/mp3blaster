lib_LTLIBRARIES	= libsplay.la

libsplay_la_SOURCES  = glue.cc \
        ../mpegsound/soundinputstream.cc ../mpegsound/fileinput.cc ../mpegsound/httpinput.cc \
			  ../mpegsound/soundplayer.cc ../mpegsound/rawplayer.cc ../mpegsound/rawtofile.cc \
			  ../mpegsound/mpegtable.cc ../mpegsound/filter.cc ../mpegsound/filter_2.cc \
			  ../mpegsound/mpegtoraw.cc ../mpegsound/mpeglayer1.cc ../mpegsound/mpeglayer2.cc \
			  ../mpegsound/mpeglayer3.cc ../mpegsound/bitwindow.cc ../mpegsound/huffmantable.cc \
			  ../mpegsound/wavetoraw.cc ../mpegsound/sidplayer.cc \
			  ../mpegsound/fileplayer.cc ../mpegsound/nasplayer.cc ../mpegsound/oggplayer.cc \
				../mpegsound/xingheader.cc ../mpegsound/xingheader.h ../mpegsound/esdplayer.cc \
				../mpegsound/sdlplayer.cc ../mpegsound/cyclicbuffer.cc ../mpegsound/cyclicbuffer.h
libsplay_la_LDFLAGS  = -version-info 0:0:0

INCLUDES                = -I$(top_srcdir)/mpegsound -I$(srcdir) -I$(includedir)
AM_CXXFLAGS             = @NAS_CFLAGS@
