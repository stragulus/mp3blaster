/* A soundmixer for linux with a nice text-interface for those non-X-ers.
 * (C)1998 Bram Avontuur (brama@stack.nl)
 */
#ifndef _LIBMIXER_H
#define _LIBMIXER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#else
#error "Can't find any ncurses include file!"
#endif
#ifdef HAVE_NASPLAYER
#include <audio/audiolib.h>
#endif

#define MIXER_DEVICE "/dev/mixer"
#define MYMIN(x, y) ((x) < (y) ? (x) : (y))
#define MYVERSION "<<NMixer "VERSION">>"

#define BOTH_CHANNELS 0x11
#define RIGHT_CHANNEL 0x10
#define LEFT_CHANNEL  0x01

enum mixer_cmd_t { MCMD_NEXTDEV, MCMD_PREVDEV, MCMD_VOLUP, MCMD_VOLDN };

struct volume
{
	short left;
	short right;
}; 

class baseMixer
{
public:
	virtual ~baseMixer();
	baseMixer(baseMixer *next);
	int *GetDevices(int *num);
	bool SetMixer(int device, struct volume *vol);
	bool GetMixer(int device, struct volume *vol);
	const char *GetMixerLabel(int device);
	virtual bool CanRecord(int device) = 0;
	virtual bool GetRecord(int device) = 0;
	virtual bool SetRecord(int device, bool set) = 0;
protected:
	void AddDevice(int device);
	virtual bool Set(int device, struct volume *vol) = 0;
	virtual bool Get(int device, struct volume *vol) = 0;
	virtual const char *Label(int device) = 0;
	int *devs, num_devs;
private:
	baseMixer *next;
	int mixerID;
};

class OSSMixer : public baseMixer
{
public:
	OSSMixer(baseMixer *next = 0);
	~OSSMixer();
	bool CanRecord(int device);
	bool GetRecord(int device);
	bool SetRecord(int device, bool set);
protected:
	bool Set(int device, struct volume *vol);
	bool Get(int device, struct volume *vol);
	const char *Label(int device);
private:
	int mixer;
};

#ifdef HAVE_NASPLAYER
class NASMixer : public baseMixer
{
public:
	NASMixer(baseMixer *next = 0);
	~NASMixer();
	bool CanRecord(int device) { return false; }
	bool GetRecord(int device) { return false; }
	bool SetRecord(int device, bool set) { return false; }
protected:
	bool Set(int device, struct volume *vol);
	bool Get(int device, struct volume *vol);
	const char *Label(int device);
private:
	AuServer *aud;
	AuDeviceAttributes *ada;
	int num_ada;
};
#endif

class NMixer
{
public:
	NMixer(WINDOW* mixwin, const char *mixdev=NULL, int xoffset=0, 
		int yoffset=0, int nrlines=0, const int *pairs=0, int bgcolor=0,
		short minimode=0);
	~NMixer();

	short NMixerInit();
	void DrawScrollbar(short i, int spos);
	void ChangeBar(short bar, short amount, short absolute, short channels,
		short update=1);
	void RedrawBars();
	void setMixerCommandKey(mixer_cmd_t, int);
	void setDefaultCmdKeys();
	short ProcessKey(int key);
	/* functions for each well-known mixertype */
	void SetMixer(int device, struct volume value, short update=1);
	bool GetMixer(int device, struct volume *vol);
	void redraw();
	
private:
	void DrawFixedStuff();

	baseMixer *mixers;
	const int *supported;
	char *mixdev;
	int
		xoffset,
		yoffset,
		nrlines,
		maxx, maxy,
		nrbars,
		*cpairs,
		bgcolor,
		cmdkeys[4];
	WINDOW
		*mixwin;
	short
		currentbar,
		minbar, /* index-nr. of topmost on-screen bar. */
		maxspos, /* index-nr. of bottommost on-screen bar. */
		currentspos, /* index-nr. of screen-position of current bar */
		minimode; /* 1 for mini-mode, 0 for large mode */
};
#endif
