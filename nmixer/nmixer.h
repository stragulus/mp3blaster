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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif

#ifdef __cplusplus
}
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

#define MIXER_DEVICE "/dev/mixer"
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define BOTH_CHANNELS 0x11
#define RIGHT_CHANNEL 0x10
#define LEFT_CHANNEL  0x01

#define MYVERSION "<<MixIt 2.0>>"

struct volume
{
	short left;
	short right;
}; 

class NMixer
{
public:
	NMixer(WINDOW* mixwin, int yoffset=0, int nrlines=0, int *pairs=0,
		int bgcolor=0);
	~NMixer();

	short NMixerInit();
	void DrawScrollbar(short i, int spos);
	void ChangeBar(short bar, short amount, short absolute, short channels,
		short update=1);
	void RedrawBars();
	short ProcessKey(int key);
	/* functions for each well-known mixertype */
	void SetMixer(int device, struct volume value, short update=1);
	short GetMixer(int device, struct volume *vol);
	
private:
	int
		yoffset,
		nrlines,
		maxx, maxy,
		*supported,
		mixer,
		nrbars,
		*cpairs,
		bgcolor;
	WINDOW
		*mixwin;
	short
		currentbar,
		minbar, /* index-nr. of topmost on-screen bar. */
		maxspos, /* index-nr. of bottommost on-screen bar. */
		currentspos; /* index-nr. of screen-position of current bar */
};
#endif
