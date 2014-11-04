#ifndef _MP3PLAYER_CLASS_
#define _MP3PLAYER_CLASS_

/* I needed to patch the mpegsound lib becuz the server member-variable from
 * the Mpegfileplayer was private so I couldn't access it with my subclass.
 * Just a reminder so I won't forget to patch future new versions of this lib..
 */
#include "mp3blaster.h"
#include "mp3play.h"
#include "playwindow.h"
#include <mpegsound.h>
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif

class mp3Player : public Mpegfileplayer
{
public:
	mp3Player(mp3Play *calling, playWindow *interface, int threads);
	~mp3Player(){};
	bool playing(int verbose);

#ifdef PTHREADEDMPEG
	bool playingwiththread(int verbose);
#endif

private:
	int nthreads;
	playstatus status;
	playWindow *interface;
	mp3Play *caller;
};

#endif /* _MP3PLAYER_CLASS_ */
