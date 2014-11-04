/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Fileplayer.cc
// It's an example for how to use MPEG/WAVE Sound library

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern void debug(const char*);

#include <string.h>

#include "mpegsound.h"

// File player superclass
Fileplayer::Fileplayer()
{
  __errorcode=SOUND_ERROR_OK;
  player=NULL;
};

Fileplayer::~Fileplayer()
{
  delete player;
};

bool Fileplayer::opendevice(char *device, soundtype write2file)
{
	//char x[80]; sprintf(x, "device=%s\n", (device?device:"NULL"));debug(x);
	if (player) delete player;
	if (!write2file) {
#ifdef HAVE_NASPLAYER
		if (device && strrchr(device, ':')) //device=hostname:port?
			player = NASplayer::opendevice(device);
		if (player) return true;
#endif /*\ HAVE_NASPLAYER \*/
		player = Rawplayer::opendevice(device);
		if (player) return true;
	}
	player = Rawtofile::opendevice(device);
	if (player && ((Rawtofile*)player)->setfiletype(write2file))
		return true;
	return false;
}

// Wave file player
Wavefileplayer::Wavefileplayer()
{
  loader=NULL;
  server=NULL;
}

Wavefileplayer::~Wavefileplayer()
{
  if(loader)delete loader;
  if(server)delete server;
}

bool Wavefileplayer::openfile(char *filename,char *device, soundtype write2file)
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
  return server->initialize();
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
  if(!server->run())return false; // Read first time

  while(server->run());           // Playing

  seterrorcode(server->geterrorcode());
  if(geterrorcode()==SOUND_ERROR_FINISH)return true;
  return false;
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

bool Wavefileplayer::initialize()
{
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
	info.totaltime = server->gettotallength();
	if (server->isstereo())
		strcpy(info.mode, "stereo");
	else
		strcpy(info.mode, "mono");
	return true;
}


// Mpegfileplayer
Mpegfileplayer::Mpegfileplayer()
{
  loader=NULL;
  server=NULL;
  player=NULL;
};

Mpegfileplayer::~Mpegfileplayer()
{
  if(loader)delete loader;
  if(server)delete server;
}

/* if device[0] == '-' output will be written to stdout.
 * if write2file != 0, output will be written to file `device'
 */
bool Mpegfileplayer::openfile(char *filename,char *device, soundtype write2file)
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
      return seterrorcode(err);
  }

// Server
  if (server)
  	delete server;

  if((server=new Mpegtoraw(loader,player))==NULL)
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);

// Initialize server
  if (!server->initialize(filename)) {
	return seterrorcode(server->geterrorcode());
  }

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
  while(server->run(100));                // Playing

  seterrorcode(server->geterrorcode());
  if(seterrorcode(SOUND_ERROR_FINISH))return true;
  return false;
}

bool Mpegfileplayer::initialize()
{
	if (!server->run(-1))
	{
		seterrorcode(server->geterrorcode());
		return false;
	}
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
	curframe -= skipframes;
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

#ifdef PTHREADEDMPEG
bool Mpegfileplayer::playingwiththread(int framenumbers)
{
  if(framenumbers<20)return playing();

  server->makethreadedplayer(framenumbers);

  if(!server->run(-1))return false;       // Initialize MPEG Layer 3
  while(server->run(100));                // Playing
  server->freethreadedplayer();
  
  seterrorcode(server->geterrorcode());
  if(seterrorcode(SOUND_ERROR_FINISH))return true;
  return false;
}
#endif
