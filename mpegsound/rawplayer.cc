/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Rawplayer.cc
// Playing raw data with sound type.
// It's for only Linux

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "mpegsound.h"

/* IOCTL */
#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

char *Rawplayer::defaultdevice="/dev/dsp";

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
Rawplayer::~Rawplayer()
{
  close(audiohandle);
}

bool Rawplayer::initialize(char *filename)
{
  int flag;

  rawbuffersize=0;
  quota=0;

  if((audiohandle=open(filename,O_WRONLY|O_NDELAY,0))==-1)
    return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

  if((flag=fcntl(audiohandle,F_GETFL,0))<0)
    return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
  flag&=~O_NDELAY;
  if(fcntl(audiohandle,F_SETFL,flag)<0)
    return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

  IOCTL(audiohandle,SNDCTL_DSP_GETBLKSIZE,audiobuffersize);
  if(audiobuffersize<4 || audiobuffersize>65536)
    return seterrorcode(SOUND_ERROR_DEVBADBUFFERSIZE);

  return true;
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
  rawstereo=stereo;
  rawsamplesize=samplesize;
  rawspeed=speed;
  forcetomono=forceto8=false;

  return resetsoundtype();
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
    forcetomono=true;
  }

  tmp=rawsamplesize;
  IOCTL(audiohandle,SNDCTL_DSP_SAMPLESIZE,tmp);
  if(tmp!=rawsamplesize)
    if(rawsamplesize==16)
    {
      rawsamplesize=8;
      IOCTL(audiohandle,SNDCTL_DSP_SAMPLESIZE,rawsamplesize);
      if(rawsamplesize!=8)
	return seterrorcode(SOUND_ERROR_DEVCTRLERROR);

      forceto8=true;
    }

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
    if(forceto8)increment++,source++;

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
