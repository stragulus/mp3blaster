#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef WANT_ESD

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#ifdef HAVE_ERRNO_H
#  include	<errno.h>
#endif /* HAVE_ERRNO_H */
#include	<ctype.h>
#include	<fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <esd.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"

//blocksize to use when writing to esd
#define ESD_BLOCK_SIZE 32768

EsdPlayer::EsdPlayer(const char *host)
{
	this->esd_handle = -1;
	this->host = host;
}

EsdPlayer::~EsdPlayer()
{
	if (esd_handle > -1)
	{
		(void)esd_close(esd_handle);
	}
}

void
EsdPlayer::abort()
{
	//doesn't seem like esd can be made to flush any caches.
}

bool
EsdPlayer::setsoundtype(int stereo, int samplesize, int speed)
{
	if (esd_handle != -1)
	{
		debug("setsoundtype() called while esd_handle exists");
		seterrorcode(SOUND_ERROR_UNKNOWN);
		return false; //can only set soundtype once
	}

	rawstereo = stereo;
	rawsamplesize = samplesize;
	rawspeed = speed;

	return resetsoundtype();
}

void
EsdPlayer::set8bitmode()
{
	//ignore forced 8bits output requests ; esd must downsample itself.
}

bool
EsdPlayer::resetsoundtype()
{
	int esd_format = 0;

	debug("esdplayer::resetsoundtype\n");
	esd_format |= (rawstereo ? ESD_STEREO : ESD_MONO);
	esd_format |= (rawsamplesize == 8 ? ESD_BITS8 : ESD_BITS16);

	if (esd_handle != -1)
	{
		//connection still exists. close it.
		(void)esd_close(esd_handle);
	}

	//[re]open connection to esd
	esd_handle = esd_play_stream(esd_format, rawspeed, NULL, "mp3blaster");

	if (esd_handle < 0)
	{
		debug("esd_play_stream failed: %s\n", strerror(errno));
		seterrorcode(SOUND_ERROR_DEVOPENFAIL);
		return false;
	}

	return true;
}

void 
EsdPlayer::releasedevice()
{
	/* releasing sound device is pointless when using a sound daemon */
}

bool
EsdPlayer::attachdevice()
{
	/* releasing sound device is pointless when using a sound daemon */
	return true;
}

bool
EsdPlayer::putblock(void *buffer, int size)
{
	int bytes_written = 0;

	if (esd_handle < 0)
		return false;
	
#if defined(AUDIO_NONBLOCKING) || defined(NEWTHREAD)
	while (bytes_written != size)
	{
		int tmp_bytes_written = putblock_nt((char*)buffer + bytes_written,
			size - bytes_written);

		if (tmp_bytes_written < 0)
		{
			seterrorcode(SOUND_ERROR_DEVBUSY);
			return false;
		}
		bytes_written += tmp_bytes_written;
	}
#else
	bytes_written = putblock_nt(buffer, size);	

	if (bytes_written < 0)
	{
		seterrorcode(SOUND_ERROR_DEVBUSY);
		return false;
	}
#endif

	return true;
}

int
EsdPlayer::putblock_nt(void *buffer, int size)
{
#ifdef LIBPTH
	return pth_write(esd_handle, buffer, size);
#else
	return write(esd_handle, buffer, size);
#endif
}

int
EsdPlayer::getblocksize()
{
	return ESD_BLOCK_SIZE; //XXX: config?
}

int
EsdPlayer::fix_samplesize(void *buffer, int size)
{
	(void)buffer;

	//esd takes care of downsampling, no need to mangle samples
	return size;
}
#endif /* WANT_ESD */
