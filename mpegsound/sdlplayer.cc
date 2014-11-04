/* Class to stream raw audio data to audio device using SDL */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef WANT_SDL

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

#include <SDL.h>
#include <SDL_audio.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"
#include "cyclicbuffer.h"

#define SDL_BLOCK_SIZE 32768
//number of seconds of audio to keep in local buffer
#define BUFFER_SECONDS 2

int SDLPlayer::sdl_init = 0;

SDLPlayer::SDLPlayer()
{
	this->buffer = NULL;
}

SDLPlayer::~SDLPlayer()
{
	if (this->buffer)
	{
		SDL_CloseAudio(); //!important! before deleting buffer
		delete buffer;
	}
}

void
SDLPlayer::abort()
{
	debug("sdl::abort\n");
	//clear buffer
	if (this->buffer)
	{
		buffer->setEmpty();
	}
}

bool
SDLPlayer::setsoundtype(int stereo, int samplesize, int speed)
{
	debug("sdl::setsoundtype()\n");

	if (buffer != NULL)
	{
		debug("setsoundtype() called while buffer exists");
		seterrorcode(SOUND_ERROR_UNKNOWN);
		return false; //can only set soundtype once
	}

	if (!sdl_init && SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		debug("SDL_Init failed: %s\n", SDL_GetError());
		seterrorcode(SOUND_ERROR_DEVOPENFAIL);
		return false;
	}

	sdl_init = 1; //static;only necessary once for the application's life span.

	rawstereo = stereo;
	rawsamplesize = samplesize;
	rawspeed = speed;

	return resetsoundtype();
}

void
SDLPlayer::set8bitmode()
{
	//ignore forced 8bits output requests ; sdl will downsample itself.
}

//sdl callback function
void
SDLCallback(void *localbuffer, Uint8 *stream, int len)
{
	CyclicBuffer *myBuffer = (CyclicBuffer*)localbuffer;

	//blocks on read. I hope the SDL_CloseAudio() properly kills the thread
	//that's waiting on this read..
	(void)myBuffer->read((unsigned char * const)stream, (unsigned int)len);
}

bool
SDLPlayer::resetsoundtype()
{
	SDL_AudioSpec fmt;

	if (this->buffer)
	{
		SDL_CloseAudio();
		delete buffer;
	}

	buffer = new CyclicBuffer(BUFFER_SECONDS * rawspeed * (rawstereo ? 2 : 1) *
		(rawsamplesize / 8));

	//(re)set audiospec
	fmt.channels = (rawstereo ? 2 : 1);
	fmt.freq = rawspeed;
	fmt.format = (rawsamplesize == 8 ? AUDIO_S8 : AUDIO_S16SYS);
	fmt.callback = SDLCallback;
	fmt.userdata = buffer;
	fmt.samples = 2048; //bigger: less hicks, more latency, and vice versa

	/* open sdl audio dev */
	if (SDL_OpenAudio(&fmt, NULL) < 0)
	{
		debug("sdl_openaudio() failed: %s", SDL_GetError());
		seterrorcode(SOUND_ERROR_DEVOPENFAIL);
		delete buffer;
		buffer = NULL;
		return false;
	}

	SDL_PauseAudio(0); //default is to start paused
	return true;
}

void 
SDLPlayer::releasedevice()
{
	if (buffer)
	{
		SDL_PauseAudio(1); //hopefully releases audiodev.
	}
}

bool
SDLPlayer::attachdevice()
{
	if (buffer)
	{
		SDL_PauseAudio(0);
	}
	
	return true;
}

//write all of buffer's content (blocking)
bool
SDLPlayer::putblock(void *buffer, int size)
{
	unsigned int counter = 0;

	if (!this->buffer)
		return false;
	
	if (!size)
	{
		return true; //writing nothing is really fast.
	}

	//potential race condition if sdl doesn't read from the buffer.
	while (0 == this->buffer->write((const unsigned char * const)buffer,
		(unsigned int)size) && counter < 1000)
	{
		usleep(10000); //sleep 10 millisec
		++counter;
	}

	if (1000 == counter)
	{
		//waited 20 seconds. I call that a race condition.
		debug("Timeout while trying to write %d bytes to cyclic buffer\n", size);
		return seterrorcode(SOUND_ERROR_DEVBUSY);
	}

	return true;
}

int
SDLPlayer::putblock_nt(void *buffer, int size)
{
	if (!buffer)
	{
		return -1;
	}

	//potentially partial write
	return this->buffer->write((const unsigned char * const)buffer, (unsigned int)size,
		1);
}

int
SDLPlayer::getblocksize()
{
	return SDL_BLOCK_SIZE; //XXX: config?
}

int
SDLPlayer::fix_samplesize(void *buffer, int size)
{
	(void)buffer;

	//sdl takes care of downsampling, no need to mangle samples
	return size;
}
#endif /* WANT_SDL */
