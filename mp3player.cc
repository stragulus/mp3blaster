#include "mp3blaster.h"
#include NCURSES
#include <mpegsound.h>
#include <string.h>
#include <unistd.h>
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

#if 0
	if ( verbose > 0 )
		showverbose(verbose);
#endif /* I'll do the verbose stuff myself, TYVM */

	while ( server->run(100) ) // playing
	{
		chtype ch = interface->getInput();
		/* do some nice good-looking interactive stuff here */
	}

	seterrorcode(server->geterrorcode());

	return (geterrorcode() == SOUND_ERROR_FINISH);
}

#ifdef PTHREADEDMPEG
bool mp3Player::playingwiththread(int verbose)
{
	if ( nthreads < 20 )
		return playing(verbose);

	server->makethreadedplayer(nthreads);

	if ( !server->run(-1) )
		return false;       // Initialize MPEG Layer 3

	interface->setStatus( (status = PS_PLAYING) );
	interface->setProperties( server->getlayer(), server->getfrequency(),
		server->getbitrate(), server->isstereo() );

	short
		should_play = 1;

	while(should_play)
	{
		if (status == PS_PLAYING)
			should_play = (server->run(5));
		else if (status == PS_PAUSED)
		{
			if (server->getframesaved() < nthreads - 1)
				server->run(3);
			else
				usleep(100);
		}

		if (!should_play)
			continue;

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
			case '3': /* play/resume */
				if (status == PS_PAUSED)
				{
					interface->setStatus( (status = PS_PLAYING) );
					server->unpausethreadedplayer();
				}
				else if (status == PS_STOPPED)
				{
					server->makethreadedplayer(nthreads);
					if (!server->run(-1)) //init. Mpeg-3
						return false;
					interface->setStatus ( (status = PS_PLAYING) );
				}
				break;
			case '5': /* next mp3 */
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
					if (status == PS_PAUSED)
						server->unpausethreadedplayer();

					interface->setStatus( (status = PS_STOPPED) );
					server->stopthreadedplayer();
					server->freethreadedplayer();
				}
				break;
			default: continue;
		}
	}

	if (status != PS_STOPPED)
		server->freethreadedplayer();

	interface->setStatus( (status = PS_NORMAL) );
  
	seterrorcode(server->geterrorcode());
	
	return (geterrorcode() == SOUND_ERROR_FINISH);
}
#endif
