/* This file belongs to the mp3player (C) 1997 Bram Avontuur.
 * It uses Jung woo-jae's mpegsound library, for which I am very grateful!
 * 27 October 1997: Code transformed into a neat C++ class instead of C code.
 * 7 November 1997: Upgraded to new mpegsound-lib (that goes with splay-0.8)
 */
#include "mp3blaster.h"
#include NCURSES
#include <mpegsound.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bool.h>
#include "mp3play.h"
#include "mp3player.h"
#include "playwindow.h"

/* prototypes */
//extern int handle_input(short);

inline void mp3Play::error(int n)
{
	const char *sound_errors[19] =
	{
		"Failed to open sound device.",
		"Sound device is busy.",
		"Buffersize of sound device is wrong.",
		"Sound device control error.",
	
		"Failed to open file for reading.",    // 5
		"Failed to read file.",                
	
		"Unkown proxy.",
		"Unkown host.",
		"Socket error.",
		"Connection failed.",                  // 10
		"Fdopen error.",
		"Http request failed.",
		"Http write failed.",
		"Too many relocations",
		
		"Out of memory.",               // 15
		"Unexpected EOF.",
		"Bad sound file format.",
		
		"Can't make thread.",
		
		"Unknown error."
	};

	warning(sound_errors[n]);
	refresh_screen();

	if (interface)
	{
		touchwin(interface->getCursesWindow());
		wrefresh(interface->getCursesWindow());
	}
}

mp3Play::mp3Play(const char *mp3tje)
{
	mp3s = NULL;
	interface = NULL;
	nmp3s = 0;
	threads = 100;
	action = AC_NONE;

	mp3s = new char*[1];
	mp3s[0] = new char[strlen(mp3tje) + 1];
	strcpy(mp3s[0], mp3tje);
	nmp3s = 1;
}

mp3Play::mp3Play(const char **mp3tjes, unsigned int nmp3tjes)
{
	mp3s = NULL;
	nmp3s = 0;
	threads = 100;
	interface = NULL;
	action = AC_NONE;

	mp3s = new char*[nmp3tjes];

	for (unsigned int i = 0; i < nmp3tjes; i++)
	{
		mp3s[i] = new char[strlen(mp3tjes[i]) + 1];
		strcpy(mp3s[i], mp3tjes[i]);
	}
	nmp3s = nmp3tjes;
}

mp3Play::~mp3Play()
{
	if (nmp3s)
	{
		for (unsigned int i = 0; i < nmp3s; i++)
			delete[] mp3s[i];
		delete[] mp3s;
	}
}

#if 0
/* old playingstuff */
int mp3Play::playingwiththread(int threadstore)
{
	int
		retval = 1,
		playstatus ps = PS_NORMAL;

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
					if ( ps != PS_PAUSED)
					{
						server->pausethreadedplayer();
						ps = PS_PAUSED;
					}
					else
					{
						server->pausethreadedplayer();
						ps = PS_NORMAL;
					}
				}
				break;
				case 'q': /* stop */
				case KEY_F(1):
				{
					server->stopthreadedplayer();
					ps = PS_STOPPED;
					retval = KEY_F(1);
				}
				break;
				case ' ': /* skip song when using playlist, else stop */
				{
					/* implement code here */
					server->stopthreadedplayer();
					ps = PS_STOPPED;
					retval = ' ';
				}
				break;
			}
			
			if (ps == PS_STOPPED)
				break;
		}
		else if (ps == PS_PAUSED)
		{
			if ( server->getframesaved() < threadstore - 1)
				server->run(1);
			else
				usleep(100);
		}

		if (ps != PS_PAUSED && !server->run(1))
			break;
	}
	server->freethreadedplayer();
  
	seterrorcode(server->geterrorcode());
	
	if ( seterrorcode(SOUND_ERROR_FINISH) )
		return -1;
	
	return retval;
}
#endif

/* the program needs to be in ncurses-mode before this function's used.
 * and the screensize had better be at least 60x12 too!
 */
int mp3Play::playMp3List()
{
	int
		retval = 1,
		mp3play_downfrequency = 0;

	if (!nmp3s) /* nothing to play. Whaaah. */
		return -1;

	interface = new playWindow();
	interface->setStatus(PS_NORMAL);

	//for (unsigned int i = 0; i < nmp3s; i++)
	unsigned int next_to_play = 0;
	short play = 1;

	while (play)
	{
		mp3Player
			*player;
		unsigned int
			i = next_to_play;
		char
			*filename = mp3s[i];
	
		next_to_play++; //default is to play next mp3 in list after this one.

		player = new mp3Player(this, interface, threads);
   
		if( !(player->openfile(filename, SOUND_DEVICE)) )
		{
			error(player->geterrorcode());
		}
		else
		{
			interface->setFileName(filename);
			player->setdownfrequency(mp3play_downfrequency);
	
#ifdef PTHREADEDMPEG
			if ( !(player->playingwiththread(1)) )
				error(player->geterrorcode());
#else
			if ( !(player->playing(1)) )
				error(player->geterrorcode());
#endif
			interface->setStatus(PS_NORMAL);

			if (action == AC_NEXT) /* that's like, default :) */
				;
			else if (action == AC_PREV) /* that's like, previous song! */
			{
				if (next_to_play >= 2)
					next_to_play -= 2; /* already increased by one */
				else
					next_to_play = 0;
			}
		}
		delete player;
		
		if (next_to_play >= nmp3s)
			play = 0;
	}

	delete interface;
	return retval;
}

