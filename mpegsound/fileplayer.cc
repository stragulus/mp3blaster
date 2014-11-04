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
	if (player) return true;
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
