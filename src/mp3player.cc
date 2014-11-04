/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) Bram Avontuur (brama@stack.nl)
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
 */
#include "mp3blaster.h"
#include NCURSES
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "global.h"
#include "playwindow.h"
#include "mp3player.h"

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif

extern struct _globalopts globalopts; /* from main.cc */
int volume, mixer = -1;

//prototypes
void mixer_mute();
void mixer_unmute();

// mp3Player constructor
mp3Player::mp3Player(mp3Play *calling, playWindow *interface, int threads)
{
	(this->caller) = calling;
	(this->interface) = interface;
	nthreads = (threads > 49 ? threads : 0);
	status = PS_NORMAL;
}

bool
mp3Player::playing()
{
	return play(0);
}

#ifdef PTHREADEDMPEG
bool mp3Player::playingwiththread()
{
	if ( nthreads < 20 )
		return play(0);

	return play(1);
}
#endif

bool mp3Player::play(short threaded)
{
	short
		should_play = 1,
		init_count = 0;
	time_t
		tyd, newtyd;

	/* To avoid ``snap''s, turn down the volume for the first 5 frames */
	mixer_mute();

#ifdef PTHREADEDMPEG
	if (threaded)
		server->makethreadedplayer(nthreads);
#endif

	if ( !server->run(-1) )
	{
		mixer_unmute();
		return false; // Initialize MPEG Layer 3
	}

	interface->setProgressBar(0);
	interface->setTotalTime(server->gettotaltime());

	interface->setStatus( (status = PS_PLAYING) );
	interface->setProperties( server->getversion()+1, server->getlayer(),
		server->getfrequency(), server->getbitrate(), server->getmodestring() );
	if (server->getname())
		interface->setSongName(server->getname());
	if (server->getartist())
		interface->setArtist(server->getartist());
	if (server->getalbum())
		interface->setAlbum(server->getalbum());
	if (server->getyear())
		interface->setSongYear(server->getyear());
	if (server->getcomment())
		interface->setSongInfo(server->getcomment());
//	if (server->getgenre())
		interface->setSongGenre(server->getgenre());
#ifdef DEBUG
	interface->setFrames(server->gettotalframe());
#endif

	time(&tyd);

#ifdef PTHREADEDMPEG
	if (threaded)
	{
		/* build up buffer */
		server->pausethreadedplayer();
		while (server->getframesaved() < nthreads - 1)
			server->run(1);
		server->unpausethreadedplayer();
	}
#endif

	while(should_play)
	{
		if (status == PS_PLAYING)
		{
			should_play = (server->run(globalopts.fpl));
			init_count += globalopts.fpl;
			/* restore volume? */
			if (mixer > -1 && (init_count > 4))
				mixer_unmute();
		}
#ifdef PTHREADEDMPEG
		else if (threaded && status == PS_PAUSED)
		{
			if (server->getframesaved() < nthreads - 1)
				server->run(1);
			else
				usleep(100);
		}
#endif
		else
			usleep(100);

		if (!should_play)
			continue;

		time(&newtyd);
		if (difftime(newtyd, tyd) >= 1)
		{
			int
				curr = server->getcurrentframe(),
				total = server->gettotalframe(),
				progress = ((curr * 100) / total),
				playtime = ((int)(curr * server->gettotaltime()) / total);

			tyd = newtyd;
			interface->setProgressBar(progress);
			interface->updateTime(playtime);
		}

		chtype ch = interface->getInput();
		/* put your interactive goodies here */

		switch(ch)
		{
			case ERR: continue;
			case 12: interface->redraw(); break;
			case '1': /* stop and play prev. mp3 */
#ifdef PTHREADEDMPEG
				if (threaded)
					server->stopthreadedplayer();
#endif
				caller->setAction(AC_PREV);
				should_play = 0;
				break;
			case '2':
			{
				if (status != PS_PLAYING)
					break;
				int
					curframe = server->getcurrentframe();
				curframe -= globalopts.skipframes;
				if (curframe < 0)
					curframe = 0;
					
				server->setframe(curframe);
			}
			break;
			case '3': /* play/resume */
				if (status == PS_PAUSED)
				{
					interface->setStatus( (status = PS_PLAYING) );
#ifdef PTHREADEDMPEG
					if (threaded)
						server->unpausethreadedplayer();
#endif
				}
				else if (status == PS_STOPPED)
				{
					caller->setAction(AC_SAMESONG);
					should_play = 0;
				}
				break;
			case 'e': /* quick undoc'd hack */
			{
				if (status != PS_PLAYING)
					break;
				int end_of_song = server->gettotalframe()-300;
				if (end_of_song < 0)
					end_of_song = 0;
				server->setframe(end_of_song);
			}
				break;

			case '4':
			{
				if (status != PS_PLAYING)
					break;
				int
					maxframe = server->gettotalframe(),
					curframe = server->getcurrentframe();
				curframe += globalopts.skipframes;
				if (curframe > maxframe)
					curframe = maxframe - 1;
					
				server->setframe(curframe);
			}
				break;
			case '5': /* next mp3 */
			case ' ':
				/* the program automagically choses the next MP3 in the play-
				 * list when nothing else is done */
#ifdef PTHREADEDMPEG
				if (threaded)
					server->stopthreadedplayer();
#endif
				should_play = 0;
				break;
			case '6': /* pause/resume */
				if (status == PS_PLAYING) //put player in paused mode!
				{
					interface->setStatus( (status = PS_PAUSED) ); 
#ifdef PTHREADEDMPEG
					if (threaded)
						server->pausethreadedplayer();
#endif
				}
				else if (status == PS_PAUSED) //resume player
				{
					interface->setStatus( (status = PS_PLAYING) );
#ifdef PTHREADEDMPEG
					if (threaded)
						server->unpausethreadedplayer();
#endif
				}
				break;
			case '7': // stop playing this particular mp2/3
				if (status == PS_PLAYING || status == PS_PAUSED)
				{
					interface->setStatus( (status = PS_STOPPED) );
#ifdef PTHREADEDMPEG
					if (threaded)
					{
						server->stopthreadedplayer();
						server->freethreadedplayer();
					}
#endif
					interface->setProgressBar(0);
				}
				break;
			case 'q': //leave playing interface.
#ifdef PTHREADEDMPEG
				if (threaded)
					server->stopthreadedplayer();
#endif
				should_play = 0;
				caller->setAction(AC_QUIT);
				break;
			default: 
				if (interface->getMixerHandle())
					(interface->getMixerHandle())->ProcessKey(ch);
		}
	}

	if (mixer > -1) /* less than 5 frames were decoded. */
		mixer_unmute();
	
	seterrorcode(server->geterrorcode());
	interface->setStatus( (status = PS_NORMAL) );

#ifdef PTHREADEDMPEG
	if (threaded && status != PS_STOPPED)
		server->freethreadedplayer();
#endif
	return ( (geterrorcode() == SOUND_ERROR_FINISH) || (geterrorcode() ==
		SOUND_ERROR_OK));
}

void
mixer_mute()
{
	int dum = 0;
	if ( (mixer = open(MIXER_DEVICE, O_RDWR)) >= 0)
	{
		ioctl(mixer, MIXER_READ(SOUND_MIXER_VOLUME), &volume);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &dum);
	}
}

void
mixer_unmute()
{
	if (mixer > -1)
	{
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
		close(mixer); mixer = -1;
	}
}

