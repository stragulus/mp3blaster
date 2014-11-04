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
#ifdef HAVE_SIDPLAYER
#include NCURSES
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "playwindow.h"
#include "sidplayer.h"

extern void debug(const char *txt);
extern int fpl; /* frames per loop to run */

// SIDPlayer constructor
SIDPlayer::SIDPlayer(mp3Play *calling, playWindow *interface, int threads)
{
	(this->caller) = calling;
	(this->interface) = interface;
	nthreads = (threads > 49 ? threads : 0);
	status = PS_NORMAL;
}

bool SIDPlayer::playing(int verbose)
{
	/* dummy code to get rid of warning of curr. unused var :) */
	if (verbose)
		verbose = 1;

	interface->setStatus( (status = PS_PLAYING) );

	if (sidInfo.nameString)
		interface->setSongName(sidInfo.nameString);
	if (sidInfo.authorString)
		interface->setArtist(sidInfo.authorString);
	if (sidInfo.copyrightString)
		interface->setSongInfo(sidInfo.copyrightString);

	short
		should_play = 1,
		init_count = 0;

	interface->setProgressBar(0);

	while(should_play)
	{
		if (status == PS_PLAYING)
		{
			should_play = (run(fpl));
			init_count += fpl;
		}
		else
			usleep(100); //cause a little delay to reduce system overhead

		if (!should_play)
			continue;

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
				if (status != PS_PLAYING) break;
				if (song > 1) --song;
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
				if (status != PS_PLAYING) break;
				if (song < sidInfo.songs) ++song;
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

	interface->setStatus( (status = PS_NORMAL) );

	return ( (geterrorcode() == SOUND_ERROR_FINISH) || (geterrorcode() ==
		SOUND_ERROR_OK));
}
#endif /*\ HAVE_SIDPLAYER \*/
