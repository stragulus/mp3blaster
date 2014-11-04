/* (C) Bram Avontuur, bram@mp3blaster.avontuur.org
 * For more information about OggVorbis, check http://www.xiph.org/ogg/vorbis/
 */
#include <mpegsound.h>
#include "mpegsound_locals.h"

#ifdef INCLUDE_OGG

#ifdef HAVE_BOOL_H
#include <bool.h>
#endif
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <string.h>
#include SOUNDCARD_HEADERFILE

Oggplayer::Oggplayer()
{
	of = NULL;
	wordsize = 2; //2 bytes
	bigendian = 0;
/* Martijn suggests that big endiannes is already taken care of in the rawplayer
 * class.
 */
#if 0 && defined(WORDS_BIGENDIAN)
	bigendian = 1;
#endif
	//TODO: On what hardware is data unsigned, and how do I know?
	signeddata = 1;
	mono = 0;
	downfreq = 0;
	resetsound = 1;
}

Oggplayer::~Oggplayer()
{
	if (of)
	{
		ov_clear(of);
		delete of;
	}
}

bool Oggplayer::openfile(char *filename, char *device, soundtype write2file)
{
	FILE *f;
	int openres;
	if (of)
	{
		ov_clear(of);
		delete of;
	}

	of = new OggVorbis_File;

	//initialize player object that writes to sound device/file
	if (!opendevice(device, write2file))
		return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
	
	if (!(f = fopen(filename, "r")))
		return seterrorcode(SOUND_ERROR_FILEOPENFAIL);
	
	//initialize OggVorbis structure
	if ( (openres = ov_open(f, of, NULL, 0)) < 0)
	{
		switch(openres)
		{
		case OV_EREAD: seterrorcode(SOUND_ERROR_FILEREADFAIL); break;
		case OV_EVERSION:
		case OV_ENOTVORBIS: seterrorcode(SOUND_ERROR_BAD); break;
		case OV_EBADHEADER: seterrorcode(SOUND_ERROR_BADHEADER); break;
		default: seterrorcode(SOUND_ERROR_UNKNOWN);
		}

		fclose(f); //if ov_open fails, it's safe to use fclose()
		return false;
	}

	vorbis_info *vi = ov_info(of, -1);
	channels = vi->channels - 1;
	srate = vi->rate;
	resetsound = 1;
	info.bitrate = (int)vi->bitrate_nominal;
	if (info.bitrate > 1000)
		info.bitrate /= 1000;
	info.mp3_version = vi->version;
	info.samplerate = (int)vi->rate;
	double totaltime = ov_time_total(of, -1);
	if (totaltime > 0)
		info.totaltime = int(totaltime);

	//retrieve interesting information from the user comments.
	char **ptr = ov_comment(of, -1)->user_comments;
	while (*ptr)
	{
		char hdr[strlen(*ptr)+1], value[strlen(*ptr)+1];
		int sval = sscanf(*ptr, "%[^=]=%[^\n]", hdr, value); //lazy, but works.

		if (sval == 2)
		{
			if (!strcasecmp(hdr, "album"))
			{
				strncpy(info.album, value, 30);
				info.album[30] = '\0';
			}
			else if (!strcasecmp(hdr, "artist"))
			{
				strncpy(info.artist, value, 30);
				info.artist[30] = '\0';
			}
			else if (!strcasecmp(hdr, "title"))
			{
				strncpy(info.songname, value, 30);
				info.songname[30] = '\0';
			}
			else
			{
				debug("OggVorbis Unkown Fieldname '%s' with value '%s'\n", hdr, value);
			}
		}
		++ptr;
	}

	return true;
}

void Oggplayer::closefile()
{
	if (of)
		ov_clear(of);
}

void Oggplayer::setforcetomono(short flag)
{
	if (flag)
		mono = 1;
	//TODO: make a function in soundplayer class to force mono.
}

void Oggplayer::set8bitmode()
{
	wordsize = 1;
	player->set8bitmode();
}

void Oggplayer::setdownfrequency(int value)
{
	downfreq = (value ? 1 : 0);
	//TODO: This doesn't do anything yet.
}

bool Oggplayer::playing()
{
	return true;
}

bool Oggplayer::run(int sec)
{
	int bitstream;
	long bytes_read = ov_read(of, soundbuf, 4096, bigendian, wordsize, signeddata,
		&bitstream);

	if (sec); //prevent warning

	if (bytes_read < 0)
		return seterrorcode(SOUND_ERROR_BAD);
	if (!bytes_read)
		return seterrorcode(SOUND_ERROR_FINISH);
	
	vorbis_info *vi = ov_info(of, bitstream);

	if (channels != vi->channels)
		channels = vi->channels, resetsound++;
	if (srate != vi->rate)
		srate = vi->rate, resetsound++;
		
	if (resetsound)
	{
		debug("OggVorbis channels/samplerate changed.\n");
		player->setsoundtype(vi->channels - 1, (wordsize == 2 ? AFMT_S16_NE : AFMT_S8),
			vi->rate >> downfreq);
		char bla[100];
		sprintf(bla, "channels/wordsize/rate=%d/%d/%ld\n",vi->channels,wordsize,
			vi->rate >> downfreq);
		debug(bla);
		resetsound = 0;
	}

	//If downsampling, we only need every other frame. We do this by
	//removing every other frame.
	//TODO: optimize!
	if (downfreq)
	{
		char *src, *dst, ssize = 1;
		int c, i;

		src=dst=(char*)soundbuf;
		ssize *= vi->channels;
		ssize *= wordsize;

		//ssize is #bytes for each sample.
		c = bytes_read / (ssize<<1); //nr. of samples to chop == half of total#
		while (c--)
		{
			i = ssize;
			while (--i > -1) //copy ssize bytes of each sample.
				*(dst + i) = *(src + i);
			dst += ssize;
			src += (ssize<<1); //skip 50% of samples.
		}
	}

	if (!player->putblock(soundbuf, (unsigned int)(bytes_read>>downfreq)))
		return seterrorcode(SOUND_ERROR_UNKNOWN);

	return true;
}

void Oggplayer::skip(int sec)
{
	double current_secs = elapsed_time();
	current_secs += (double)sec;
	ov_time_seek(of, current_secs);
}

bool Oggplayer::initialize(void *data)
{
	if (data);
	return true;
}
 
bool Oggplayer::forward(int sec)
{
	skip(sec);
	return true;
}
 
bool Oggplayer::rewind(int sec)
{
	skip(-sec);
	return true;
}
 
bool Oggplayer::pause()
{
	return true;
}
 
bool Oggplayer::unpause()
{
	return true;
}
 
bool Oggplayer::stop()
{
	return true;
}
 
bool Oggplayer::ready()
{
	return true;
}
 
int Oggplayer::elapsed_time()
{
	double elapsed = ov_time_tell(of);

	return (int)elapsed;
}
 
int Oggplayer::remaining_time()
{
	return info.totaltime - elapsed_time();
}
 
#endif /* INCLUDE_OGG */
