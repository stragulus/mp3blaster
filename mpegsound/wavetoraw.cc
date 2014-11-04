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

#ifdef WORDS_BIGENDIAN
typedef union {
    long arg;
    char byte_represent[4];
} endian_hack_1;

typedef union {
    short arg;
    char byte_represent[2];
} endian_hack_2;

inline short HOST_TO_LE16(short x)
{
    endian_hack_2 in,out;
    in.arg=x;
    out.arg=0;
    out.byte_represent[0]=in.byte_represent[1];
    out.byte_represent[1]=in.byte_represent[0];
    return (short)out.arg;
}
inline int HOST_TO_LE32(int x)
{
    endian_hack_1 in,out;
    in.arg=x;
    out.arg=0;
    out.byte_represent[0]=in.byte_represent[3];
    out.byte_represent[1]=in.byte_represent[2];
    out.byte_represent[2]=in.byte_represent[1];
    out.byte_represent[3]=in.byte_represent[0];
    return out.arg;
}

#else
#define HOST_TO_LE16(x)  (x)
#define HOST_TO_LE32(x)  (x)
#endif
Wavetoraw::Wavetoraw(Soundinputstream *loader,Soundplayer *player)
{
  __errorcode=SOUND_ERROR_OK;
  initialized=false;buffer=NULL;
  this->loader=loader;this->player=player;
};

Wavetoraw::~Wavetoraw()
{
  if (buffer)free(buffer);
};


// Convert wave format to raw format class
bool Wavetoraw::initialize(void)
{
	int c;	

	char tmpbuffer[1024];
	if ( !(c=loader->getblock(tmpbuffer,sizeof(WAVEHEADER))) )
	{
		return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	}

	if (!testwave(tmpbuffer))
		return false;
	int ssize = (samplesize == 16 ? AFMT_S16_LE : AFMT_U8);
	if (!(player->setsoundtype(stereo, ssize, speed)))
		return false;

  if (!buffer)
  {
    buffersize=player->getblocksize();
		if (buffersize < (int)sizeof(WAVEHEADER))
			buffersize = sizeof(WAVEHEADER);
    if ((buffer=(char *)malloc(buffersize * sizeof(char)))==NULL)
    {
      return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
    }
  }

	currentpoint=0;
	initialized=true;
  return true;
}

bool Wavetoraw::run(void)
{
  int c;

	c=loader->getblock(buffer,buffersize);
	if (c==0)
	{
		return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	}

	currentpoint+=c;
	if (player->putblock(buffer,buffersize) == false)
		return false;

	if (currentpoint >= size)
	{
		return seterrorcode(SOUND_ERROR_FINISH);
	}

  return true;
}

void Wavetoraw::setcurrentpoint(int p)
{
  if (p*pcmsize>size)currentpoint=size;
  else currentpoint=p*pcmsize;
  loader->setposition(currentpoint+sizeof(WAVEHEADER));
}

bool Wavetoraw::testwave(char *buffer)
{
  WAVEHEADER *tmp = (WAVEHEADER *)buffer;

	if (tmp->main_chunk==RIFF && tmp->chunk_type==WAVE &&
		tmp->sub_chunk==FMT && tmp->data_chunk==DATA)
	{
		if (tmp->format == PCM_CODE && (tmp->modus == WAVE_STEREO ||
			tmp->modus == WAVE_MONO))
		{
			stereo = (tmp->modus==WAVE_STEREO) ? 1 : 0;
			samplesize = HOST_TO_LE16((int)(tmp->bit_p_spl));
			speed = HOST_TO_LE32((int)(tmp->sample_fq));
			size = HOST_TO_LE32((int)(tmp->data_length));
			pcmsize=1;
			if (stereo == 1)
				pcmsize *= 2;
			if (samplesize == 16)
				pcmsize*=2;
			return true;
		}
	}

	return seterrorcode(SOUND_ERROR_BAD);
}

