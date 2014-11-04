/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Fileplayer.cc
// It's an example for how to use MPEG/WAVE Sound library

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

bool Wavefileplayer::openfile(char *filename,char *device)
{
// Player
  if(device==NULL)device=Rawplayer::defaultdevice;

  if(device[0]=='/')player=new Rawplayer;
  else 
  {
    if(device[0]=='-')device=NULL;
    player=new Rawtofile;
  }

  if(player==NULL)
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
  if(!player->initialize(device))return seterrorcode(player->geterrorcode());

// Loader
  {
    int err;

    if((loader=Soundinputstream::hopen(filename,&err))==NULL)
      return seterrorcode(err);
  }

// Server
  if((server=new Wavetoraw(loader,player))==NULL)
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
  return server->initialize();
}

void Wavefileplayer::setforcetomono(short flag)
{
  server->setforcetomono(flag);
};

bool Wavefileplayer::playing(int verbose)
{
  if(!server->run())return false; // Read first time

  if(verbose>0)
  {
    fprintf(stderr,
	    "Verbose : %dbits, "
	    "%dHz, "
	    "%s\n",
	    server->getsamplesize(),
	    server->getfrequency(),
	    server->isstereo()?"Stereo":"Mono");
  }

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

bool Mpegfileplayer::openfile(char *filename,char *device)
{
// Player
  if (!player)
  {
    if(device==NULL)device=Rawplayer::defaultdevice;

    if(device[0]=='/')player=new Rawplayer;
    else 
    {
      if(device[0]=='-')device=NULL;
     player=new Rawtofile;
    }

    if(player==NULL)
      return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
    if(!player->initialize(device))
      return seterrorcode(player->geterrorcode());
  }
// Loader
  {
    int err;
    if((loader=Soundinputstream::hopen(filename,&err))==NULL)
      return seterrorcode(err);
  }

// Server
  if (server)
  	delete server;

  if((server=new Mpegtoraw(loader,player))==NULL)
    return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);

// Initialize server
  if (!server->initialize(filename))
    return seterrorcode(SOUND_ERROR_BAD);

  return true;
}

void Mpegfileplayer::setforcetomono(short flag)
{
  server->setforcetomono(flag);
};

void Mpegfileplayer::setdownfrequency(int value)
{
  server->setdownfrequency(value);
};

bool Mpegfileplayer::playing(int verbose)
{
  if(!server->run(-1))return false;       // Initialize MPEG Layer 3
  if(verbose>0)showverbose(verbose);
  while(server->run(100));                // Playing

  seterrorcode(server->geterrorcode());
  if(seterrorcode(SOUND_ERROR_FINISH))return true;
  return false;
}

#ifdef PTHREADEDMPEG
bool Mpegfileplayer::playingwiththread(int verbose,int framenumbers)
{
  if(framenumbers<20)return playing(verbose);

  server->makethreadedplayer(framenumbers);

  if(!server->run(-1))return false;       // Initialize MPEG Layer 3
  if(verbose>0)showverbose(verbose);
  while(server->run(100));                // Playing
  server->freethreadedplayer();
  
  seterrorcode(server->geterrorcode());
  if(seterrorcode(SOUND_ERROR_FINISH))return true;
  return false;
}
#endif

void Mpegfileplayer::showverbose(int verbose)
{
  static char *modestring[4]={"stereo","joint stereo","dual channel","mono"};

  fprintf(stderr,"Verbose: MPEG-%d Layer %d, %s,\n\t%dHz%s, %dkbit/s, ",
	  server->getversion()+1,
	  server->getlayer(),modestring[server->getmode()],
	  server->getfrequency(),server->getdownfrequency()?"//2":"",
	  server->getbitrate());
  fprintf(stderr,server->getcrccheck() 
	  ? "with crc check\n" 
	  : "without crc check\n");
  if(verbose>1)
  {
    fprintf(stderr,
	    "Songname : %s\n"
	    "Artist   : %s\n"
	    "Album    : %s\n"
	    "Year     : %s\n"
	    "Comment  : %s\n",
	    server->getname(),
	    server->getartist(),
	    server->getalbum(),
	    server->getyear(),
	    server->getcomment());
  }
}
