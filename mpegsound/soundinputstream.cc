/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Soundinputstream.cc
// Abstractclass of inputstreams

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "mpegsound.h"

Soundinputstream::Soundinputstream()
{
  __errorcode=SOUND_ERROR_OK;
};

Soundinputstream::~Soundinputstream()
{
  // Nothing...
};

/********************/
/* File & Http open */
/********************/
Soundinputstream *Soundinputstream::hopen(const char *filename,int *errorcode)
{
	Soundinputstream *st;

	if (filename==NULL)
		st=new Soundinputstreamfromfile;
	else if (strstr(filename,"://"))
		st=new Soundinputstreamfromhttp;
	else
		st=new Soundinputstreamfromfile;

	if (st==NULL)
	{
		*errorcode=SOUND_ERROR_MEMORYNOTENOUGH;
		return NULL;
	}

	if (!st->open(filename))
	{
		*errorcode=st->geterrorcode();
		delete st;
		return NULL;
	}

	return st;
}
