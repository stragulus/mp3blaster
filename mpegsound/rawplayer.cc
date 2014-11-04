/* MPEG/WAVE Sound library

	 (C) 1997 by Jung woo-jae 
	 Altered heavily by Bram Avontuur and others */

// Rawplayer.cc
// Playing raw data with sound type.
// Runs on linux and some flavours of bsd

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "mpegsound.h"

#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif


/* IOCTL */
#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

/* IOTRUBBEL : Massive debugging of my implementation of blocking writes..*/
#undef IOTRUBBEL
extern void debug(const char *);

/* AUDIO_NONBLOCKING : If defined, non-blocking audio playback is used. */

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

#ifdef AUDIO_NONBLOCKING
	flag|=O_NDELAY; //don't block!
	debug("Using non-blocking audio writes. This might hurt the sound.\n");
#endif

	if(fcntl(audiohandle,F_SETFL,flag)<0)
		return NULL;

#ifdef AIOSSIZE
	debug("AIOSSIZE defined\n");
	struct snd_size blocksize;
	blocksize.play_size = RAWDATASIZE * sizeof(short int);
	blocksize.rec_size = RAWDATASIZE * sizeof(short int);
	IOCTL(audiohandle,AIOSSIZE, blocksize);
#endif

	IOCTL(audiohandle,SNDCTL_DSP_GETBLKSIZE,audiobuffersize);
	if(audiobuffersize<4 || audiobuffersize>65536)
		return NULL;

	int fragsize = (16<<16) | 10;
	ioctl(audiohandle, SNDCTL_DSP_SETFRAGMENT, &fragsize);

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
	{
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
	}
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
		if(forceto8)
		{
			increment++;
#ifndef WORDS_BIGENDIAN
			source++;
#endif
			modify=128;
		}

		c=modifiedsize=size>>increment;
		increment<<=1;

		while(c--)
		{
			*(dest++)=(*source)+modify;
			source+=increment;
		}
	}

#if 0
	if(quota)
		while(getprocessed()>quota)USLEEP(3);
#endif

#ifdef AUDIO_NONBLOCKING
	register ssize_t
		wsize,
		remainsize = modifiedsize;
	char *wbuf = (char*)buffer;

/* In an ideal world, one would use select() to see if the device is ready
 * to accept data. However, this doesn't seem to work very well with sound
 * devices so I left it out. One the one FreeBSD box where I got this to 
 * work, it didn't consume significantly less CPU time compared to using
 * usleep() to wait between writes. On my box, select() on the sound device
 * seems broken (it never appears ready).
 */
#if 0
	struct fd_set writefds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

	FD_ZERO(&writefds);
	FD_SET(audiohandle, &writefds);
	int selectval;
	
	while ((selectval = select(audiohandle + 1, NULL, &writefds, NULL, &tv)) !=
		1)
	{
		char flupje[255];
		sprintf(flupje, "Selectval is: %d\n", selectval);
		debug(flupje);
	}
#endif

#ifdef IOTRUBBEL
	int loop = 0;
#endif

	while(remainsize > 0 && (wsize = write(audiohandle,(void*)wbuf,remainsize)) !=
		remainsize)
	{
#ifdef IOTRUBBEL
		loop++;
		if (loop > 1)
		{
			char fiepje[255];
			sprintf(fiepje,"Partial/bad write, wsize=%d remainsize=%d wbuf=%d (%d)\n",
				wsize, remainsize-wsize,(int)wbuf, loop);
			debug(fiepje);
		}
#endif
		if (wsize > 0)
		{
			remainsize -= wsize;
			wbuf += wsize;
		}
		USLEEP(10000); //always sleep a bit at bad/partial writes..
	}
#ifdef IOTRUBBEL
	if (loop > 1)
		debug("Done (write)\n");
#endif
#else /* AUDIO_NONBLOCKING */ 
#ifdef LIBPTH
	pth_write(audiohandle, buffer, modifiedsize);
#else
	write(audiohandle, buffer, modifiedsize);
#endif
#endif /* AUDIO_NONBLOCKING */

	return true;
}

int Rawplayer::getblocksize(void)
{
	return audiobuffersize;
}
