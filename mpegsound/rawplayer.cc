/* MPEG/WAVE Sound library

         (C) 1997 by Jung woo-jae
         Altered heavily by Bram Avontuur and others */

// Rawplayer.cc
// Playing raw data with sound type.
// Runs on linux and some flavours of bsd

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WANT_OSS

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"

#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif

/* IOCTL */
#ifdef SOUND_VERSION
#define IOCTL(a, b, c) ioctl(a, b, &c)
#else
#define IOCTL(a, b, c) (c = ioctl(a, b, c))
#endif

/* IOTRUBBEL : Massive debugging of my implementation of blocking writes..*/
#undef IOTRUBBEL

/* AUDIO_NONBLOCKING : If defined, non-blocking audio playback is used. */

const char *Rawplayer::defaultdevice = SOUND_DEVICE;

/* Volume */
int Rawplayer::setvolume(int volume) {
  int handle;
  int r;

  handle = open("/dev/mixer", O_RDWR);

  if (volume > 100)
    volume = 100;
  if (volume >= 0) {
    r = (volume << 8) | volume;

    ioctl(handle, MIXER_WRITE(SOUND_MIXER_VOLUME), &r);
  }
  ioctl(handle, MIXER_READ(SOUND_MIXER_VOLUME), &r);

  close(handle);

  return (r & 0xFF);
}

/*******************/
/* Rawplayer class */
/*******************/
// Rawplayer class
/* filename is stored in case audiohandle needs to be reopened */
Rawplayer::Rawplayer(const char *filename, int audiohandle, int audiobuffersize) {
  this->audiohandle = audiohandle;
  this->audiobuffersize = audiobuffersize;
  this->filename = strdup(filename);

  // need to do this when object of this class is used for more than one
  // mp3.
  rawstereo = rawsamplesize = rawspeed = want8bit = 0;
  rawbuffersize = 0;
  quota = 0;
  first_init = 1;
  /* !! Only read audiobuffersize (using getblocksize()) after a call to
   * setsoundtype()!!!!!!!! (OSS manual says so)
   */
}

Rawplayer::~Rawplayer() {
  if (audiohandle != -1) {
    close(audiohandle);
  }

  if (this->filename)
    free(this->filename);
}

int Rawplayer::getdevicehandle(const char *filename) {
  int audiohandle;

#if defined(AUDIO_NONBLOCKING) || defined(NEWTHREAD)
  if ((audiohandle = open(filename, O_WRONLY | O_NDELAY, 0)) == -1)
#else
  if ((audiohandle = open(filename, O_WRONLY, 0)) == -1)
#endif
  {
    return -1;
  }

#ifdef AIOSSIZE
  debug("AIOSSIZE defined\n");
  struct snd_size blocksize;
  blocksize.play_size = RAWDATASIZE * sizeof(short int);
  blocksize.rec_size = RAWDATASIZE * sizeof(short int);
  IOCTL(audiohandle, AIOSSIZE, blocksize);
#endif

  return audiohandle;
}

Rawplayer *Rawplayer::opendevice(const char *filename) {
  int audiohandle, audiobuffersize;

  if (!filename)
    filename = defaultdevice;

  audiohandle = getdevicehandle(filename);

  if (audiohandle == -1)
    return NULL;

  audiobuffersize = 1024; // it's properly set at setsoundtype().

  return new Rawplayer(filename, audiohandle, audiobuffersize);
}

void Rawplayer::abort(void) {
  int a;

  IOCTL(audiohandle, SNDCTL_DSP_RESET, a);
}

bool Rawplayer::setsoundtype(int stereo, int samplesize, int speed) {
  int changed = 0;
  int samplesize_oss = (samplesize == 16 ? AFMT_S16_NE : AFMT_U8);

  debug("Rawplayer::setsoundtype(%d,%d,%d)\n", stereo, samplesize, speed);
  if (rawstereo != stereo) {
    rawstereo = stereo;
    changed++;
  }
  if (rawsamplesize != samplesize_oss) {
    rawsamplesize = samplesize_oss;
    changed++;
  }
  if (rawspeed != speed) {
    rawspeed = speed;
    changed++;
  }
  forceto8 = (want8bit && samplesize == AFMT_S16_NE ? true : false);
  forcetomono = 0;
  if (changed)
    return resetsoundtype();
  return true;
}

