/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Binput.cc
// Inputstream from file

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <unistd.h>

#include "mpegsound.h"

/************************/
/* Input bitstrem class */
/************************/

Soundinputstreamfromfile::Soundinputstreamfromfile()
{
	fp = NULL;
	__canseek = true;
}

Soundinputstreamfromfile::~Soundinputstreamfromfile()
{
  if(fp)
  	fclose(fp);
}

bool Soundinputstreamfromfile::open(const char *filename)
{
  struct stat buf;

  if(filename==NULL)
  {
    fp=stdin;
    size=0;
    return true;
  }
  else if((fp=fopen(filename,"r"))==NULL)
  {
    seterrorcode(SOUND_ERROR_FILEOPENFAIL);
    return false;
  }

  stat(filename,&buf);
  size=buf.st_size;
	if (size < 10 * 1024)
	{
		seterrorcode(SOUND_ERROR_FILEOPENFAIL);
		return false;
	}

  return true;
}

int Soundinputstreamfromfile::getbytedirect(void)
{
  int c;

  if((c=getc(fp))<0)
    seterrorcode(SOUND_ERROR_FILEREADFAIL);

  return c;
}

bool Soundinputstreamfromfile::_readbuffer(char *buffer,int size)
{
  if(fread(buffer,size,1,fp)!=1)
  {
    seterrorcode(SOUND_ERROR_FILEREADFAIL);
    return false;
  }
  return true;
}

bool Soundinputstreamfromfile::eof(void)
{
  return feof(fp);
};

int Soundinputstreamfromfile::getblock(char *buffer,int size)
{
  return fread(buffer,1,size,fp);
}

int Soundinputstreamfromfile::getsize(void)
{
  return size;
}

void Soundinputstreamfromfile::setposition(int pos)
{
  if(fp==stdin)return;
  fseek(fp,(long)pos,SEEK_SET);
}

int  Soundinputstreamfromfile::getposition(void)
{
  if(fp==stdin)return 0;
  return (int)ftell(fp);
}


void Soundinputstreamfromfile::close(void)
{
	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}
}
