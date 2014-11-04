/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Fileplayer.cc
// It's an example for how to use MPEG/WAVE Sound library

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"

// File player superclass
Fileplayer::Fileplayer()
{
  __errorcode=SOUND_ERROR_OK;
  player=NULL;
	filename = NULL;
	info.songname[0] = '\0';
	info.artist[0] = '\0';
	info.comment[0] = '\0';
	info.year[0] = '\0';
	info.album[0] = '\0';
	info.genre = 0;
	info.mode[0] = '\0';
	info.bitrate=0;
	info.mp3_layer = 0;
	info.mp3_version = 0;
	info.samplerate = 0;
	info.totaltime = 0;
	audiodriver = AUDIODRV_OSS; //default audio driver
};

Fileplayer::~Fileplayer()
{
  delete player;
};

/* shared by all subclasses */
bool Fileplayer::opendevice(const char *device, soundtype write2file)
{
	delete player; //delete possibly existing player
	
	if (write2file)
	{
		player = Rawtofile::opendevice(device);
		return (player && ((Rawtofile*)player)->setfiletype(write2file));
	}

	//Choose an audio driver for audio playback
	switch (audiodriver)
	{
#ifdef WANT_OSS
	case AUDIODRV_OSS:
		debug("Using OSS audiodriver\n");
		player = Rawplayer::opendevice(device);
		break;
#endif
#ifdef WANT_NAS
	case AUDIODRV_NAS:
		debug("Using NAS audiodriver\n");
		player = NASplayer::opendevice(device);
		break;
#endif	
#ifdef WANT_ESD
	case AUDIODRV_ESD:
		debug("Using ESD audiodriver\n");
		player = new EsdPlayer(); //TODO: accept HOST param
		break;
#endif
#ifdef WANT_SDL
	case AUDIODRV_SDL:
		debug("Using SDL audiodriver\n");
		player = new SDLPlayer();
		break;
#endif
	default:
		debug("Unsupported audiodriver!\n");
		return false;
	}

	return player != NULL;
}

void
Fileplayer::set_driver(audiodriver_t driver)
{
	this->audiodriver = driver;
}

// Wave file player
Wavefileplayer::Wavefileplayer(audiodriver_t driver)
{
  loader=NULL;
  server=NULL;
	filename = NULL;
	set_driver(driver);
}

Wavefileplayer::~Wavefileplayer()
{
  if(loader)delete loader;
  if(server)delete server;
}

bool Wavefileplayer::openfile(const char *filename, const char *device, soundtype write2file)
{
// Player
  if (!opendevice(device, write2file))
	return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
// Loader
  {
    int err;

    if((loader=Soundinputstream::hopen(filename,&err))==NULL)
      return seterrorcode(err);
  }

// Server
  if (server)
  	delete server;
  if((server=new Wavetoraw(loader,player))==NULL)
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
	return true;
}

void Wavefileplayer::closefile(void)
{
	if (loader)
		loader->close();
}

void Wavefileplayer::setforcetomono(short flag)
{
  server->setforcetomono(flag);
};

bool Wavefileplayer::playing()
{
  if(!server->run())
	{
		return false; // Read first time
	}

  while(server->run())
	{
		// Playing
	}

  seterrorcode(server->geterrorcode());

  return (geterrorcode() == SOUND_ERROR_FINISH);
}

bool Wavefileplayer::run(int)
{
	if (!server->run())
	{
		seterrorcode(server->geterrorcode());
		return false;
	}
	return true;
}

int Wavefileplayer::elapsed_time()
{
	return server->getcurrentpoint() / server->getfrequency();
}

int Wavefileplayer::remaining_time()
{
	return info.totaltime - elapsed_time();
}

void Wavefileplayer::skip(int sec)
{
	int nrsamples = sec * server->getfrequency();
	server->setcurrentpoint(server->getcurrentpoint() + nrsamples);
}

bool Wavefileplayer::initialize(void *init_args)
{
	if (init_args); //prevent warning

  if (!server->initialize())
		return seterrorcode(server->geterrorcode());
	
	if (!server->run())
	{
		seterrorcode(server->geterrorcode());
		return false;
	}
	info.songname[0] = 0;
	info.artist[0] = 0;
	info.comment[0] = 0;
	info.year[0] = 0;
	info.album[0] = 0;
	info.genre = 255;
	info.bitrate = 0;
	info.mp3_layer = 0;
	info.mp3_version = 0;
	info.samplerate = server->getfrequency();
	if (server->getfrequency())
		info.totaltime = server->gettotallength() / server->getfrequency();
	else
		info.totaltime = 0;
	if (server->isstereo())
		strcpy(info.mode, "stereo");
	else
		strcpy(info.mode, "mono");
	return true;
}


// Mpegfileplayer
Mpegfileplayer::Mpegfileplayer(audiodriver_t driver)
{
  loader=NULL;
  server=NULL;
  player=NULL;
	filename = NULL;
	use_threads = 0;
	set_driver(driver);
};

Mpegfileplayer::~Mpegfileplayer()
{
#if !defined(NEWTHREAD) && defined(PTHREADEDMPEG)
	if (use_threads && server)
		server->freethreadedplayer();
#endif

  if(loader)delete loader;
  if(server)delete server;
}

/* if device[0] == '-' output will be written to stdout.
 * if write2file != 0, output will be written to file `device'
 */
