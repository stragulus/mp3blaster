/* This file is best viewed with tabs being 4 spaces.
 * In this file you can set certain options. 
 */

#ifndef _MP3BLASTER_
#define _MP3BLASTER_

/* NCURSES is the location of the ncurses headerfile 
 * I should use config.h that can be generated from autoconf for this
 * hassle, but it's just this ncurses-file that's a known bugger.
 */
#ifdef HAVE_NCURSES_NCURSES_H
#define NCURSES <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#define NCURSES <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#define NCURSES <ncurses.h>
#elif defined(HAVE_CURSES_H)
#define NCURSES <curses.h>
#else
#error "?Ncurses include files not found  error"
#endif 

/* DEBUG is only useful if you're programming this.. */
#undef DEBUG

/* define BWSCREEN when you're using a non-colour screen/terminal */
#define BWSCREEN

/* SOUND_DEVICE is the digital sound processor-device on your box. */
/* It's now in configure.in */
/*#define SOUND_DEVICE "/dev/dsp"*/

/* --------------------------------------- */
/* Do not change anything below this line! */
/* --------------------------------------- */

/* bad hack to get things to work. I might figure out the autoconf package one
 * day though :-)
 * 27 Oct. 1998: Which I did. No bad hack now, just pure cleverness..
 * 19 Jan. 1999: configure.in handles this stuff now.
 * #ifdef HAVE_LIBPTHREAD
 * #define PTHREADEDMPEG
 * #endif
 */

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