bool Rawplayer::resetsoundtype(void) {
  int tmp;

  if (!first_init) {
    if (audiohandle == -1)
      return false;

    debug("Resetting soundcard! (%d, %d, %d)\n", rawstereo, rawsamplesize, rawspeed);
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
    /* OSS tells us to reopen the handle after a change in sample props */
    if (close(audiohandle) == -1) {
      debug("Couldn't close audiohandle\n");
      return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
    }
#if defined(AUDIO_NONBLOCKING) || defined(NEWTHREAD)
    if ((audiohandle = open(filename, O_WRONLY | O_NDELAY, 0)) == -1)
#else
    if ((audiohandle = open(filename, O_WRONLY, 0)) == -1)
#endif
    {
      debug("Couldn't reopen audiohandle\n");
      return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
    }
    debug("Soundcard successfully reopened.\n");
  } else
    first_init = 0;

  if (want8bit && rawsamplesize != AFMT_U8)
    forceto8 = true;

  /* OSS manual says: first samplesize, then channels, then speed */
  tmp = (want8bit ? AFMT_U8 : rawsamplesize);

  debug("Resetsound: samplesize = %d\n", tmp);
  IOCTL(audiohandle, SNDCTL_DSP_SAMPLESIZE, tmp);
  debug("Resetsound: samplesize = %d (2)\n", tmp);

  /* maybe the soundcard doesn't do 16 bits sound */
  if (!want8bit && tmp != rawsamplesize) {
    if (rawsamplesize == AFMT_S16_NE) {
      rawsamplesize = AFMT_U8; // most boxen use unsigned for 8bits!
      debug("Resetsound: samplesize_encore = %d\n", rawsamplesize);
      IOCTL(audiohandle, SNDCTL_DSP_SAMPLESIZE, rawsamplesize);
      debug("Resetsound: samplesize_encore(2) = %d\n", rawsamplesize);
      if (rawsamplesize != AFMT_U8)
        return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

      forceto8 = true;
    } else
      return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
  } else if (want8bit && tmp != AFMT_U8)
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

  /*
  if (want8bit)
          forceto8 = true;
  */

  debug("Resetsound: rawstereo=%d\n", rawstereo);
#ifdef SOUND_VERSION
  if (ioctl(audiohandle, SNDCTL_DSP_STEREO, &rawstereo) < 0)
#else
  if (rawstereo != ioctl(audiohandle, SNDCTL_DSP_STEREO, rawstereo))
#endif
  {
    // TODO: Now setsoundtype will *always* call resetsoundtype....
    rawstereo = MODE_MONO;
    forcetomono = 1;
  }

  debug("Resetsound: rawspeed=%d\n", rawspeed);
  if (IOCTL(audiohandle, SNDCTL_DSP_SPEED, rawspeed) < 0)
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

  /* NOW we can set audiobuffersize without breaking with OSS specs */
  IOCTL(audiohandle, SNDCTL_DSP_GETBLKSIZE, audiobuffersize);
  if (audiobuffersize < 4 || audiobuffersize > 65536)
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

  return true;
}

void Rawplayer::releasedevice() {
  if (audiohandle != -1) {
    /* abort(); <= prevents opening the sound device on my box! */
    if (close(audiohandle) == -1)
      debug("releasedevice: Couldn't close audiohandle!\n");
    audiohandle = -1;
  }
}

bool Rawplayer::attachdevice() {
  /* make sure it's closed.. */
  if (audiohandle != -1) {
    debug("attachdevice(): audiohandle wasn't closed?\n");
    releasedevice();
  }

  audiohandle = getdevicehandle(filename);
  if (audiohandle == -1)
    return false;

  first_init = 1;
  if (!resetsoundtype())
    return false;

  return true;
}

int Rawplayer::fix_samplesize(void *buffer, int size) {
  int modifiedsize = size;

  if (forcetomono || forceto8) {
    register unsigned char modify = 0;
    register unsigned char *source, *dest;
    int increment = 0, c;

    source = dest = (unsigned char *)buffer;

    if (forcetomono)
      increment++;
    if (forceto8) {
      increment++;
#ifndef WORDS_BIGENDIAN
      source++;
#endif
      modify = 128;
    }

    c = modifiedsize = size >> increment;
    increment <<= 1;

    while (c--) {
      *(dest++) = (*source) + modify;
      source += increment;
    }
  }
  return modifiedsize;
}

int Rawplayer::putblock_nt(void *buffer, int size) {
  if (audiohandle == -1)
    return -1;

  /* I'm not sure if pth_write is called for here, since we're doing non-blocked
     I/O anyway */

#ifdef LIBPTH
  return pth_write(audiohandle, buffer, size);
#else

  return write(audiohandle, buffer, size);

#endif
}

bool Rawplayer::putblock(void *buffer, int size) {
  int modifiedsize = size;

  if (audiohandle == -1)
    return false;

  if (forcetomono || forceto8) {
    register unsigned char modify = 0;
    register unsigned char *source, *dest;
    int increment = 0, c;

    source = dest = (unsigned char *)buffer;

    if (forcetomono)
      increment++;
    if (forceto8) {
      increment++;
#ifndef WORDS_BIGENDIAN
      source++;
#endif
      modify = 128;
    }

    c = modifiedsize = size >> increment;
    increment <<= 1;

    while (c--) {
      *(dest++) = (*source) + modify;
      source += increment;
    }
  }

#if defined(AUDIO_NONBLOCKING) || defined(NEWTHREAD)
  register ssize_t wsize, remainsize = modifiedsize;
  char *wbuf = (char *)buffer;

#ifdef IOTRUBBEL
  int loop = 0;
#endif

  while (remainsize > 0 && (wsize = write(audiohandle, (void *)wbuf, remainsize)) != remainsize) {
#ifdef IOTRUBBEL
    loop++;
    if (loop > 1) {
      char fiepje[255];
      sprintf(fiepje, "Partial/bad write, wsize=%d remainsize=%d wbuf=%d (%d)\n", wsize, remainsize - wsize, (int)wbuf,
              loop);
      debug(fiepje);
    }
#endif
    if (wsize > 0) {
      remainsize -= wsize;
      wbuf += wsize;
    }
    USLEEP(10000); // always sleep a bit at bad/partial writes..
  }
#ifdef IOTRUBBEL
  if (loop > 1)
    debug("Done (write)\n");
#endif
#else /* AUDIO_NONBLOCKING */
#ifdef LIBPTH
  pth_write(audiohandle, buffer, modifiedsize);
#else
  write(audiohandle, buffer, modifiedsize);
#endif
#endif /* AUDIO_NONBLOCKING */

  return true;
}

int Rawplayer::getblocksize(void) { return audiobuffersize; }

#endif /* WANT_OSS */
