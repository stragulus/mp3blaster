#ifndef _SIDPLAYER_CLASS_
#define _SIDPLAYER_CLASS_

#include "mp3blaster.h"
#include "mp3play.h"
#include "playwindow.h"
#include <mpegsound.h>
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif

class SIDPlayer : public SIDfileplayer, public genPlayer
{
public:
	SIDPlayer(mp3Play *calling, playWindow *interface, int threads);
	~SIDPlayer(){}
	bool playing(int verbose);
	int geterrorcode(void) { return SIDfileplayer::geterrorcode(); }
	bool openfile(char *filename, char *device, soundtype write2file=NONE) {
		return SIDfileplayer::openfile(filename, device, write2file); }
	void setdownfrequency(int value) {
		SIDfileplayer::setdownfrequency(value); }

private:
	int nthreads;
	playstatus status;
	playWindow *interface;
	mp3Play *caller;
};

#endif /*\ _SIDPLAYER_CLASS_ \*/
