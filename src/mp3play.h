#ifndef _MP3PLAY_CLASS_
#define _MP3PLAY_CLASS_

#include "mp3blaster.h"
#include "gstack.h"
#include "scrollwin.h"
#include "playwindow.h"
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif
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

#ifdef PTHREADEDMPEG
	void setThreads(int t) { threads = t; }
	int getThreads() { return threads; }
#endif
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

/*\ A lot of virtual functions to be able to stick multiple
|*| players into the same class pointer
\*/

class genPlayer
{
public:
	virtual ~genPlayer();
	virtual bool playing() = 0;

#ifdef PTHREADEDMPEG
	virtual bool playingwiththread() { return playing(); }
#endif
	virtual int geterrorcode(void) = 0;
	virtual bool openfile(char *, char *, soundtype write2file=NONE) = 0;
	virtual void closefile(void) = 0;
	virtual void setdownfrequency(int) = 0;
	virtual void set8bitmode() = 0;
};

#endif
