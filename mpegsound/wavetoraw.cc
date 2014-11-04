/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Wavetoraw.cc
// Server which strips wave header.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "mpegsound.h"

#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif


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
	int c;	

  if(!buffer)
  {
    buffersize=player->getblocksize();
    if((buffer=(char *)malloc(buffersize))==NULL)
    {
      return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
    }
  }

	if( !(c=loader->getblock(buffer,sizeof(WAVEHEADER))) )
	{
		return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	}

	if (!testwave(buffer))
		return false;
	int ssize = (samplesize == 16 ? AFMT_S16_NE : AFMT_S8);
	if (!(player->setsoundtype(stereo, ssize, speed)))
		return false;
	currentpoint=0;
	initialized=true;
  return true;
}

bool Wavetoraw::run(void)
{
  int c;

	c=loader->getblock(buffer,buffersize);
	if(c==0)
	{
		return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	}

	currentpoint+=c;
	if(player->putblock(buffer,buffersize)==false)return false;

	if(currentpoint>=size)
	{
		return seterrorcode(SOUND_ERROR_FINISH);
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

  return seterrorcode(SOUND_ERROR_BAD);
}

