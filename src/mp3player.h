#ifndef _MP3PLAYER_CLASS_
#define _MP3PLAYER_CLASS_

#include "mp3blaster.h"
#include "mp3play.h"
#include "playwindow.h"
#include <mpegsound.h>
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif

class mp3Player : public Mpegfileplayer, public genPlayer
{
public:
	mp3Player(mp3Play *calling, playWindow *interface, int threads);
	~mp3Player(){};
	bool playing();

#ifdef PTHREADEDMPEG
	bool playingwiththread();
#endif
	bool play(short threaded=0);
	int geterrorcode(void) { return Mpegfileplayer::geterrorcode(); }
	bool openfile(char *filename, char *device, soundtype write2file=NONE) {
		return Mpegfileplayer::openfile(filename, device, write2file); }
	void closefile(void) { return Mpegfileplayer::closefile(); }
	void setdownfrequency(int value) {
		Mpegfileplayer::setdownfrequency(value); }
	void set8bitmode() { Mpegfileplayer::set8bitmode(); }

private:
	int nthreads;
	playstatus status;
	playWindow *interface;
	mp3Play *caller;
};

#endif /* _MP3PLAYER_CLASS_ */
