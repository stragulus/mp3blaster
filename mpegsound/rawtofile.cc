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
  if (filetype == WAV)
  {
    off_t filelen = lseek(filehandle, 0, SEEK_CUR);
    lseek(filehandle, 0, SEEK_SET);
    hdr.length = (u_int32_t)filelen; /* file length */
	hdr.data_length = (u_int32_t)(filelen - 36);
	write(filehandle, &hdr, sizeof(hdr));
  }
  close(filehandle);
}

bool Rawtofile::initialize(char *filename)
{
  if(filename==NULL)filehandle=1;
  else if((filehandle=creat(filename,0644))==-1)
    return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

  init_putblock = 1;
  return true;
}

bool Rawtofile::setsoundtype(int stereo,int samplesize,int speed)
{
  rawstereo=stereo;
  rawsamplesize=samplesize;
  rawspeed=speed;

  return true;
}

/* set type of file to write. Default: RAW (no header) */
bool Rawtofile::setfiletype(soundtype filetype)
{
	if (filetype != RAW && filetype != WAV)
		return false;

	this->filetype = filetype;	
	return true;
}

bool Rawtofile::putblock(void *buffer,int size)
{
  if (init_putblock && filetype != RAW)
  {
    if (filetype == WAV)
	{
	  //initial datasize = 0...when all data is written, determine filesize
	  //and rewrite the header.
      hdr.main_chunk = RIFF;
      hdr.length = 0 + 36; /* file length */
      hdr.chunk_type = WAVE;
      hdr.sub_chunk = FMT;
      hdr.sc_len = rawsamplesize; /* = 16 */
      hdr.format = PCM_CODE;
      hdr.modus = (rawstereo ? WAVE_STEREO : WAVE_MONO); /* stereo, 1 = mono */
      hdr.sample_fq = rawspeed; /* sample frequency */
      hdr.byte_p_sec = (rawspeed * (rawsamplesize/8) * (rawstereo?2:1)); 
      hdr.byte_p_spl = (rawstereo?2:1) * (rawsamplesize/8); 
      hdr.bit_p_spl = rawsamplesize; /* 8, 12, or 16 bit */
      hdr.data_chunk = DATA;
      hdr.data_length = 0; /* file length without this header */
	  if (write(filehandle, &hdr, sizeof(hdr)) != sizeof(hdr))
	  	return false;
	}
  }
  init_putblock = 0;
  return (write(filehandle,buffer,size)==size);
};

