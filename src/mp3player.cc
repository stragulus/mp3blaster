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
#include "playwindow.h"
#include "mp3player.h"

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
	if ( !server->run(-1) )
		return false; // Initialize MPEG Layer 3

	/* dummy code to get rid of warning of curr. unused var :) */
	if (verbose)
		verbose = 1;

	interface->setStatus( (status = PS_PLAYING) );
	interface->setProperties( server->getlayer(), server->getfrequency(),
		server->getbitrate(), server->isstereo() );
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
#ifdef DEBUG
	interface->setFrames(server->gettotalframe());
#endif

	short
		should_play = 1;

	interface->setProgressBar(0);

	time_t
		tyd, newtyd;
	time(&tyd);
	
	while(should_play)
	{
		if (status == PS_PLAYING)
			should_play = (server->run(5));
		else 
			usleep(100); //cause a little delay to reduce system overhead

		if (!should_play)
			continue;

		time(&newtyd);
		if (difftime(newtyd, tyd) >= 1)
		{
			int
				progress = (server->getcurrentframe() * 100) /
					server->gettotalframe();
			tyd = newtyd;
			interface->setProgressBar(progress);
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
			default: continue;
		}
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

	server->makethreadedplayer(nthreads);

	if ( !server->run(-1) )
		return false;       // Initialize MPEG Layer 3

	interface->setProgressBar(0);
	interface->setStatus( (status = PS_PLAYING) );
	interface->setProperties( server->getlayer(), server->getfrequency(),
		server->getbitrate(), server->isstereo() );
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
#ifdef DEBUG
	interface->setFrames(server->gettotalframe());
#endif

	short
		should_play = 1;
	time_t
		tyd, newtyd;

	time(&tyd);

	/* build up buffer */
	server->pausethreadedplayer();
	while (server->getframesaved() < nthreads - 1)
		server->run(1);
	server->unpausethreadedplayer();
		
	while(should_play)
	{
		if (status == PS_PLAYING)
			should_play = (server->run(5));
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
				progress = (server->getcurrentframe() * 100) / 
					server->gettotalframe();

			if (progress > 100)
				progress = 100;
			tyd = newtyd;
			interface->setProgressBar(progress);
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
			default: continue;
		}
	}

	seterrorcode(server->geterrorcode());

	if (status != PS_STOPPED)
		server->freethreadedplayer();

	interface->setStatus( (status = PS_NORMAL) );
  
	return ( (geterrorcode() == SOUND_ERROR_FINISH) || (geterrorcode() ==
		SOUND_ERROR_OK));
}
#endif
