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

// mp3Player constructor
mp3Player::mp3Player(mp3Play *calling, playWindow *interface, int threads)
{
	(this->caller) = calling;
	(this->interface) = interface;
	nthreads = (threads > 49 ? threads : 0);
	status = PS_NORMAL;
}

bool mp3Player::playing(int verbose)
{
	/* To avoid ``snap''s, turn down the volume for the first 5 frames */
	int volume, mixer = -1;
	if ( (mixer = open(MIXER_DEVICE, O_RDWR)) >= 0)
	{
		ioctl(mixer, MIXER_READ(SOUND_MIXER_VOLUME), &volume);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), 0);
	}

	if ( !server->run(-1) )
	{
		if (mixer > -1)
		{
			ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
			close(mixer); mixer = -1;
		}
		return false; // Initialize MPEG Layer 3
	}

	/* dummy code to get rid of warning of curr. unused var :) */
	if (verbose)
		verbose = 1;

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

	short
		should_play = 1,
		init_count = 0;

	interface->setProgressBar(0);
	interface->setTotalTime(server->gettotaltime());

	time_t
		tyd, newtyd;
	time(&tyd);

	while(should_play)
	{
		if (status == PS_PLAYING)
		{
			should_play = (server->run(globalopts.fpl));
			init_count += globalopts.fpl;
			/* restore volume? */
			if (mixer > -1 && (init_count > 4))
			{
				ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
				close(mixer); mixer = -1;
			}
		}
		else
			usleep(100); //cause a little delay to reduce system overhead

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
			case '1': /* stop and play prev. mp3 */
				caller->setAction(AC_PREV);
				should_play = 0;
				break;
			case '2':
			{
				if (status != PS_PLAYING)
					break;
				int
					curframe = server->getcurrentframe();
				curframe -= 100;
				if (curframe < 0)
					curframe = 0;
					
				server->setframe(curframe);
			}
				break;
			case '3': /* play/resume */
				if (status == PS_PAUSED)
					interface->setStatus( (status = PS_PLAYING) );
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
				int end_of_song = server->gettotalframe()-100;
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
				curframe += 100;
				if (curframe > maxframe)
					curframe = maxframe - 1;

				server->setframe(curframe);
			}
				break;
			case '5': /* next mp3 */
			case ' ':
				/* the program automagically choses the next MP3 in the play-
				 * list when nothing else is done */
				should_play = 0;
				break;
			case '6': /* pause/resume */
				if (status == PS_PLAYING) //put player in paused mode!
					interface->setStatus( (status = PS_PAUSED) ); 
				else if (status == PS_PAUSED) //resume player
					interface->setStatus( (status = PS_PLAYING) );
				break;
			case '7': // stop playing this particular mp2/3
				if (status == PS_PLAYING || status == PS_PAUSED)
				{
					interface->setStatus( (status = PS_STOPPED) );
					interface->setProgressBar(0);
				}
				break;
			case 'q': //leave playing interface.
				should_play = 0;
				caller->setAction(AC_QUIT);
				break;
			default: 
				if (interface->getMixerHandle())
					(interface->getMixerHandle())->ProcessKey(ch);
		}
	}

	if (mixer > -1) /* less than 5 frames were decoded. */
	{
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
		close(mixer);
	}

	seterrorcode(server->geterrorcode());
	interface->setStatus( (status = PS_NORMAL) );

	return ( (geterrorcode() == SOUND_ERROR_FINISH) || (geterrorcode() ==
		SOUND_ERROR_OK));
}

#ifdef PTHREADEDMPEG
bool mp3Player::playingwiththread(int verbose)
{
	if ( nthreads < 20 )
		return playing(verbose);

	/* To avoid ``snap''s, turn down the volume for the first 5 frames */
	int volume, mixer = -1;
	if ( (mixer = open(MIXER_DEVICE, O_RDWR)) >= 0)
	{
		ioctl(mixer, MIXER_READ(SOUND_MIXER_VOLUME), &volume);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), 0);
	}

	server->makethreadedplayer(nthreads);

	if ( !server->run(-1) )
	{
		if (mixer > -1)
		{
			ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
			close(mixer); mixer = -1;
		}
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

	short
		should_play = 1,
		init_count = 0;
	time_t
		tyd, newtyd;

	time(&tyd);

	/* build up buffer */
	server->pausethreadedplayer();
	while (server->getframesaved() < nthreads - 1)
		server->run(1);
	server->unpausethreadedplayer();

int	framesaveloop = 99;

	while(should_play)
	{
		if (status == PS_PLAYING)
		{
if (++framesaveloop>99)
{
framesaveloop=0;
char blub[100];sprintf(blub, "Framesaved: %d\n", server->getframesaved());
debug(blub);
}
			should_play = (server->run(globalopts.fpl));
			init_count += globalopts.fpl;
			/* restore volume? */
			if (mixer > -1 && (init_count > 4))
			{
				ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
				close(mixer); mixer = -1;
			}
		}
		else if (status == PS_PAUSED)
		{
			if (server->getframesaved() < nthreads - 1)
				server->run(1);
			else
				usleep(100);
		}

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
			case '1': /* stop and play prev. mp3 */
				server->stopthreadedplayer();
				caller->setAction(AC_PREV);
				should_play = 0;
				break;
			case '2':
			{
				if (status != PS_PLAYING)
					break;
				int
					curframe = server->getcurrentframe();
				curframe -= 100;
				if (curframe < 0)
					curframe = 0;
					
				server->setframe(curframe);
			}
			break;
			case '3': /* play/resume */
				if (status == PS_PAUSED)
				{
					interface->setStatus( (status = PS_PLAYING) );
					server->unpausethreadedplayer();
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
				int end_of_song = server->gettotalframe()-100;
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
				curframe += 100;
				if (curframe > maxframe)
					curframe = maxframe - 1;
					
				server->setframe(curframe);
			}
			break;
			case '5': /* next mp3 */
			case ' ':
				/* the program automagically choses the next MP3 in the play-
				 * list when nothing else is done */
				server->stopthreadedplayer();
				should_play = 0;
				break;
			case '6': /* pause/resume */
				if (status == PS_PLAYING) //put player in paused mode!
				{
					interface->setStatus( (status = PS_PAUSED) ); 
					server->pausethreadedplayer();
				}
				else if (status == PS_PAUSED) //resume player
				{
					interface->setStatus( (status = PS_PLAYING) );
					server->unpausethreadedplayer();
				}
				break;
			case '7': // stop playing this particular mp2/3
				if (status == PS_PLAYING || status == PS_PAUSED)
				{
					//if (status == PS_PAUSED)
					//	server->unpausethreadedplayer();

					interface->setStatus( (status = PS_STOPPED) );
					server->stopthreadedplayer();
					server->freethreadedplayer();
					interface->setProgressBar(0);
				}
				break;
			case 'q': //leave playing interface.
				server->stopthreadedplayer();
				should_play = 0;
				caller->setAction(AC_QUIT);
				break;
			default: 
				if (interface->getMixerHandle())
					(interface->getMixerHandle())->ProcessKey(ch);
		}
	}

	if (mixer > -1) /* less than 5 frames were decoded. */
	{
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volume);
		close(mixer);
	}
	
	seterrorcode(server->geterrorcode());

	if (status != PS_STOPPED)
		server->freethreadedplayer();
	interface->setStatus( (status = PS_NORMAL) );
	return ( (geterrorcode() == SOUND_ERROR_FINISH) || (geterrorcode() ==
		SOUND_ERROR_OK));
}
#endif
