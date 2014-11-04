/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) Bram Avontuur (bram@avontuur.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "mp3blaster.h"
#include NCURSES
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "mp3play.h"
#include "mp3player.h"
#include "global.h"
#ifdef HAVE_SIDPLAYER
#include "sidplayer.h"
#endif /*\ HAVE_SIDPLAYER \*/
#include "playwindow.h"

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif

/* prototypes */
//extern int handle_input(short);
extern struct _globalopts globalopts;

inline void mp3Play::error(int n)
{
	const char *sound_errors[21] =
	{
		"Everything's OK (You shouldnt't see this!)",
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
		"Bad mpeg header.",
		
		"Unknown error."
	};

	if (n < 0)
		warning("Sudden EndOfFile");
	else
		warning(sound_errors[n]);
	if (interface)
		interface->redraw();
	else
		wclear(stdscr);
}

mp3Play::mp3Play(const char *mp3tje)
{
	mp3s = NULL;
	interface = NULL;
	nmp3s = 0;
	threads = 0;
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
	threads = 0;
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

/* the program needs to be in ncurses-mode before this function's used.
 * and the screensize had better be at least 60x12 too!
 */
int mp3Play::playMp3List()
{
	int
		retval = 1;
	unsigned int
		next_to_play = 0;
	short
		play = 1;
	genPlayer
		*player;
	char
		*snddev;

	if (!nmp3s) /* nothing to play. Whaaah. */
		return -1;

	if (globalopts.sound_device)
	{
		snddev = new char[strlen(globalopts.sound_device)+1];
		strcpy(snddev, globalopts.sound_device);
	}
	else
	{
		snddev = NULL; /* let the lib handle it */
	}

	interface = new playWindow();
	interface->setStatus(PS_NORMAL);

	/* quick hack to avoid ``snap''s, turn down the volume */
	int volume, dum = 0, mixer = -1;
	if ( (mixer = open(MIXER_DEVICE, O_RDWR)) >= 0)
	{
		ioctl(mixer, MIXER_READ(SOUND_MIXER_VOLUME), &volume);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &dum);
	}

	mp3Player mp3p(this, interface, threads);
#ifdef HAVE_SIDPLAYER
	SIDPlayer sidp(this, interface, threads);
#endif

	if (mixer > -1)
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);

	while (play)
	{
		unsigned int
			i = next_to_play;
		char
			*filename = mp3s[i];

		next_to_play++; //default is to play next mp3 in list after this one.
		action = AC_NEXT; //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
		int is_sid(const char*);

#ifdef HAVE_SIDPLAYER
		if (is_sid(filename)) player = &sidp;
		else
#endif
		player = &mp3p;

		if( !(player->openfile(filename, snddev)) )
		{
			int vaut = player->geterrorcode();

			error(vaut);
			if (vaut == SOUND_ERROR_DEVOPENFAIL)
				play = 0;
		}
		else
		{
			interface->setFileName(filename);
			player->setdownfrequency(globalopts.downsample);
			if (globalopts.eightbits)
				player->set8bitmode();


#ifdef PTHREADEDMPEG
			if ( !(player->playingwiththread()) )
				error(player->geterrorcode());
#else
			if ( !(player->playing()) )
				error(player->geterrorcode());
#endif
			interface->setStatus(PS_NORMAL);

			player->closefile(); //don't leave files open (oops)
			if (action == AC_QUIT) /* leave playing interface */
				play = 0;
			else if (action == AC_NEXT) /* that's like, default :) */
				;
			else if (action == AC_PREV) /* that's like, previous song! */
			{
				if (next_to_play >= 2)
					next_to_play -= 2; /* already increased by one */
				else
					next_to_play = 0;
			}
			else if (action == AC_SAMESONG) /* play that song again, sam */
				next_to_play--;
		}
		
		if (next_to_play >= nmp3s)
			play = 0;
	}
	
	delete interface;
	delete[] snddev;

	return retval;
}

genPlayer::~genPlayer()
{
}
