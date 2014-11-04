/* This file belongs to the mp3player (C) 1997 Bram Avontuur.
 * It uses Jung woo-jae's mpegsound library, for which I am very grateful!
 */
#include <mpegsound.h>
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>

#define SOUND_DEVICE "/dev/dsp"

/* play-stati */
#define PS_NORMAL  0
#define PS_STOPPED 1
#define PS_PAUSED  2

/* prototypes */
extern int handle_input(short);

class Mp3player
{
public:
	Mp3player()
	{ 
		__errorcode = SOUND_ERROR_OK;
		player = NULL;
		loader = NULL;
		server = NULL;
	};
	~Mp3player() { delete server; delete loader; delete player; };

	int geterrorcode(void) { return __errorcode; };
	int playingwiththread(int threadstore);
	bool openfile(char *filename);
	bool seterrorcode(int errno) { __errorcode = errno; return false; }
	
private:
	Mpegtoraw *server;
	Soundinputbitstream *loader;
	Soundplayer *player;
	int __errorcode;
};

bool Mp3player::openfile(char *filename) 
{
	char *device = SOUND_DEVICE;

	/* Initialize player */
	player = new Rawplayer;

	if ( player == NULL )
		return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
		
	if( !player->initialize(device) )
		return seterrorcode(player->geterrorcode());

	/* Initialize loader */
	if ( (loader = new Soundinputbitstreamfromfile) == NULL)
		return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
		
	if ( !loader->open(filename) )
		return seterrorcode(loader->geterrorcode());

	/* Initialize server */
 	if( (server = new Mpegtoraw(loader, player)) == NULL)
		return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
	
	server->initialize();
	return true;
}

int Mp3player::playingwiththread(int threadstore)
{
	int
		retval = 1,
		play_status = PS_NORMAL;

	server->makethreadedplayer(threadstore);

	if( !server->run(-1) )
		return -1; // Initialize MPEG Layer 3

	for (;;)
	{
		int
			keypressed = 0;
		
		if ( (keypressed = handle_input(1)) != ERR)
		{
			switch(keypressed)
			{
				case 'p': /* pause */
				case KEY_F(2):
				{
					if ( play_status != PS_PAUSED)
					{
						server->pausethreadedplayer();
						play_status = PS_PAUSED;
					}
					else
					{
						server->pausethreadedplayer();
						play_status = PS_NORMAL;
					}
				}
				break;
				case 'q': /* stop */
				case KEY_F(1):
				{
					server->stopthreadedplayer();
					play_status = PS_STOPPED;
					retval = KEY_F(1);
				}
				break;
				case ' ': /* skip song when using playlist, else stop */
				{
					/* implement code here */
					server->stopthreadedplayer();
					play_status = PS_STOPPED;
					retval = ' ';
				}
				break;
			}
			
			if (play_status == PS_STOPPED)
				break;
		}
		else if (play_status == PS_PAUSED)
		{
			if ( server->getframesaved() < threadstore - 1)
				server->run(1);
			else
				usleep(100);
		}

		if (play_status != PS_PAUSED && !server->run(1))
			break;
	}
	server->freethreadedplayer();
  
	seterrorcode(server->geterrorcode());
	
	if ( seterrorcode(SOUND_ERROR_FINISH) )
		return -1;
	
	return retval;
}

int
playingthread(Mp3player *player, int threads)
{
	int
		retval;
		
 	if ( player->geterrorcode() > 0)
		return -1;

	retval = player->playingwiththread(threads);
	
	if (player->geterrorcode() > 0)
		return -1;
	
	return retval;
}

int 
playmp3(char *filename, int threads)
{
	Mp3player
		*player;
	int
		retval = 1;

	if (threads < 20)
		threads = 20;

	player = new Mp3player;

	if ( !(player->openfile(filename)) )
		retval = -1; /* error */
	else
		retval = playingthread(player, threads);

	delete player;
	return retval;
}
