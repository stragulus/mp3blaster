/* Copyright (C) Bram Avontuur (bram@avontuur.org) */
@TOP@

/* A non-blocking implementation of writing to the audio device. On some 
 * systems (not many!) with threads, this will lower hickups in sound 
 */
#undef AUDIO_NONBLOCKING

/* Include the OggVorbis sound format */
#undef INCLUDE_OGG

/* MySQL support for mp3tag. Probably only useful for the author :) */
#undef WANT_MYSQL
#undef MYSQL_H

/* Use libpth as threads */
#undef LIBPTH

/* alternative threaded playback system - requires libpth */
#undef NEWTHREAD

/* sid support (broken) */
#undef HAVE_SIDPLAYER

/* nas support (broken) */
#undef HAVE_NASPLAYER

/* what's your sound device? */
#undef SOUND_DEVICE

/* _THREAD_SAFE is for FreeBSD threaded programs (although I think it's 
 * predefined already nowadays) 
 * _REENTRANT is needed for threaded programs.
 */
#undef _THREAD_SAFE
#undef _REENTRANT

/* use pthreads for threading */
#undef PTHREADEDMPEG

#undef SOUNDCARD_HEADERFILE
@BOTTOM@
/* Copyright (C) Bram Avontuur (bram@avontuur.org) */
