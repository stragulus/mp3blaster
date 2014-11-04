/* This file is best viewed with tabs being 4 spaces.
 * In this file you can set certain options. 
 */

#ifndef _MP3BLASTER_
#define _MP3BLASTER_

/* NCURSES is the location of the ncurses headerfile */
#define NCURSES <ncurses/curses.h>
/* #define NCURSES <ncurses.h> */
/* or even: (those ncurses-people really can't make up their mind!) */
/* #define NCURSES <ncurses/ncurses.h> */

/* DEBUG is only useful if you're programming this.. */
#undef DEBUG

/* SOUND_DEVICE is the digital sound processor-device on your box. */
#define SOUND_DEVICE "/dev/dsp"

/* define this if you have the pthread-lib installed (recommended!) */
#define PTHREADEDMPEG

/* --------------------------------------- */
/* Do not change anything below this line! */
/* --------------------------------------- */

#define VERSION "2.0b1"

/* bad hack to get things to work. I might figure out the autoconf package one
 * day though :-)
 */
#ifdef PTHREADEDMPEG
#define HAVE_PTHREAD_H
#endif

enum playstatus { PS_NORMAL, PS_PLAYING, PS_STOPPED, PS_PAUSED };
enum actiontype { AC_NONE, AC_NEXT, AC_PREV, AC_QUIT, AC_SAMESONG };
enum playmode {
	PLAY_NONE, 	
	PLAY_GROUP,           /* play songs from displayed group in given order */
	PLAY_GROUPS,          /* play all songs from all groups in given order */
	PLAY_GROUPS_RANDOMLY, /* play all songs grouped by groups, with random
	                         group-order */
	PLAY_SONGS };         /* play all songs from all groups in random order */

inline int MIN(int x, int y)
{
	return (x < y ? x : y);
}

inline int MAX(int x, int y)
{
	return (x > y ? x : y);
}

/* interesting prototypes */
#ifdef DEBUG
void debug(const char*);
#endif

//void popupWindow(const char*, int, int, int);
void warning(const char*); /* for use with the play-interface */
void Error(const char*); /* for use with the selection-interface */
//void messageBox(const char*);

#endif // _MP3BLASTER_
