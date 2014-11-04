/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Rawtofile.cc
// Output raw data to file.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <unistd.h>

#include "mpegsound.h"

// Rawplayer class
Rawtofile::~Rawtofile()
{
  close(filehandle);
}

bool Rawtofile::initialize(char *filename)
{
  if(filename==NULL)filehandle=1;
  else if((filehandle=creat(filename,0644))==-1)
    return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

  return true;
}

bool Rawtofile::setsoundtype(int stereo,int samplesize,int speed)
{
  rawstereo=stereo;
  rawsamplesize=samplesize;
  rawspeed=speed;

  return true;
}

bool Rawtofile::putblock(void *buffer,int size)
{
  return (write(filehandle,buffer,size)==size);
};