bool Mpegfileplayer::openfile(const char *filename, const char *device, soundtype write2file)
{
// Player
  if (!opendevice(device, write2file))
	return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
// Loader
  {
		int err;
		if (loader)
			delete loader;
		if((loader=Soundinputstream::hopen(filename,&err))==NULL)
		{
			return seterrorcode(err);
		}
	}

// Server
  if (server)
  	delete server;

  if((server=new Mpegtoraw(loader,player))==NULL)
	{
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
	}

	if (this->filename)
		delete[] this->filename;
	this->filename = filename; //now needed for server->initialize()
  return true;
}

void
Mpegfileplayer::closefile(void)
{
	if (loader)
		loader->close();
}

void Mpegfileplayer::setforcetomono(short flag)
{
  server->setforcetomono(flag);
};

void Mpegfileplayer::setdownfrequency(int value)
{
  server->setdownfrequency(value);
};

bool Mpegfileplayer::playing()
{
  if(!server->run(-1))return false;       // Initialize MPEG Layer 3
  while(server->run(10))USLEEP(10000);                // Playing

  seterrorcode(server->geterrorcode());
  return geterrorcode() == SOUND_ERROR_FINISH;
}

bool Mpegfileplayer::initialize(void *init_args)
{
	int threads = 0;
	//struct init_opts *opts = NULL;
	init_opts *opts = NULL;

	if (init_args)
		opts = (struct init_opts *)init_args;

	//scan init_args
	while (opts)
	{
		const char *on = opts->option;

		if (!strcmp(on, "threads"))
			threads = *((int*)(opts->value)); 
		else if (!strcmp(on, "scanmp3s"))
			server->set_time_scan(*((short*)(opts->value)));

		opts = opts->next;
	}

// Initialize server
  if (!server->initialize(filename)) {
	return seterrorcode(server->geterrorcode());
  }

#ifndef NEWTHREAD
#ifdef PTHREADEDMPEG
	if (threads > 20)
	{
		use_threads = 1;
		server->makethreadedplayer(threads);
	}
#endif
#endif /* NEWTHREAD */

	if (!server->run(-20))
	{
		seterrorcode(server->geterrorcode());
		return false;
	}
#ifdef NEWTHREAD
	server->continueplaying();
#endif
	info.mp3_layer = server->getlayer();
	info.mp3_version = server->getversion();
	strcpy(info.mode, server->getmodestring());
	info.samplerate = server->getfrequency();
	info.bitrate = server->getbitrate();
	info.genre = server->getgenre();
	info.totaltime = server->gettotaltime();
	strcpy(info.songname, server->getname());
	strcpy(info.artist, server->getartist());
	strcpy(info.comment, server->getcomment());
	strcpy(info.year, server->getyear());
	strcpy(info.album, server->getalbum());
	return true;
}

bool Mpegfileplayer::run(int frames)
{
	if (!server->run(frames))
	{
		seterrorcode(server->geterrorcode());
		return false;
	}
	return true;
}

bool Mpegfileplayer::forward(int skipframes)
{
	int
		maxframe = server->gettotalframe(),
		curframe = server->getcurrentframe();
	
	//since skipframes resembles seconds, multiply it by 75 to match #frames
	skipframes *= 38;

	if (skipframes < 0) //jump to end of file
		curframe = maxframe + skipframes;
	else
		curframe += skipframes;
	if (curframe > maxframe)
		curframe = maxframe - 1;
		
	server->setframe(curframe);
	return true;
}

bool Mpegfileplayer::rewind(int skipframes)
{
	int
		curframe = server->getcurrentframe();
	
	curframe -= (skipframes * 38);
	if (curframe < 0)
		curframe = 0;

	server->setframe(curframe);
	return true;
}

int Mpegfileplayer::elapsed_time()
{
	int
		curr = server->getcurrentframe(),
		total = server->gettotalframe(),
		playtime = ((int)(curr * server->gettotaltime()) / total);
	return playtime;
}

int Mpegfileplayer::remaining_time()
{
	int
		curr = server->getcurrentframe(),
		total = server->gettotalframe(),
		playtime = ((int)(curr * server->gettotaltime()) / total);
	return server->gettotaltime() - playtime;
}

void Mpegfileplayer::skip(int frames)
{
	int
		maxframe = server->gettotalframe(),
		curframe = server->getcurrentframe();
	curframe += frames;
	if (curframe > maxframe)
		curframe = maxframe - 1;
	if (curframe < 0)
		curframe = 0;
	server->setframe(curframe);
}

#ifndef NEWTHREAD
#ifdef PTHREADEDMPEG
bool Mpegfileplayer::playingwiththread(int framenumbers)
{
  if(framenumbers<20)return playing();

  if (!server->makethreadedplayer(framenumbers))
	{
		return seterrorcode(server->geterrorcode());
	}

  if(!server->run(-1))
	{
		server->freethreadedplayer();
		return seterrorcode(server->geterrorcode());
	}

  while(server->run(100)) 
	{
		//Playing
	}

  server->freethreadedplayer();
  
  seterrorcode(server->geterrorcode());

  return geterrorcode() == SOUND_ERROR_FINISH;
}

#endif
#endif /* NEWTHREAD */

bool
Mpegfileplayer::stop()
{
#if !defined(NEWTHREAD) && defined(PTHREADEDMPEG)
	if (use_threads && server)
		server->stopthreadedplayer();
		//server->freethreadedplayer();
#endif
	return true;
}

/* all sound output has been sent to the sound card? */
bool
Mpegfileplayer::ready()
{
#if !defined(NEWTHREAD) && defined(PTHREADEDMPEG)
	if (use_threads && server && server->getframesaved() && 
	(server->geterrorcode() == SOUND_ERROR_OK || server->geterrorcode() ==
		SOUND_ERROR_FINISH))
		return false;
#endif

	return true;	
}
