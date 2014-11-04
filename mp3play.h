#ifndef _MP3PLAY_CLASS_
#define _MP3PLAY_CLASS_

#include "mp3blaster.h"
#include "gstack.h"
#include "scrollwin.h"
#include "playwindow.h"
#include <bool.h>
#include <mpegsound.h>

class mp3Play
{
#if 0
private:
	mp3Play() { mp3s = NULL; nmp3s = 0; threads = 100; }
#endif

public:
	mp3Play(const char *mp3tje);
	mp3Play(const char **mp3tjes, unsigned int nmp3tjes);
	~mp3Play(); 

	void setThreads(int t) { threads = t; }
	int getThreads() { return threads; }
	int playMp3List();
	void setAction(actiontype bla) { action = bla; }
	actiontype getAction(void) { return action; }
	
private:
	inline void error(int n);

	int threads;
	char **mp3s;
	unsigned int nmp3s;
	playWindow *interface;
	actiontype action;
};

#endif
