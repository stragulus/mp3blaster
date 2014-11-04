/* Sound Player

   Copyright (C) 1997 by Woo-jae Jung */

// It's an example of using MPEG/WAVE Sound library

// Anyone can use MPEG/WAVE Sound library under GPL

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>

#include <mpegsound.h>

#include "splay.h"

static const char *help=
"\t-2 : playing with half frequency.\n"
"\t-e : exit when playing is done. (only XSPLAY)\n"
"\t-m : force to mono.\n"
"\t-r : repeat forever.\n"
"\t-s : shuffle play.\n"
"\t-v : verbose, Very verbose, Very very verbose.\n"
"\t-M : playing MPEG-Audio file as standard input. (only SPLAY)\n"
"\t-V : display version.\n"
"\t-W : playing Wave file as standard input. (only SPLAY)\n"
"\n"
"\t-h font : Set hangul font. (only XSPLAY for korean)\n"
"\t-d dev  : Set dev as device or file. (only device in XSPLAY)\n"
"\t-l list : list file.\n"
"\t-t buf  : Set number of buffer. (default : 50)\n"
"\n"
"For detail, see man pages.\n";

/***********************/
/* Command line player */
/***********************/
inline void error(int n)
{
  fprintf(stderr,"%s: %s\n",splay_progname,splay_Sounderrors[n-1]);
  return;
}

#ifdef PTHREADEDMPEG
static void playingthread(Mpegfileplayer *player)
{
  if(player->geterrorcode()>0)error(player->geterrorcode());
  else
  {
    player->setforcetomono(splay_forcetomonoflag);
#ifdef NEWTHREAD
		player->playing();
#else
    player->playingwiththread(splay_threadnum);
#endif
    if(player->geterrorcode()>0)error(player->geterrorcode());
  }
}
#endif

static void playing(Fileplayer *player)
{
  if(player->geterrorcode()>0)error(player->geterrorcode());
  else
  {
    player->setforcetomono(splay_forcetomonoflag);
    player->playing();
    if(player->geterrorcode()>0)error(player->geterrorcode());
  }
}

static void play(char *filename)
{
  if(splay_verbose)
    fprintf(stderr,"%s: \n",filename);

  if(strstr(filename,".mp") || strstr(filename,".MP"))
  {
    Mpegfileplayer *player;
		bool didopen = false;

    player=new Mpegfileplayer(Fileplayer::AUDIODRV_OSS);
		if (!strcmp(splay_devicename, "-"))
			didopen = player->openfile(filename, "/dev/stdout", WAV);
		else
			didopen = player->openfile(filename, splay_devicename);

		if (didopen)
			player->initialize(NULL);

    if(!didopen)
    {
      error(player->geterrorcode());
      delete player;
      return;
    }
    player->setdownfrequency(splay_downfrequency);
#ifdef PTHREADEDMPEG
    playingthread(player);
#else
    playing(player);
#endif
    delete player;
  }
  else
  {
    Wavefileplayer *player;
  
    player=new Wavefileplayer(Fileplayer::AUDIODRV_OSS);
    if(!player->openfile(filename,splay_devicename))
    {
      error(player->geterrorcode());
      delete player;
      return;
    }
    playing(player);

    delete player;
  }
}

int main(int argc,char *argv[])
{
  int c;

  splay_progname=argv[0];

  while((c=getopt(argc,argv,
		  "VMW2mrsvd:l:"
#ifdef PTHREADEDMPEG
		  "t:"
#endif
		  ))>=0)
  {
    switch(c)
    {

      case 'V':
			{
#ifdef PTHREADEDMPEG
			const char *pt = " with pthreads";
#else
			const char *pt = "";
#endif
 			printf("%s %s%s\n", "splay", "0.8.2", pt);
      return 0;
			break;
			}
      case 'M':
	{
	  Mpegfileplayer player(Fileplayer::AUDIODRV_OSS);

	  player.openfile(NULL,splay_devicename);
	  playing(&player);
	  return 0;
	}
      case 'W':
	{
	  Wavefileplayer player(Fileplayer::AUDIODRV_OSS);

	  player.openfile(NULL,splay_devicename);
	  playing(&player);
	  return 0;
	}

      case '2':splay_downfrequency  =   1;break;
      case 'm':splay_forcetomonoflag=true;break;
      case 'r':splay_repeatflag     =true;break;
      case 's':splay_shuffleflag    =true;break;
      case 'v':splay_verbose++;           break;

      case 'd':splay_devicename=optarg;   break;
      case 'l':if(splay_verbose)
		fprintf(stderr,"List file : %s\n",optarg);
	       readlist(optarg);
	       break;
#ifdef PTHREADEDMPEG
      case 't':
	{
	  int a;

	  sscanf(optarg,"%d",&a);
	  splay_threadnum=a;
	}
	break;
#endif
      default:fprintf(stderr,"Bad argument.\n");
    }
  }

  if(argc<=optind && splay_listsize==0)
  {
#ifdef PTHREADEDMPEG
		const char 
			*t1 = " with pthread",
			*t2 = "[-t number]";
#else
		const char 
			*t1 = "",
			*t2 = "";
#endif

    printf("%s %s%s"
	   "\n"
	   "Usage : splay [-2mrsvMVW] [-d device] [-l listfile] %s"
	   "files ...\n"
	   "\n"\
	   "%s"\
	   ,PACKAGE,VERSION,t1,t2,help);
    return 0;
  }

  if(splay_listsize==0)    // Make list by arguments
    arglist(argc,argv,optind);

  do
  {
    if(splay_shuffleflag)shufflelist();

    for(int i=0;i<splay_listsize;i++)
      play(splay_list[i]);
  }while(splay_repeatflag);

  return 0;
}
