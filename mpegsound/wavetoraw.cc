/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Wavetoraw.cc
// Server which strips wave header.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <malloc.h>

#include "mpegsound.h"

Wavetoraw::Wavetoraw(Soundinputstream *loader,Soundplayer *player)
{
  __errorcode=SOUND_ERROR_OK;
  initialized=false;buffer=NULL;
  this->loader=loader;this->player=player;
};

Wavetoraw::~Wavetoraw()
{
  if(buffer)free(buffer);
};


// Convert wave format to raw format class
bool Wavetoraw::initialize(void)
{
  if(!buffer)
  {
    buffersize=player->getblocksize();
    if((buffer=(char *)malloc(buffersize))==NULL)
    {
      seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
      return false;
    }
  }
  return true;
}

bool Wavetoraw::run(void)
{
  int c;

  if(initialized)
  {
    c=loader->getblock(buffer,buffersize);
    if(c==0)
    {
      seterrorcode(SOUND_ERROR_FILEREADFAIL);
      return false;
    }

    currentpoint+=c;
    if(player->putblock(buffer,buffersize)==false)return false;

    if(currentpoint>=size)
    {
      seterrorcode(SOUND_ERROR_FINISH);
      return false;
    }
  }
  else
  {
    c=loader->getblock(buffer,sizeof(WAVEHEADER));
    if(c==0)
    {
      seterrorcode(SOUND_ERROR_FILEREADFAIL);
      return false;
    }

    if(!testwave(buffer))return false;
    if(player->setsoundtype(stereo,samplesize,speed)==false)return false;
    currentpoint=0;
    initialized=true;
  }
  return true;
}

void Wavetoraw::setcurrentpoint(int p)
{
  if(p*pcmsize>size)currentpoint=size;
  else currentpoint=p*pcmsize;
  loader->setposition(currentpoint+sizeof(WAVEHEADER));
}

bool Wavetoraw::testwave(char *buffer)
{
  WAVEHEADER *tmp=(WAVEHEADER *)buffer;

  if(tmp->main_chunk==RIFF && tmp->chunk_type==WAVE &&
     tmp->sub_chunk==FMT && tmp->data_chunk==DATA)
    if(tmp->format==PCM_CODE && tmp->modus<=2)
    {
      stereo=(tmp->modus==WAVE_STEREO) ? 1 : 0;
      samplesize=(int)(tmp->bit_p_spl);
      speed=(int)(tmp->sample_fq);
      size =(int)(tmp->data_length);

      pcmsize=1;
      if(stereo==1)pcmsize*=2;
      if(samplesize==16)pcmsize*=2;
      return true;
    }

  seterrorcode(SOUND_ERROR_BAD);
  return false;
}

