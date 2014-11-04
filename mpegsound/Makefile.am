noinst_LIBRARIES	= libmpegsound.a

libmpegsound_a_SOURCES  = soundinputstream.cc fileinput.cc httpinput.cc \
			  soundplayer.cc rawplayer.cc rawtofile.cc \
			  mpegtable.cc filter.cc filter_2.cc \
			  mpegtoraw.cc mpeglayer1.cc mpeglayer2.cc \
			  mpeglayer3.cc bitwindow.cc huffmantable.cc \
			  wavetoraw.cc sidplayer.cc \
			  fileplayer.cc nasplayer.cc oggplayer.cc \
				xingheader.cc xingheader.h

noinst_HEADERS		= mpegsound.h mpegsound_locals.h

INCLUDES                = -I$(srcdir) -I$(includedir)
CXXFLAGS                = @CXXFLAGS@ @NAS_CFLAGS@
