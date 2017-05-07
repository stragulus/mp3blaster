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
#include "mpegsound_locals.h"

#ifdef __cplusplus
extern "C" {
#endif

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
// Rawplayer class
Rawtofile::~Rawtofile()
{
	if (filetype == ST_WAV)
	{
		off_t filelen = lseek(audiohandle, 0, SEEK_CUR);
		lseek(audiohandle, 0, SEEK_SET);
		hdr.length = HOST_TO_LE32((u_int32_t)(filelen-8)); /* file length */
		hdr.data_length = HOST_TO_LE32((u_int32_t)(filelen - 44));
		(void)write(audiohandle, &hdr, sizeof(hdr));
	}
	close(audiohandle);
}

Rawtofile::Rawtofile(int audiohandle)
{
	this->audiohandle = audiohandle;
	init_putblock = 1;
}

Rawtofile *Rawtofile::opendevice(const char *filename)
{
	int audiohandle;
	if(filename==NULL)audiohandle=1;
	else if((audiohandle=creat(filename,0644))==-1)
		return NULL;
	if (isatty(audiohandle)) return NULL;

	return new Rawtofile(audiohandle);
}

bool Rawtofile::setsoundtype(int stereo,int samplesize,int speed)
{
	static bool myinit = false;
	/* changing sample specs when writing to a file is not done! */
	if (myinit && (
		(rawstereo != stereo) ||
		(rawsamplesize != samplesize) ||
		(rawspeed != speed)))
	{
		debug("Change in sample size/speed/mode.\n");
		return false;
	}
	else
		myinit = true;

	rawstereo=stereo;
	rawsamplesize=samplesize;
	rawspeed=speed;

	return true;
}

/* set type of file to write. Default: ST_RAW (no header) */
bool Rawtofile::setfiletype(soundtype filetype)
{
	if (filetype != ST_RAW && filetype != ST_WAV)
		return false;

	this->filetype = filetype;	
	return true;
}
int Rawtofile::putblock_nt(void *buffer, int size)
{
	if (init_putblock && filetype != ST_RAW)
	{
		int wordsize;
	
		wordsize = rawsamplesize;

		if (filetype == ST_WAV)
		{
		//initial datasize = 0...when all data is written, determine filesize
		//and rewrite the header.
			hdr.main_chunk = RIFF;
			hdr.length = HOST_TO_LE32(0 + 36); /* file length */
			hdr.chunk_type = WAVE;
			hdr.sub_chunk = FMT;
			hdr.sc_len = HOST_TO_LE32(wordsize); /* = 16 */
			hdr.format = PCM_CODE;
			hdr.modus = (rawstereo ? WAVE_STEREO : WAVE_MONO); /* stereo, 1 = mono */
			hdr.sample_fq = HOST_TO_LE32(rawspeed); /* sample frequency */
			hdr.byte_p_sec = HOST_TO_LE32((rawspeed * (wordsize/8) * (rawstereo?2:1)));
			hdr.byte_p_spl = HOST_TO_LE16((rawstereo?2:1) * (wordsize/8));
			hdr.bit_p_spl = HOST_TO_LE16(wordsize); /* 8, 12, or 16 bit */
			hdr.data_chunk = DATA;
			hdr.data_length = 0; /* file length without this header */
			if (write(audiohandle, &hdr, sizeof(hdr)) != sizeof(hdr))
				return false;
		}
	}
	init_putblock = 0;
#ifdef WORDS_BIGENDIAN
	if (rawsamplesize == 16)
	{
		/* big endian -> switch bytes in output file */
		unsigned short *sh_buffer=(unsigned short *)(buffer);
		int modifiedsize=size/2;
		/* data is in 16 bits bigendian, target is 16 bits little endian. */
		while(modifiedsize--)
		{
			sh_buffer[modifiedsize]=HOST_TO_LE16(sh_buffer[modifiedsize]);
		}
	}
#endif
	return write(audiohandle,buffer,size);
}

bool Rawtofile::putblock(void *buffer,int size)
{
	return putblock_nt(buffer,size)==size;
};

