/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Rawplayer.cc
// Playing raw data with sound type.
// It's for only Linux
// (Bram)Ahem..also for FreeBSD & OpenBSD & perhaps more.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#else
#include <soundcard.h>
#endif

#ifdef __cplusplus
}
#endif

#include "mpegsound.h"

/* IOCTL */
#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

char *Rawplayer::defaultdevice=SOUND_DEVICE;

/* Volume */
int Rawplayer::setvolume(int volume)
{
  int handle;
  int r;

  handle=open("/dev/mixer",O_RDWR);

  if(volume>100)volume=100;
  if(volume>=0)
  {
    r=(volume<<8) | volume;

    ioctl(handle,MIXER_WRITE(SOUND_MIXER_VOLUME),&r);
  }
  ioctl(handle,MIXER_READ(SOUND_MIXER_VOLUME),&r);

  close(handle);

  return (r&0xFF);
}

/*******************/
/* Rawplayer class */
/*******************/
// Rawplayer class
Rawplayer::Rawplayer(int audiohandle, int audiobuffersize)
{
	this->audiohandle = audiohandle;
	this->audiobuffersize = audiobuffersize;
	//need to do this when object of this class is used for more than one
	//mp3.
	rawstereo = rawsamplesize = rawspeed = want8bit = 0;
	rawbuffersize = 0;
	quota = 0;
}

Rawplayer::~Rawplayer()
{
	close(audiohandle);
}

Rawplayer *Rawplayer::opendevice(char *filename)
{
  int flag;
  int audiohandle, audiobuffersize;

  if (!filename) filename = defaultdevice;

  if((audiohandle=open(filename,O_WRONLY|O_NDELAY,0))==-1)
    return NULL;

  if((flag=fcntl(audiohandle,F_GETFL,0))<0)
    return NULL;
  flag&=~O_NDELAY;
  if(fcntl(audiohandle,F_SETFL,flag)<0)
    return NULL;

  IOCTL(audiohandle,SNDCTL_DSP_GETBLKSIZE,audiobuffersize);
  if(audiobuffersize<4 || audiobuffersize>65536)
    return NULL;

  return new Rawplayer(audiohandle, audiobuffersize);
}

void Rawplayer::abort(void)
{
  int a;

  IOCTL(audiohandle,SNDCTL_DSP_RESET,a);
}

int Rawplayer::getprocessed(void)
{
  audio_buf_info info;
  int r;

  IOCTL(audiohandle,SNDCTL_DSP_GETOSPACE,info);

  r=(info.fragstotal-info.fragments)*info.fragsize;

  return r;
}

bool Rawplayer::setsoundtype(int stereo,int samplesize,int speed)
{
  int changed = 0;

  if (rawstereo != stereo) { rawstereo = stereo; changed++; }
  if (rawsamplesize != samplesize) { rawsamplesize = samplesize; changed++; }
  if (rawspeed != speed) { rawspeed = speed; changed++; }
  forceto8 = (want8bit ? true : false);
  forcetomono = 0;
  if (changed)
  	return resetsoundtype();
  return true;
}

bool Rawplayer::resetsoundtype(void)
{
  int tmp;

  if(ioctl(audiohandle,SNDCTL_DSP_SYNC,NULL)<0)
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

#ifdef SOUND_VERSION
  if(ioctl(audiohandle,SNDCTL_DSP_STEREO,&rawstereo)<0)
#else
  if(rawstereo!=ioctl(audiohandle,SNDCTL_DSP_STEREO,rawstereo))
#endif
  {
    rawstereo=MODE_MONO;
    forcetomono=1;
  }

  tmp = ( want8bit ? 8 : rawsamplesize);

  IOCTL(audiohandle,SNDCTL_DSP_SAMPLESIZE,tmp);
  if(!want8bit && tmp!=rawsamplesize)
    if(rawsamplesize==16)
    {
      //rawsamplesize=8; 
	  rawsamplesize=AFMT_S8; //most soundcards used SIGNED for 8bits!
      IOCTL(audiohandle,SNDCTL_DSP_SAMPLESIZE,rawsamplesize);
      if(rawsamplesize!=AFMT_S8)
	return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

      forceto8=true;
    }
  else if (want8bit && tmp != 8)
  	return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

  if (want8bit)
    forceto8 = true;

  if(IOCTL(audiohandle,SNDCTL_DSP_SPEED,rawspeed)<0)
    return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

  return true;
}

bool Rawplayer::putblock(void *buffer,int size)
{
  int modifiedsize=size;

  if(forcetomono || forceto8)
  {
    register unsigned char modify=0;
    register unsigned char *source,*dest;
    int increment=0,c;

    source=dest=(unsigned char *)buffer;

    if(forcetomono)increment++;
    if(forceto8)increment++,source++,modify=128;

    c=modifiedsize=size>>increment;
    increment<<=1;

    while(c--)
    {
      *(dest++)=(*source)+modify;
      source+=increment;
    }
  }

  if(quota)
    while(getprocessed()>quota)usleep(3);
  write(audiohandle,buffer,modifiedsize);

  return true;
}

int Rawplayer::getblocksize(void)
{
  return audiobuffersize;
}
