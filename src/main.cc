/* Copyright (C) Bram Avontuur (bram@avontuur.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * If you like this program, or if you have any comments, tips for improve-
 * ment, money, etc, please do so on the sourceforge project page :
 * http://mp3blaster.sourceforge.net/
 *
 * May 21 2000: removed all calls to attr_get() since not all [n]curses 
 *            : versions use the same paramaters (sigh)
 * Dec 31 2000: Applied a patch from Rob Funk to fix randomness bug
 * Newer dates: changes made are stored in CVS.
 */

/*
 * Added basic LIRC support.
 * Olgierd Pieczul <wojrus@linux.slupsk.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "history.h"
#include "mp3blaster.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fnmatch.h>
#if defined(PTHREADEDMPEG) && defined(HAVE_PTHREAD_H)
#include <pthread.h>
#elif defined (LIBPTH) && defined(HAVE_PTH_H)
#include <pth.h>
#endif
#include NCURSES_HEADER
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt_local.h"
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "global.h"
#include "scrollwin.h"
#include "mp3win.h"
#include "fileman.h"
#include "exceptions.h"
#include <mpegsound.h>
#include <nmixer.h>
//load keybindings
#include "keybindings.h"
#include "getinput.h"
#ifdef INCLUDE_LIRC
#include <lirc/lirc_client.h>
#endif
#ifdef LIBPTH
#define USLEEP(x) pth_usleep(x)
#else
#define USLEEP(x) usleep(x)
#endif

/* paranoia define[s] */
#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

#define MIN_SCREEN_WIDTH 80
#define MIN_SCREEN_HEIGHT 23

/* values for global var 'window' */
enum _action { AC_NONE, AC_REWIND, AC_FORWARD, AC_NEXT, AC_PREV,
	AC_STOP, AC_ONE_MP3, AC_PLAY,
	AC_STOP_LIST, AC_SKIPEND, AC_NEXTGRP, AC_PREVGRP } action;

/* There are different types of input modes, e.g. searching for text, 
 * inputting text, ..
 */
enum _input_mode { IM_DEFAULT, IM_SEARCH, IM_INPUT } input_mode;

/* External functions */
extern short cf_parse_config_file(const char *);
extern cf_error cf_get_error();
extern const char *cf_get_error_string();

/* Prototypes */
void cw_toggle_group_mode();
void cw_draw_group_mode();
void cw_draw_play_mode(short cleanit=0);
short set_play_mode(const char*);
void cw_toggle_play_mode();
void mw_clear();
void mw_settxt(const char*);
short read_playlist(const char *);
short write_playlist(const char *);
void *play_list(void *);
short start_song(short was_playing=0);
void set_one_mp3(char*);
void stop_song();
void stop_list();
void update_play_display();
void lock_playing_mutex();
void unlock_playing_mutex();
void update_status_file();
void write_status_file();

#ifdef PTHREADEDMPEG
void change_threads();
void cw_draw_threads(short cleanit=0);
short set_threads(int);
#endif
void add_selected_file(const char*);
char *get_current_working_path();
void usage();
void show_help();
void draw_static(int file_mode=0);
void draw_settings(int cleanit=0);
void fw_getpath(const char *, void *);
void fw_convmp3(const char *, void *);
void fw_addurl(const char *, void *);
void fw_search_next_char(char);
void fw_start_search(int timeout=2);
void fw_end_search();
void fw_delete();
void set_sound_device(const char *);
void set_mixer_device(const char *);
short set_fpl(int);
void set_default_colours();
void set_action(_action bla);
char *determine_song(short set_played=1);
void set_song_info(const char *fl, struct song_info si);
void set_song_status(playstatus_t);
void set_song_time(int,int);
void popup_win(const char **label, void (*func)(const char *, void *),  void *func_args = NULL, const char *init_str = NULL, short want_input=0, int maxlen=0);
int myrand(int);
void reset_playlist(int);
void cw_draw_repeat();
void init_helpwin();
void repaint_help();
void get_label(command_t, char *);
void set_keylabel(int k, char *label);
void init_playopts();
void init_globalopts();
void draw_next_song(const char*);
void set_next_song(int);
void reset_next_song();
mp3Win* newgroup();
short fw_markfilebad(const char *file);
void get_mainwin_size(int*, int*, int*, int*);
void get_mainwin_borderchars(chtype*, chtype*, chtype*, chtype*,
                             chtype*, chtype*, chtype*, chtype*); 
void change_program_mode(unsigned short pm);
void end_help();
void update_songinfo(playstatus_t, short);
void update_ncurses(int update_all);
void end_program();
const char *get_error_string(int);
void fw_toggle_sort();
void fw_draw_sortmode(sortmodes_t);
short fw_toggle_display();
short toggle_display();
void warning(const char *, ...);
void wake_player();
void add_init_opt(struct init_opts *myopts, const char *option, void *value);
void get_input(
	WINDOW *win, const char*defval, unsigned int size, unsigned int maxlen,
	int y, int x, void (*got_input)(const char *, void *), void *args = NULL
);
void set_inout_opts();
void select_files_by_pattern(const char *, void *);
void playlist_write(const char *playlistfile, void *args);
void fw_rename_file(const char *newname, void *args);
void *lirc_loop(void *);
#if defined(PTHREADEDMPEG)
# define LOCK_NCURSES (!pthread_mutex_lock(&ncurses_mutex))
# define UPDATE_CURSES
# define UNLOCK_NCURSES pthread_mutex_unlock(&ncurses_mutex)
#elif defined(LIBPTH)
# define LOCK_NCURSES (pth_mutex_acquire(&ncurses_mutex, FALSE, NULL) != -1)
# define UPDATE_CURSES
# define UNLOCK_NCURSES pth_mutex_release(&ncurses_mutex)
#endif

#define OPT_LOADLIST 1
#define OPT_DEBUG 2
#define OPT_PLAYMP3 4
#define OPT_NOMIXER 8
#define OPT_AUTOLIST 16
#define OPT_QUIT 32
#define OPT_CHROOT 64
#define OPT_DOWNSAMPLE 128
#define OPT_SOUNDDEV 256
#define OPT_PLAYMODE 512
#define OPT_FPL 1024
#define OPT_8BITS 2048
#define OPT_THREADS 4096
#define OPT_MIXERDEV 8192
#define OPT_REPEAT 16384

/* global vars */

/* songinf contains info about the currently playing audio file.
 * Access to songinf may only occur when a lock has been acquired on the
 * ncurses mutex (see LOCK_NCURSES)
 */
struct song_t
{
	struct song_info songinfo;
	playstatus_t status;
	int elapsed;
	int remain;
	char *path;
	char *next_song;
	char *warning;
	int update; // 0, don't update, 1: update songinfo, 2: update time, 3: both
} songinf;

struct playopts_t
{
	short song_played;
	short playing_one_mp3;
	short allow_repeat; //true if at least 1 song in the list finished correctly
	short pause;
	short playing;
	char * one_mp3;
	Fileplayer *player;
	time_t tyd;
	time_t newtyd;
	short quit_program;
} playopts;

struct ptag_t
{
	char tag[80];
	char keywords[5][80];
	char values[5][255];
};

unsigned short
	progmode;
#ifdef PTHREADEDMPEG
pthread_mutex_t
	playing_mutex = PTHREAD_MUTEX_INITIALIZER,
	ncurses_mutex = PTHREAD_MUTEX_INITIALIZER,
	onemp3_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t
	playing_cond = PTHREAD_COND_INITIALIZER;
pthread_t
	thread_playlist; //thread that handles playlist (main handles input)
#ifdef INCLUDE_LIRC
	pthread_t thread_lirc;
#endif
#elif defined(LIBPTH)
pth_mutex_t
	playing_mutex,
	ncurses_mutex,
	onemp3_mutex;
pth_cond_t
	playing_cond;
pth_t
	thread_playlist;

#endif /* PTHREADEDMPEG */
mp3Win
	*mp3_curwin = NULL, /* currently displayed mp3 window */
	*mp3_rootwin = NULL, /* root mp3 window */
	*mp3_groupwin = NULL, /* starting group for PLAY_GROUPS mode */
	*playing_group = NULL; /* in group-mode playback: group being played */
char
	*status_file = NULL;
fileManager
	*file_window = NULL; /* window to select files with */
scrollWin
	*helpwin = NULL,
	*bighelpwin = NULL;
NMixer
	*mixer = NULL;
int
	nselfiles = 0,
	current_song = -1,
	nr_played_songs = 1,
	next_song = -1;
short
	popup = 0, /* 1 when a `popup' window is onscreen */
	quit_after_playlist = 0;
struct _globalopts
	globalopts;
char
	**selected_files = NULL,
	*startup_path = NULL,
	*fw_searchstring,
	**played_songs = NULL,
	**environment = NULL;
History
	*history = NULL;
getInput
	*global_input = NULL;
#ifdef INCLUDE_LIRC
	command_t lirc_cmd = CMD_NONE;
#endif
/* Function Name: usage
 * Description  : Warn user about this program's usage and then exit nicely
 *              : Program must NOT be in ncurses-mode when this function is
 *              : called.
 */
void
usage()
{
	fprintf(stderr, "Mp3blaster v%s (C)1997 - 2009 Bram Avontuur.\n%s", \
		VERSION, \
		"Usage:\n" \
		"\tmp3blaster [options]\n"\
		"\tmp3blaster [options] [file1 ...]\n"\
		"\t\tPlay one or more mp3's\n"\
		"\tmp3blaster [options] --list/-l <playlist.lst>\n"\
		"\t\tLoad a playlist but don't start playing.\n"\
		"\tmp3blaster [options] --autolist/-a <playlist.lst>\n"\
		"\t\tLoad a playlist and start playing.\n"\
		"\nOptions:\n"\
		"\t--downsample/-2: Downsample (44->22Khz etc)\n"\
		"\t--8bits/-8: 8bit audio (autodetected if your card can't handle 16 bits)\n"\
		"\t--config-file/-c=file: Use other config file than the default\n"\
		"\t\t~/.mp3blasterrc\n"\
		"\t--debug/-d: Log debug-info in $HOME/.mp3blaster.\n"\
		"\t--status-file/-f=file: Keep info on the mp3s being played, in the\n"\
		"\t\tspecified file.\n"\
		"\t--help/-h: This help screen.\n"\
		"\t--mixer-device/-m: Mixer device to use (use 'NAS' for NAS mixer)\n"\
		"\t--no-mixer/-n: Don't start the built-in mixer.\n"\
		"\t--chroot/-o=<rootdir>: Set <rootdir> as mp3blaster's root dir.\n"\
		"\t\tThis affects *ALL* file operations in mp3blaster!!(including\n"\
		"\t\tplaylist reading&writing!) Note that only users with uid 0\n"\
		"\t\tcan use this option (yet?). This feature will change soon.\n"\
		"\t--playmode/-p={onegroup,allgroups,allrandom}\n"\
		"\t\tDefault playing mode is resp. Play first group only, Play\n"\
		"\t\tall groups, Play all songs in random order.\n"\
		"\t--dont-quit/-q: Don't quit after playing mp3[s] (only makes sense\n"\
		"\t\tin combination with --autolist or files from command-line)\n"\
		"\t--repeat/-R: Repeat playlist.\n"\
		"\t--runframes/-r=<number>: Number of frames to decode in one loop.\n"\
		"\t\tRange: 1 to 10 (default=5). A low value means that the\n"\
		"\t\tinterface (while playing) reacts faster but slow CPU's might\n"\
		"\t\thick. A higher number implies a slow interface but less\n"\
		"\t\thicks on slow CPU's.\n"\
		"\t--sound-device/-s=<device>: Device to use to output sound.\n"\
		"\t\tDefault for your system is " SOUND_DEVICE ".\n"\
		"\t\tIf you want to use NAS (Network Audio System) as playback\n"\
		"\t\tdevice, then enter the nasserver's address as device (e.g.\n"\
		"\t\thost.name.com:0; it *must* contain a colon)\n"\
		"\t--threads/-t=<amount>: Numbers of threads to use for buffering\n"\
		"\t\t(only works if mp3blaster was compiled with threads). Range is \n"\
		"\t\t0..500 in increments of 50 only.\n"\
		"\t--version,v: Display version number.\n"\
		"");

	exit(1);
}

/* Function   : calc_mainwindow_width
 * Description: Calculates the width of the main window (the big
 *            : scrollable one) based on the current term size
 * Parameters : termWidth
 *            :  Current width of the terminal
 * Returns    : The calculated width
 * SideEffects: None.
 */
int
calc_mainwindow_width(int termWidth) {
	int result = termWidth - 12;
	if (result < 1)
		result = 1;
	
	return result;
}

/* Function   : calc_mainwindow_height
 * Description: Calculates the height of the main window (the big
 *            : scrollable one) based on the current term size
 * Parameters : termHeight
 *            :  Current height of the terminal
 * Returns    : The calculated height
 * SideEffects: None.
 */
int
calc_mainwindow_height(int termHeight) {
	int result = termHeight - 13;

	if (result < 1)
		result = 1;
	
	return result;
}


/* Function   : fw_begin
 * Description: Enters file manager mode. Program should be in normal mode
 *            : when this function's called.
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
fw_begin()
{
	quit_after_playlist = 0;
	change_program_mode(PM_FILESELECTION);
	fw_draw_sortmode(globalopts.fw_sortingmode);
	if (file_window)
		delete file_window;
	file_window = new fileManager(NULL, calc_mainwindow_height(LINES),
		calc_mainwindow_width(COLS), 10, 0, CP_DEFAULT, 1);
	file_window->drawTitleInBorder(1);
	file_window->setBorder(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_LTEE, ACS_PLUS, ACS_LTEE, ACS_PLUS);
	if (globalopts.want_id3names)
		file_window->setDisplayMode(2); //show as ID3 names
	else
		file_window->setDisplayMode(1); //show file sizes as well
	file_window->setWrap(globalopts.wraplist);
	file_window->swRefresh(0);
	draw_static(1);
}

/* fw_end closes the file_window if it's on-screen, and moves all selected
 * files into the currently used group. Finally, the selection-window of the
 * current group is put back on the screen and the title in the header-window
 * is set to match that of the current group. If files were selected in another
 * dir than the current dir, these will be added too (they're stored in 
 * selected_files then). selected_files will be freed as well.
 */
void
fw_end()
{
	int
		i,
		nselected;
	char
		**selitems = NULL;
	mp3Win
		*sw = mp3_curwin;

	if (progmode != PM_FILESELECTION) /* or: !file_window */
		return;

	selitems = file_window->getSelectedItems(&nselected); 
	
	for ( i = 0; i < nselected; i++)
	{
		if (!is_audiofile(selitems[i]) || is_dir(selitems[i]))
			continue;

		const char *path = file_window->getPath();
		char *itemname = (char *)malloc(strlen(selitems[i]) +
			strlen(path) + 2);
		if (path[strlen(path) - 1] != '/')
			sprintf(itemname, "%s/%s", path, selitems[i]);
		else
			sprintf(itemname, "%s%s", path, selitems[i]);
	
		add_selected_file(itemname);
		free(itemname);
	}

	if (nselected)
	{
		for (i = 0; i < nselected; i++)
			delete[] selitems[i];
		delete[] selitems;
	}

	if (selected_files)
	{
		short foo[2] = { 0, 1 };
		const char *bla[4];
		int idx;
		for (i = 0; i < nselfiles; i++)
		{
			bla[0] = (const char*)selected_files[i];
			if (is_httpstream(selected_files[i]))
				bla[1] = (const char*)selected_files[i];
			else
				bla[1] = chop_path(selected_files[i]);
			bla[2] = NULL;
			bla[3] = NULL;
			idx=sw->addItem(bla, foo, CP_FILE_MP3);
			if (idx!=-1)
				sw->changeItem(idx,&id3_filename,strdup(bla[0]),2);
			free(selected_files[i]);
		}
		free(selected_files);
		selected_files = NULL;
		nselfiles = 0;
	}

	delete file_window;
	file_window = NULL;
	change_program_mode(PM_NORMAL);

	sw->swRefresh(1);
	draw_static();
	mp3_curwin->swRefresh(2);
}

void
fw_changedir(const char *newpath = 0)
{
	if (progmode != PM_FILESELECTION)
		return;

	int
		nselected = 0;
	char 
		**selitems;
	const char
		*path;

	if (!newpath)
		path = file_window->getSelectedItem();
	else
		path = newpath;

	selitems = file_window->getSelectedItems(&nselected);

	/* if not changed to current dir and files have been selected add them to
	 * the selection list 
	 */
	if (strcmp(path, ".") && nselected)
	{
		int i;

		for (i = 0; i < nselected; i++)
		{
			const char
				*pwd = file_window->getPath();
			char
				*file = (char *)malloc((strlen(pwd) + strlen(selitems[i]) +
					2) * sizeof(char));

			strcpy(file, pwd);
			if (pwd[strlen(pwd) - 1] != '/')
				strcat(file, "/");
			strcat(file, selitems[i]);

			if (is_audiofile(file))
				add_selected_file(file);

			free(file);
			delete[] selitems[i];
		}
		delete[] selitems;
	}
	
	/* If the old dir is a directory in the new dir, set the current
	 * selection to the entry for that dir. */
	const char *oldPath = file_window->getPath();
	char *foundItem = NULL;
	if (oldPath != NULL)
	{
		if (strcmp(path, "../") == 0) {
			const char *oldPtr = oldPath + strlen(oldPath) - 1;
			if (oldPtr > oldPath && *oldPtr == '/')
				oldPtr--;
			while (oldPtr >= oldPath)
			{
				if (*oldPtr == '/') {
					foundItem = strdup(oldPtr + 1);
					break;
				}
				oldPtr--;
			}
		}
		else if (strcmp(path, "./") == 0)
			foundItem = strdup(path);
	}

	file_window->changeDir(path);

	if (foundItem != NULL)
	{
		int index = file_window->findItem(foundItem);
		if (index != -1)
			file_window->setItem(index);
		free(foundItem);
	}

	refresh();
}

/* PRE: Be in PM_FILESELECTION
 */
void
fw_getpath(const char *npath, void *opts)
{
	DIR
		*dir = NULL;
	char
		*newpath;
	
	if (opts);

	if (!npath || !(newpath = expand_path(npath)))
		return;

	if ( !(dir = opendir(newpath))) 
	{
		free(newpath);
		return;
	}
	closedir(dir);
	fw_changedir(newpath);
	free(newpath);
	return;
}

void
draw_play_key(short do_refresh)
{
	char klabel[4];
	int maxy,maxx;
	int r;

	getmaxyx(stdscr, maxy, maxx);
	r = maxx - 12;


	attrset(COLOR_PAIR(CP_LABEL));

	move(maxy-9,r+4);
	
	lock_playing_mutex();
	if (!playopts.playing || (playopts.playing && playopts.song_played && playopts.pause))
		addstr(" |>");
	else
		addstr(" ||");
	unlock_playing_mutex();

	get_label(CMD_PLAY_PLAY, klabel);
	move(maxy-8,r+4); addstr(klabel);

	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);

	if (do_refresh)
		refresh();
}

/* Draws static interface components (and refreshes the screen). */
void
draw_static(int file_mode)
{
	int maxy, maxx, r;
	char klabel[4];

	getmaxyx(stdscr,maxy,maxx);
	r = maxx - 12;

	if (file_mode);
	//First, the border
	box(stdscr, ACS_VLINE, ACS_HLINE);
	//Now, the line under the function keys and above the status window
	move(5,1); hline(ACS_HLINE, maxx-2);
	move(5,0); addch(ACS_LTEE); move(5,maxx-1); addch(ACS_RTEE);
	move(maxy-4,1); hline(ACS_HLINE, maxx-2);
	move(maxy-4,0); addch(ACS_LTEE); move(maxy-4,maxx-1); addch(ACS_RTEE);

	//draw the box bottom-right from the function keys
	move(10,r); hline(ACS_HLINE, 11);
	move(6,r-1); vline(ACS_VLINE,4);
	move(5,r-1); addch(ACS_TTEE);
	move(10,maxx-1); addch(ACS_RTEE);
	//fix upperleft corner for playlist window
	move(10,0); addch(ACS_LTEE);
	move(11,r); addch('[');
	move(11,r+2); addstr("] shuffle");
	move(12,r); addch('[');
	move(12,r+2); addstr("] repeat");
	move(9,r); addstr("Threads:");

	move(0,COLS-6);
	addstr("(   )");
	move(5,COLS-6);
	addstr("(   )");
	attrset(COLOR_PAIR(CP_BUTTON));
	get_label(CMD_HELP_PREV, klabel);
	move(0,COLS-5); addnstr(klabel, 3);
	get_label(CMD_HELP_NEXT, klabel);
	move(5,COLS-5); addnstr(klabel, 3);
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	move(maxy-4,maxx-13); addch(ACS_PLUS);
	move(maxy-3,maxx-13); addch(ACS_VLINE);
	move(maxy-2,maxx-13); addch(ACS_VLINE);
	move(maxy-1,maxx-13); addch(ACS_BTEE);
	//draw cd-style keys

	draw_play_key(0);
	attrset(COLOR_PAIR(CP_LABEL));
	move(maxy-9,  r); addstr(" |<"); move(maxy-8,  r);
	get_label(CMD_PLAY_PREVIOUS, klabel); addstr(klabel);
	move(maxy-9,r+8); addstr(" >|"); move(maxy-8,r+8);
	get_label(CMD_PLAY_NEXT, klabel); addstr(klabel);
	move(maxy-6,  r); addstr(" <<"); move(maxy-5,  r);
	get_label(CMD_PLAY_REWIND, klabel); addstr(klabel);
	move(maxy-6,r+4); addstr(" []"); move(maxy-5,r+4);
	get_label(CMD_PLAY_STOP, klabel); addstr(klabel);
	move(maxy-6,r+8); addstr(" >>"); move(maxy-5,r+8);
	get_label(CMD_PLAY_FORWARD, klabel); addstr(klabel);
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);

	if (!globalopts.no_mixer)
	{
		char klabel[4];
		get_label(CMD_TOGGLE_MIXER, klabel);

		move(maxy-4,maxx-12);
		addstr("[   ]Mixer");
		move(maxy-4, maxx-11);
		attrset(COLOR_PAIR(CP_BUTTON));
		addnstr(klabel, 3);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	}
	//Draw Next Song:
	move(7,2);
	addstr("Next Song      :");

	refresh();
	return;
}

void
refresh_screen()
{
	wrefresh(curscr);
}

/*
 * Returns the main (scrollable) window. This windows is
 * Dependant on the context the program is currently in.
 */
scrollWin *
getMainWindow()
{
	scrollWin * mainwindow = (scrollWin *)mp3_curwin;

	if (progmode == PM_FILESELECTION)
		mainwindow = (scrollWin *)file_window;
	else if (progmode == PM_HELP)
		mainwindow = (scrollWin *)bighelpwin;

	return mainwindow;
}

/* Function   : init_mixer
 * Description: (re)creates the mini mixer in the bottom right of the
 *            : screen, but only if a mixer is desired.
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
init_mixer()
{
	/* initialize mixer */
	if (!globalopts.no_mixer)
	{
		const int color_pairs[4] = { 13,14,15,16 };
		int mixerXOffset = COLS - 12;
		int mixerYOffset = LINES - 3;

		if (mixer) {
			delete mixer;
		}

		//sanity check against too small screen[resize]s
		if (mixerXOffset < 0)
			mixerXOffset = 0;
		if (mixerYOffset < 0)
			mixerYOffset = 0;

		mixer = new NMixer(stdscr, globalopts.mixer_device, mixerXOffset,
			mixerYOffset, 2, color_pairs, globalopts.colours.default_bg, 1);
		if (!(mixer->NMixerInit()))
		{
			delete mixer;
		}
		else
		{
			//TODO: These should be customizable too..
			mixer->setMixerCommandKey(MCMD_NEXTDEV, 't');
			mixer->setMixerCommandKey(MCMD_PREVDEV, 'T');
			mixer->setMixerCommandKey(MCMD_VOLUP, '>');
			mixer->setMixerCommandKey(MCMD_VOLDN, '<');
		}
	}
}


void
handle_resize(int dummy = 0)
{
	int newWindowWidth;
	int newWindowHeight;
	(void)dummy; //unused param

	/* re-initialize ncurses, this updates LINES & COLS */
	endwin();
	refresh();
	clear();

	newWindowWidth = calc_mainwindow_width(COLS);
	newWindowHeight = calc_mainwindow_height(LINES);

	try {
		//resize the window that's currently onscreen.
		getMainWindow()->resize(newWindowWidth, newWindowHeight);
		//resize all playlist windows recursively recursively. Just one of them
		//may be onscreen.
		mp3_rootwin->resize(newWindowWidth, newWindowHeight, 1);
	} catch (IllegalArgumentsException e) {
		endwin();
		fprintf(stderr, "Resize too small!\n");
		exit(1);
	}

	draw_settings();
	init_helpwin();
	init_mixer();
	repaint_help();
	getMainWindow()->swRefresh(2);
	draw_static();
	//redraw current song status / info
	update_ncurses(1);
	refresh_screen();
}


/* Adds a new group to mp3_curwin and returns it.
 */
/* Function   : add_group
 * Description: Adds a new group to mp3_curwin and returns it.
 * Parameters : newgroupname: Name of the group. If no name is supplied,
 *            : [Default] is used. If the groupname does not start and end with
 *            : '[' and ']', these will be added.
 * Returns    : The newly created group.
 * SideEffects: None.
 */
mp3Win *
add_group(const char *newgroupname=0)
{
	mp3Win
		*sw;

	sw = newgroup();
	if (!newgroupname)
		newgroupname = "[Default]";
	sw->setTitle(newgroupname);
	mp3_curwin->addGroup(sw, newgroupname);
	mp3_curwin->swRefresh(1);
	return sw;
}

/* Function   : set_group_name
 * Description: Sets the group name of the currently highlighted group. 
 *            : It is called from popup_win.
 * Parameters : name: Name to give to group. free() after use.
 *            : args: not used
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
set_group_name(const char *name, void *args)
{
	const char
		*nm;
	mp3Win
		*sw = mp3_curwin;
	int
		index;

	if (args);

	if (!sw)
		return;
	index = sw->sw_selection;

	if (!sw->isGroup(index) || !(nm = sw->getItem(index)) || !strcmp(nm, "[..]"))
		return;

	if (!strcmp(name, ".."))
	{
		return;
	}
	char tmpname[strlen(name)+3];
	sprintf(tmpname, "[%s]", name);	
	sw->changeItem(index, tmpname);
	mp3Win *gr = sw->getGroup(index);
	if (gr)
		gr->setTitle(tmpname);
}

void
mw_clear()
{
	int width = (COLS > 14 ? COLS - 14 : 1);
	char emptyline[width+1];

	//clear old text
	memset(emptyline, ' ', width * sizeof(char));
	emptyline[width] = '\0';

	move(LINES-2,1);
	addstr(emptyline);
}

void
mw_settxt(const char *txt)
{
	mw_clear();
	move(LINES-2,1);
	addnstr(txt, (COLS > 14 ? COLS-14 : 1));
	refresh();
}

/* PRE: f is opened
 * POST: if a group was successfully read from f, non-zero will be returned
 *       and the group is added.
 */
int
read_group_from_file(FILE *f)
{
	return 0 || f; //gotta fix this..
#if 0
	int
		linecount = 0;
	char
		*line = NULL;
	short
		read_ok = 1,
		group_ok = 0;
	scrollWin
		*sw = NULL;

	while (read_ok)
	{
		line = readline(f);

		if (!line)
		{
			read_ok = 0;
			continue;
		}
		
		if (linecount == 0)
		{
			char
				*tmp = (char*)malloc((strlen(line) + 1) * sizeof(char));
			int
				success = sscanf(line, "GROUPNAME: %[^\n]", tmp);	

			if (success < 1) /* bad group-header! */
				read_ok = 0;
			else /* valid group - add it */
			{
				char
					*title;

				group_ok = 1;
				sw = group_stack->entry(globalopts.current_group - 1);

				/* if there are no entries in the current group, the first
				 * group will be read into that group.
				 */
				if (sw->getNitems())
				{	
					add_group();
					sw = group_stack->entry(group_stack->stackSize() - 1);
				}
				title = (char*)malloc((strlen(tmp) + 16) * sizeof(char));
				sprintf(title, "%02d:%s", globalopts.current_group, tmp);
				sw->setTitle(tmp);
				sw->swRefresh(0);
				free(title);
			}

			free(tmp);
		}
		else if (linecount == 1) /* read play order from file */
		{
			char
				*tmp = new char[strlen(line) + 1];
			int
				success = sscanf(line, "PLAYMODE: %[^\n]", tmp);	

			if (success < 1) /* bad group-header! */
			{
				read_ok = 0; group_ok = 0;
			}
			else
				((mp3Win*)sw)->setPlaymode((short)(atoi(tmp) ? 1 : 0));

			delete[] tmp;
		}
		else
		{
			if (!strcmp(line, "\n")) /* line is empty, great. */
				read_ok = 0;
			else
			{
				char *songname = new char[strlen(line) + 1];
					
				sscanf(line, "%[^\n]", songname);

				if (is_audiofile(songname))
				{
					const char *bla[4];
					short foo[2] = { 0, 1 };
					int idx;
					bla[0] = (const char*)songname;
					bla[1] = chop_path(songname);
					bla[2] = NULL;
					bla[3] = NULL;
					idx=sw->addItem(bla, foo, CP_FILE_MP3);
					if (idx!=-1)
					{
						sw->changeItem(idx,&id3_filename,strdup(bla[0]),2);
				}
				}

				delete[] songname;
			}
			/* ignore non-mp3 entries.. */
		}

		free(line);
		++linecount;
	}

	if (group_ok)
		sw->swRefresh(1);

	return group_ok;
#endif
}

/* return value = allocated using new, so delete[] it */
void
popup_win
(
	const char **label, 
	void (*func)(const char *, void *), void *func_args,
	const char *init_str,
	short want_input, int maxlen
)
{
	int maxx, maxy, width, height, by, bx, i, j;

	popup = 1; /* global variable, indicating input status */
	getmaxyx(stdscr, maxy, maxx);
	width = maxx - 14;
	height = maxy - 15;
	by = 11; bx = 1;
	if (!maxlen)
		maxlen = width - 2;
	bkgdset(COLOR_PAIR(CP_POPUP)|A_BOLD);
	//color the warning screen. It's really awkward, but it works..
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			//move(by+i, bx+j); addch('x'); 
			move(by+i, bx+j); addch(' '); 
		}
	}
	//write text
	i = 0;
	while (label[i])
	{
		move(by+i+2, bx+1); addstr(label[i++]);
	}
	if (want_input)
	{
		//attrset(A_BOLD|COLOR_PAIR(CP_POPUP_INPUT));
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
		bkgdset(COLOR_PAIR(CP_DEFAULT));
		for (i = 1; i < width -1; i++)
		{
			move(by + height - 2, bx + i); addch(' ');
		}
		move(by+height-2, bx+1);
		echo();
		leaveok(stdscr, FALSE);
		curs_set(1);
		refresh();
#if 0
		text = new char[MIN(width-2,maxlen)+1];
		getnstr(text, MIN(width-2,maxlen));
#else
		mw_settxt("Use up and down arrows to toggle insert mode");
		get_input(stdscr, init_str, width - 2, maxlen, by + height -2,
			bx + 1, func, func_args);
		return;
		/*
		set_inout_opts();
		*/
#endif
	}
	/* TODO: IIRC want_input is always true, so this part is never called */
#if 0
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	bkgdset(COLOR_PAIR(CP_DEFAULT));
	popup = 0;
	refresh_screen();
	return text;
#endif
}

//playlist functions
struct ptag_t
parse_playlist_token(const char *str)
{
	struct ptag_t restag, badtag;
	short status = 0;
	/* meaning of status (state in state machine):
	 * 0: scanning the tagname
	 * 1: scanning whitespace (searching for keyword)
	 * 2: scanning keywordname
	 * 3: scanning '=' separating keyword/value
	 * 4: scanning '"' that starts value
	 * 5: scanning value
	 */
	short tagcnt = 0;
	short current_keyword = 0;
	short current_value = 0;
	char token, last_token;
	short i;
	char *mystr = strdup(str);

	restag.tag[0] = badtag.tag[0] = '\0';
	for (i = 0; i < 5; i++)
	{
		restag.keywords[i][0] = badtag.keywords[i][0] = '\0';
		restag.values[i][0] = badtag.values[i][0] = '\0';
	}

	if (!mystr || !strlen(mystr) || strlen(mystr) < 2 || mystr[0] != '<' ||
		mystr[strlen(mystr)-1] != '>')
		return badtag;
	
	last_token = '\0';
	mystr[strlen(mystr)-1] = '\0';

	while(*(++mystr))
	{
		token = *mystr;

		if (!status) //still completing keyword
		{
			if (token == ' ' || token == '\t')
			{
				status++; //look for keywords now
				restag.tag[tagcnt] = '\0';
				tagcnt = 0;
				last_token = token;
				continue;
			}

			last_token = token;

			if (tagcnt > 78) //just chop off too long tagnames
				continue;

			restag.tag[tagcnt++] = token;
		}
		else if (status == 1 || status == 2) //looking for the start o/a keyword
		{
			if (status == 1)
			{
				if (token == ' ' || token == '\t')
					continue;
				else
					status = 2; //found keyword
			}
				

			if (token == ' ' || token == '\t' || token == '=') //end of keyword
			{
				restag.keywords[current_keyword][tagcnt] = '\0';
				tagcnt = 0;
				current_keyword++;
				last_token = token;
				tagcnt = 0;
				if (token == '=')
					status = 4;
				else
					status = 3;
				continue;
			}

			last_token = token;

			if (tagcnt > 78)
				continue;

			restag.keywords[current_keyword][tagcnt++] = token;
		}
		else if (status == 3) //searching for =
		{
			if (token == '=')
			{
				last_token = token;
				status = 4;
			}
			else
			{
				last_token = token;
			}
		}
		else if (status == 4) //looking for " starting value;
		{
			if (token != '"')
			{
				last_token = token;
				continue;
			}
			else
			{
				status = 5; //start of value
				last_token = token;
			}
		}
		else if (status == 5) //scanning value
		{
			if (token == '"' && last_token != '\\')
			{
				last_token = token;
				status = 1; //start looking for keyword again
				restag.values[current_value][tagcnt] = '\0';
				current_value++;
				tagcnt = 0;
				continue;
			}

			if (tagcnt > 78)
			{
				last_token = token;
				continue;
			}

			if (token != '\\' || last_token == '\\')
				restag.values[current_value][tagcnt++] = token;

			last_token = token;

		}
	}

	if (!status) //a tag with just a tagname is valid too ofcourse
	{
		restag.tag[tagcnt] = '\0';
		status = 1;
	}

	if (status != 1) //not good.
		return badtag;
	
	debug("Scanning done\n");
	{
		short i;

		debug("tag[tag]: \"%s\"\n", restag.tag);
		for (i = 0; i < 5; i++)
		{
			if (restag.keywords[i][0])
			{
				debug("tag.keywords[%d] = \"%s\"\n", i, restag.keywords[i]);
			}
			if (restag.values[i][0])
			{
				debug("tag.values[%d] = \"%s\"\n", i, restag.values[i]);
			}
		}
	}

	return restag;
}

/* Function   : tag_keyword_value
 * Description: Searches for the presence of a keyword in a HTML-like tag
 *            : and returns the value if it's present. If the keyword is not
 *            : found, NULL is returned. Searches are case-insensitive.
 * Parameters : tag: the HTML-like tag, e.g. generated by parse_playlist_token
 * Returns    : See description.
 * SideEffects: None.
 * Example    : If the tag is something like <GROUP TYPE="shuffle" MODE="0">
 *            : and the requested keyword is "type", "shuffle" will be
 *            : returned.
 */
const char*
tag_keyword_value(const struct ptag_t * tag, const char *kword)
{
	int i;

	for (i = 0; i < 5; i++)
	{
		if (!strcasecmp(tag->keywords[i], kword))
		{
			return tag->values[i];
		}
	}
	return NULL;
}

/* Function   : read_playlist
 * Description: Reads a playlist from a file and adds the songs to the playlist
 * Parameters : filename: 
 *            :   Path to the playlist
 * Returns    : If succesful 1, else 0.
 * SideEffects: None.
 * TODO       : Adding groups seems a bit buggy to me. If sigsegv's happen,
 *            : this is probably the place to look for them.
 */
short
read_playlist(const char *filename)
{
	FILE
		*f;
	char
		*tmpline, *line;

	const char 
		*extension;

	mp3Win
		*current_group = mp3_curwin;
	int
		is_m3u = 0;

	f = NULL;
	line = tmpline = NULL;

	if (!filename || !(f = fopen(filename, "r")))
		return 0;
	
	if (strlen(filename)>=4)
	{
		extension = filename + (strlen(filename)-4);
		if (strcmp(".m3u", extension) == 0)
			is_m3u = 1;
	}

	while ( (tmpline = readline(f)) )
	{
		line = crop_whitespace((const char *)tmpline);
		free(tmpline);

		if (!strlen(line))
		{	
			free(line);
			continue;
		}

		if (is_m3u && strncmp(line, "#EXT", 4)==0)
		{
			free(line);
			continue;
		}

		if (line[0] == '<')
		{
			struct ptag_t ptag = parse_playlist_token((const char *)line);
			const char *keyval;

			if (!strcasecmp(ptag.tag, "global"))
			{
				keyval = tag_keyword_value(&ptag, "playmode"); 
				if (keyval)
				{
					set_play_mode(keyval);
					debug("Set playmode from playlist file\n");
				}

				keyval = tag_keyword_value(&ptag, "mode");
				if (keyval && !strcasecmp(keyval, "shuffle"))
				{
					mp3_rootwin->setPlaymode(1);
					debug("Shufflemode on!\n");
				}
			}
			else if (!strcasecmp(ptag.tag, "group"))
			{
				const char *group_name = tag_keyword_value(&ptag, "name");
				const char *group_mode = NULL;
				if (group_name)
				{
					char *gname = new char[strlen(group_name) + 3];
					memset(gname, '\0', (strlen(group_name) + 3) * sizeof(char));
					strcat(gname, "[");
					strncat(gname, group_name, 48);
					strcat(gname, "]");

					debug("GroupName: %s\n",gname);
					mp3Win *new_group = newgroup();
					if (!new_group)
						continue;

					new_group->setTitle(gname);
					if ( (group_mode = tag_keyword_value(&ptag, "mode")) &&
						!strcasecmp(group_mode, "shuffle"))
						new_group->setPlaymode(1);
					current_group->addGroup(new_group, gname);
					current_group = new_group;
				}
			}
			else if (!strcasecmp(ptag.tag, "/group"))
			{
				mp3Win *prev_group = current_group->getGroup(0); //should be [..]
				if (prev_group)
					current_group = prev_group;
			}

			free(line);
			continue;
		}
		
		const char *bla[4];
		short foo[2] = { 0, 1 };
		short idx;
		if (!strchr(line, '/'))
		{
			char *currentPath = get_current_working_path();
			currentPath = (char*)realloc(currentPath, (strlen(currentPath) +
				strlen(line) + 1) * sizeof(char));
			strcat(currentPath, line);
			free(line);
			line = currentPath;
			//file without a path in the playlist dir
		}
		bla[0] = (const char*)line;
		bla[1] = chop_path((const char *)line);
		bla[2] = NULL;
		bla[3] = NULL;
		idx=current_group->addItem(bla, foo, CP_FILE_MP3);
		if (idx!=-1)
			current_group->changeItem(idx,&id3_filename,strdup(bla[0]),2);
		free(line);
	}
	
	//refresh_screen();
	return 1;
}	

#if 0
	FILE
		*f;
	char 
		*name;
		
	if (!filename)
		name = gettext("Enter filename:", 1);
	else
	{	
		name = (char*)malloc((strlen(filename)  + 1) * sizeof(char));
		strcpy(name, filename);
	}
	
	/*append startup path if filename does not start with a slash */
	if (strlen(name) && name[0] != '/')
	{
		char *newname = (char*)malloc(strlen(startup_path) +
			strlen(name) + 1);
		strcpy(newname, startup_path);
		strcat(newname, name);
		free(name);
		name = newname;
	}

	f = fopen(name, "r");
	free(name);

	if (!f)
	{
		warning("Couldn't open playlist-file.");
		refresh_screen();
		return;
	}
	
	short header_ok = 1;
	/* read global playmode from file */
	{
		char
			*line = readline(f);
		playmode
			old_mode = globalopts.play_mode;

		if (!line)
		{
			fclose(f); return;
		}

		char tmp[strlen(line)+1];
		if ( sscanf(line, "GLOBALMODE: %[^\n]", tmp) == 1)
		{
			short dum = set_play_mode((const char *)tmp);

			if (!dum)
				header_ok = 0;
		}
		else
			header_ok = 0;

		if (header_ok && old_mode != globalopts.play_mode)
			cw_draw_play_mode();
		free(line);
	}	
		
	if (header_ok)
	{
		while (read_group_from_file(f))
			;
		cw_draw_group_mode();
	}
	else
	{
		warning("No global playmode in playlist found.");
		refresh_screen();
	}	

	fclose(f);
	//mw_settxt("Added playlist!");
#endif

short
write_group(mp3Win *group, FILE *f, short indent_level)
{
	int nitems = group->getNitems();
	int itemcount = 0;
	char *indent_string = new char[indent_level + 1];
	
	if (indent_level > 10)
		return 0;

	memset(indent_string, '\t', indent_level);
	indent_string[indent_level] = '\0';
	
	while (itemcount < nitems)
	{
		char *itemname = NULL;
		const char *tmpname = NULL;

		if (group->isGroup(itemcount))
		{
			if (strcmp(group->getItem(itemcount), "[..]")) //skip [..] !!!
			{
				mp3Win *new_group = group->getGroup(itemcount);
				const char *tmp_gname1 = new_group->getTitle();
				char *tmp_gname = strdup(tmp_gname1);
				tmp_gname[strlen(tmp_gname)-1] = '\0'; //chop off ']';
				char *gname = new char[strlen(indent_string) + strlen(tmp_gname) + 20];
				//TODO: escape "'s in groupname
				sprintf(gname, "%s<GROUP NAME=\"%s\"%s>\n", indent_string,
					(const char *)(tmp_gname + 1), (new_group->getPlaymode() ? " MODE=\"shuffle\"" : ""));
				debug(gname);
				free(tmp_gname);
				if (fwrite(gname, sizeof(char), strlen(gname), f) < strlen(gname) *
					sizeof(char))
				{
					debug("Short write!\n");
					delete[] gname;
					return 0;
				}
				delete[] gname; gname = NULL;

				write_group(new_group, f, indent_level + 1);

				gname = new char[strlen(indent_string) + 10];
				sprintf(gname, "%s</GROUP>\n", indent_string);
				if (fwrite(gname, sizeof(char), strlen(gname), f) < strlen(gname) *
					sizeof(char))
				{
					delete[] gname;
					return 0;
				}
				delete[] gname;
			}
		}
		else
		{
			tmpname = group->getItem(itemcount);
			itemname = new char[strlen(indent_string) + strlen(tmpname) + 2];
			sprintf(itemname, "%s%s\n", indent_string, tmpname);
			if (fwrite(itemname, sizeof(char), strlen(itemname), f) < 
				strlen(itemname) * sizeof(char))
			{
				delete[] itemname;
				return 0;
			}

			delete[] itemname;
		}
		itemcount++;
	}

	return 1;
}

short
write_playlist(const char *filename)
{
	mp3Win *current_group = mp3_rootwin;
	FILE *f = fopen(filename, "w");
	const char *pmstr = NULL;
	char globalstr[255];

	if (!f)
		return 0;
	
	switch(globalopts.play_mode)
	{
		case PLAY_GROUP: pmstr = "onegroup"; break;
		case PLAY_GROUPS: pmstr = "allgroups"; break;
		//case PLAY_GROUPS_RANDOMLY: pmstr = "groupsrandom"; break;
		case PLAY_SONGS: pmstr = "allrandom"; break;
		default: pmstr = "unknown";
	}

	//write global stuff
	sprintf(globalstr, "<GLOBAL PLAYMODE=\"%s\"%s>\n", pmstr,
		(mp3_rootwin->getPlaymode() ? " MODE=\"shuffle\"" : ""));
	if (fwrite(globalstr, sizeof(char), strlen(globalstr), f) !=
	    strlen(globalstr) * sizeof(char))
	{
		fclose(f);
		return 0;
	}

	if (!write_group(current_group, f, 0))
	{
		fclose(f);
		return 0;
	}

	fclose(f);
	return 1;
}

#if 0
	char *name = NULL;
	
	name = gettext("Enter filename", 1);
	/*append startup path if filename does not start with a slash */
	if (strlen(name) && name[0] != '/')
	{
		char *newname = (char*)malloc(strlen(startup_path) +
			strlen(name) + 1);
		strcpy(newname, startup_path);
		strcat(newname, name);
		free(name);
		name = newname;
		//fprintf(stderr, "New path: %s\n", name);
	}

	if (!(group_stack->writePlaylist(name, globalopts.play_mode)))
		Error("Error writing playlist!");

#if 0
	f = fopen(name, "w");
	free(name);

	if (!f)
	{
		Error("Error opening playlist for writing!");
		return;
	}
	
	for (i = 0; i < group_window->getNitems(); i++)
	{
		if ( !((group_stack->entry(i))->writeToFile(f)) ||
			!(fwrite("\n", sizeof(char), 1, f)))
		{
			Error("Error writing playlist!");
			fclose(f);
			return;
		}
	}
	
	fclose(f);
#endif
#endif

void
cw_toggle_group_mode()
{
	mp3Win
		*sw = mp3_curwin;
	sw->setPlaymode(1 - sw->getPlaymode());
	cw_draw_group_mode();
	refresh();
}

/* draws (or clears) group mode. Does *not* refresh screen */
void
cw_draw_group_mode()
{
	mp3Win
		*sw = mp3_curwin;
	int r = COLS - 12;

	move(11, r+1);
	if (!sw->getPlaymode())
		addch(' ');
	else
		addch('X');
}

/* set playmode */
short
set_play_mode(const char *pm)
{
	if (!pm)
		return 0;

	if (!strcasecmp(pm, "onegroup"))
		globalopts.play_mode = PLAY_GROUP;
	else if (!strcasecmp(pm, "allgroups"))
		globalopts.play_mode = PLAY_GROUPS;
	//else if (!strcasecmp(pm, "groupsrandom"))
	//	globalopts.play_mode = PLAY_GROUPS_RANDOMLY;
	else if (!strcasecmp(pm, "allrandom"))
		globalopts.play_mode = PLAY_SONGS;
	else
		return 0;

	return 1;
}

/* toggle & redraw playmode */
void
cw_toggle_play_mode()
{
	switch(globalopts.play_mode)
	{
		case PLAY_GROUPS: globalopts.play_mode = PLAY_GROUP; break;
		case PLAY_GROUP:  globalopts.play_mode = PLAY_SONGS; break;
		case PLAY_SONGS: globalopts.play_mode = PLAY_GROUPS; break;
		default: globalopts.play_mode = PLAY_GROUPS;
	}
	cw_draw_play_mode();
	refresh();
}

/* redraw playmode, but does not update the screen. */
void
cw_draw_play_mode(short cleanit)
{
	unsigned int i;
	const char *playmodes_desc[] = {
		/* PLAY_NONE */
		"",
		// PLAY_GROUP
		"Global Playback: Play current group, but not its subgroups",
		//PLAY_GROUPS
		"Global Playback: Play current group, including subgroups",
		//PLAY_GROUPS_RANDOMLY
		"Global Playback: You Should Not See This",
		//PLAY_SONGS
		"Global Playback: Shuffle all songs from all groups",
	};


	unsigned int nrchars = (COLS > 16 ? COLS - 16 : 1);
	for (i = 2; i < nrchars + 2; i++)
	{
		move(6, i);
		addch(' ');
	}
	if (!cleanit)
	{
		move(6, 2);
		addstr(playmodes_desc[globalopts.play_mode]);
	}
}

void *
play_list(void *arg)
{
	(void)arg; //unused

#ifdef PTHREADEDMPEG
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL),
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
#endif

	//this loops controls the 'playing' variable. The 'action' variable is
	//controlled by the input thread.
	//playing=1: user wants to play
	//playing=0: user doesn't want to play
	while (1)
	{
		_action my_action;

#ifdef PTHREADEDMPEG
		pthread_testcancel();
#elif defined(LIBPTH)
		pth_cancel_point();
#endif
		lock_playing_mutex();
		if (!playopts.playing)
		{
			debug("Not playing, waiting to start playlist\n");
			while (action != AC_PLAY && action != AC_ONE_MP3)
			{
				debug("THREAD playing going to sleep.\n");
#ifdef PTHREADEDMPEG
				pthread_cond_wait(&playing_cond, &playing_mutex);
#elif defined(LIBPTH)
				pth_cond_await(&playing_cond, &playing_mutex, NULL);
#endif
				if (playopts.quit_program)
				{
					break; //we need to die.
				}
			}
			debug("THREAD playing woke up!\n");
		}

		/* are we requested to die? */
		if (playopts.quit_program)
		{
			if (playopts.player)
			{
				//clean up the player
				debug("destroying player - we're quitting\n");
				playopts.player->stop();
				delete playopts.player;
				debug("Destroyed!\n");
			}

			//exit loop and terminate thread
			break;
		}


		my_action = action;
		playopts.playing = 1;
		unlock_playing_mutex();

		switch(my_action)
		{
		case AC_STOP:
		case AC_STOP_LIST:
			/* explicit user stop request */
			stop_song(); //sets action to AC_NONE
			lock_playing_mutex();
			playopts.playing = 0;
			unlock_playing_mutex();
			update_songinfo(PS_NONE, 16);
			if (my_action == AC_STOP_LIST)
				reset_playlist(1);
			playopts.playing_one_mp3 = 0;
		break;
		case AC_NEXT:
			stop_song(); //status => AC_NONE
			/*29-10-2000: This isn't really necessary..the next loop will fix this.
			if (!start_song())
				stop_list();
			 */
		break;
		case AC_PREV: //previous song
			if (!history->atStart())
			{
				/* skip 2 places in history; once to skip current song, once more to
				 * jump to song before current one
				 */
				history->previous();
				history->previous(); 
				debug ("Not at end of history, selecting older song.\n");
				stop_song(); //status => AC_NONE 
			}
			//mw_settxt("Not implemented (yet).");
			break;
		case AC_NONE:
		{
			if (!playopts.pause && playopts.song_played) {
				bool mystatus = (playopts.player)->run(globalopts.fpl);
				if (!mystatus) {
					if ((playopts.player)->ready()) {
						/* ugly. If run returns false, all playback should be finished! */
						stop_song(); //status => AC_NONE
					} else {
						//output buffer full, wait a bit.
						USLEEP(10000);
					}
				} else {
					/* TODO: different thread for status updates. Now, this function
					 * gets called way too often when buffering. It does prevent
					 * updating the actual display too often, but that check is
					 * costly as well..
					 */
					update_play_display();
					USLEEP(10000);
				}
			}

			if (!playopts.song_played) //let's start one then
			{
				short
						start_song_result;

				debug("No song to play, let's find one!\n");

				start_song_result = start_song();

				if (start_song_result == -1) //nothing more to play
				{
					short allow_repeat = playopts.allow_repeat;

					debug("End of playlist reached.\nI want to repeat: %d\n" \
						"Am I allowed to?: %d\n", globalopts.repeat, allow_repeat);
					stop_list();
					//end the program?
					if (quit_after_playlist)
						end_program(); //this is *so* not thread-safe!
					if (allow_repeat && globalopts.repeat)
					{
						debug("I may repeat the playlist.\n");
						reset_playlist(1);
						lock_playing_mutex();
						playopts.playing = 1;
						unlock_playing_mutex();
					}
				}
			}
			else if (playopts.pause)
				USLEEP(10000);
		}
		break;
		case AC_ONE_MP3: //plays 1 mp3, can interrupt a playlist
		{
			short was_playing = 0;
			if (playopts.song_played)
			{
				stop_song(); //action => AC_NONE
				if (!playopts.playing_one_mp3 || playopts.playing_one_mp3 == 1)
					was_playing = 1;
			}
			playopts.playing_one_mp3 = 0;
			start_song(was_playing); //(fails: action => AC_NONE, plist continues)
		}
		break;
		case AC_FORWARD:
			if (playopts.song_played && playopts.player)
			{
				(playopts.player)->forward(globalopts.skipframes);
				update_play_display();
			}
			set_action(AC_NONE);
		break;
		case AC_REWIND:
			if (playopts.song_played && playopts.player)
			{
				(playopts.player)->rewind(globalopts.skipframes);
				update_play_display();
			}
			set_action(AC_NONE);
		break;
		case AC_SKIPEND:
			if (playopts.song_played)
			{
				(playopts.player)->forward(-300);
				update_play_display();
			}
			set_action(AC_NONE);
		break;
		case AC_PLAY: //pause,unpause,na stop
			set_action(AC_NONE);
			if (playopts.song_played && playopts.pause)
			{
				playopts.pause = 0;
				update_songinfo(PS_PLAY, 16|4);
				if (!((playopts.player)->unpause()))
					stop_song();
			}
			else if (playopts.song_played && !playopts.pause)
			{
				playopts.pause = 1;
				update_songinfo(PS_PAUSE, 16|4);
				(playopts.player)->pause();
			}
			else if (!playopts.song_played)
			{
				playopts.allow_repeat = 0;
				debug("AC_PLAY: allow_repeat = 0\n");
			}
		break;
		case AC_NEXTGRP:
			if ((globalopts.play_mode == PLAY_GROUP || globalopts.play_mode ==
				PLAY_GROUPS) && playopts.song_played && playing_group)
			{
				stop_song();
				playing_group->setAllSongsPlayed();
			}
		break;
		case AC_PREVGRP:
			mw_settxt("Sorry, that doesn't work yet.");
		break;
		}
#ifdef LIBPTH
		pth_yield(NULL);
#endif
	}

	return NULL;
}

#ifdef INCLUDE_LIRC
void *
lirc_loop(void *arg)
{
	struct lirc_config *config;

	if (arg) {}

	if (lirc_init("mp3blaster", 1) == -1)
	{
		LOCK_NCURSES;
	  warning("Couldn't initialize LIRC support (no config?)");
		UNLOCK_NCURSES;
	  return NULL;
  }

	if (lirc_readconfig(NULL, &config, NULL) == 0)
	{
		char *code;
		char *name;
		int ret;

		while (lirc_nextcode(&code) == 0 && code) //blocks
		{
			while ((ret = lirc_code2char(config, code, &name)) == 0 && name)
			{
				if (!strcasecmp(name, "PLAY"))
					lirc_cmd = CMD_PLAY_PLAY; 	
				else if (!strcasecmp(name, "NEXT"))
					lirc_cmd = CMD_PLAY_NEXT; 	
				else if (!strcasecmp(name, "PREVIOUS"))
					lirc_cmd = CMD_PLAY_PREVIOUS;
				else if (!strcasecmp(name, "REWIND"))
					lirc_cmd = CMD_PLAY_REWIND;
				else if (!strcasecmp(name, "FORWARD"))
					lirc_cmd = CMD_PLAY_FORWARD;
				else if (!strcasecmp(name, "STOP"))
					lirc_cmd = CMD_PLAY_STOP;
				else if (!strcasecmp(name, "UP"))
					lirc_cmd = CMD_UP;
				else if (!strcasecmp(name, "DOWN"))
					lirc_cmd = CMD_DOWN;
				else if (!strcasecmp(name, "PLAYMODE"))
					lirc_cmd = CMD_TOGGLE_PLAYMODE;
				else if (!strcasecmp(name, "ENTER"))
				{
					if (progmode == PM_NORMAL)
						lirc_cmd = CMD_ENTER;
					else if (progmode == PM_FILESELECTION)
						lirc_cmd = CMD_FILE_ENTER;
				}
				else if (!strcasecmp(name, "FILES"))
				{
					if (progmode == PM_NORMAL)
						lirc_cmd = CMD_SELECT_FILES;
					else if (progmode == PM_FILESELECTION)
						lirc_cmd = CMD_FILE_ADD_FILES;
				}
				else if (!strcasecmp(name, "SELECT"))
				{
					if (progmode == PM_NORMAL)
						lirc_cmd = CMD_SELECT;
					else if (progmode == PM_FILESELECTION)
						lirc_cmd = CMD_FILE_SELECT;
				}
				else if (!strcasecmp(name, "QUIT"))
					lirc_cmd = CMD_QUIT_PROGRAM;
			}

			free(code);

			if (ret == -1)
				break;
		}

		lirc_freeconfig(config);
	}

	lirc_deinit();
	return NULL;
}
#endif

void
update_playlist_history()
{
	//TODO: implement ;-)
}

void
update_play_display()
{
	//update elapsed / remaining time?
	if (!playopts.player)
		return;

	time(&(playopts.newtyd));
	if (difftime(playopts.newtyd, playopts.tyd) >= 1)
	{
		if (LOCK_NCURSES)
		{
			songinf.elapsed = (playopts.player)->elapsed_time();
			songinf.remain = (playopts.player)->remaining_time();
			songinf.update |= 2;
			UPDATE_CURSES;
			UNLOCK_NCURSES;
		}
		playopts.tyd = playopts.newtyd;
	}
}

void 
set_one_mp3(const char *mp3)
{
#ifdef PTHREADEDMPEG
	pthread_mutex_lock(&onemp3_mutex);
#elif defined(LIBPTH)
	pth_mutex_acquire(&onemp3_mutex, FALSE, NULL);
#endif
	if (playopts.one_mp3)
		delete[] playopts.one_mp3;
	if (mp3)
	{
		playopts.one_mp3 = new char[strlen(mp3)+1];
		strcpy(playopts.one_mp3, mp3);
	}
	else
		playopts.one_mp3 = NULL;
#ifdef PTHREADEDMPEG
	pthread_mutex_unlock(&onemp3_mutex);
#elif defined(LIBPTH)
	pth_mutex_release(&onemp3_mutex);
#endif
}

/* POST: Delete[] returned char*-pointer when you don't use it anymore!! */
/* Function   : get_one_mp3
 * Description: Returns a char*-pointer with a filename of a single MP3,
 *            : if the user chose to play a single mp3 outside the usual
 *            : playlist (which happens when you hit enter over an mp3)
 * Parameters : None.
 * Returns    : A C++ allocated char*-pointer (delete[] after use), or NULL
 *            : if no single mp3 was requested.
 * SideEffects: None.
 */
char *
get_one_mp3()
{
	char *newmp3 = NULL;

	if (!playopts.one_mp3)
		return NULL;
	newmp3 = new char[strlen(playopts.one_mp3)+1];
	strcpy(newmp3,playopts.one_mp3);
	return newmp3;
}

void
stop_list()
{
	playopts.playing = 0;
	playopts.playing_one_mp3 = 0;
	debug("stop_list() : allow_repeat = 0\n");
	playopts.allow_repeat = 0;
	next_song = -1;
	reset_playlist(1);
	if (LOCK_NCURSES)
	{
		songinf.update |= 32;
		UPDATE_CURSES;
		UNLOCK_NCURSES;
	}
}

const char *
get_error_string(int errcode)
{
	const char *sound_errors[21] =
	{
		"Everything's OK (You shouldnt't see this!)",
		"Failed to open sound device.",
		"Sound device is busy.",
		"Buffersize of sound device is wrong.",
		"Sound device control error.",
	
		"Failed to open file for reading.",    // 5
		"Failed to read file.",                
	
		"Unkown proxy.",
		"Unkown host.",
		"Socket error.",
		"Connection failed.",                  // 10
		"Fdopen error.",
		"Http request failed.",
		"Http write failed.",
		"Too many relocations",
		
		"Out of memory.",               // 15
		"Unexpected EOF.",
		"Bad sound file format.",
		
		"Can't make thread.",
		"Bad mpeg header.",
		
		"Unknown error."
	};

	if (errcode < 0 || errcode > 20)
		return "";
	return sound_errors[errcode];
}

void
stop_song()
{
	int vaut;
	if (playopts.player)
	{
		(playopts.player)->stop();
		playopts.pause = 0;
		vaut = (playopts.player)->geterrorcode();
	}
	else
	{
		vaut = SOUND_ERROR_FINISH;
		debug("stop_song(): no playopts.player\n");
	}

	if (vaut == SOUND_ERROR_OK || vaut == SOUND_ERROR_FINISH)
	{
		debug("Allowing playlist repeat\n");
		playopts.allow_repeat = 1;
	}
	else
	{
		debug("stop_song() ended in error (allow_repeat = %d)\n",
			playopts.allow_repeat);
	}

	//generate warning
	if (LOCK_NCURSES)
	{
		songinf.update = 0;
		if (songinf.warning)
		{
			delete[] songinf.warning;
			songinf.warning = NULL;
		}
		if (vaut != SOUND_ERROR_OK && vaut != SOUND_ERROR_FINISH)
		{
			songinf.warning = new char[strlen(get_error_string(vaut))+1];
			strcpy(songinf.warning, get_error_string(vaut));
			songinf.update |= 8;
		}
		songinf.status = PS_STOP;
		songinf.update |= 4;
		UPDATE_CURSES;
		UNLOCK_NCURSES;
	}

	if (playopts.player)
	{
		delete playopts.player;
		playopts.player = NULL;
	}
	playopts.song_played = 0;
	//playopts.playing_one_mp3 = 0; (DON'T!)
	//set_one_mp3((const char *)NULL); (DON'T!!)
	set_action(AC_NONE);
	if (vaut == SOUND_ERROR_DEVOPENFAIL)
		stop_list();
}

/* TODO: Implement history
 * PRE: This function can only be called from the playing thread, otherwise
 *    : we'll enter a deadlock.
 * was_playing: if one mp3 is to be played, this determines if the playlist
 *              continues playing after the one mp3 (is was_playing = 1, it
 *              will)
 * POST: playopts.one_mp3 is always NULL
 *
 * Returns: >0 on success, <=0 on failure:
 *  0: general error
 * -1: could not find song to play
 */
short
start_song(short was_playing)
{
	char *song = NULL;
	short one_mp3 = 0;
	const char *sp = NULL;

	debug("start_song()\n");
	/* If there's no standalone mp3 to be played, and a previous standalone mp3
	 * has just been played while there was no playlist playing, return without
	 * choosing a new song.
	 */
	if (!(song = get_one_mp3()) && playopts.playing_one_mp3 == 2)
	{
		debug("Not going to play a new song.\n");
		playopts.playing_one_mp3 = 0;
		return -1;
	}

	if (!song)
	{
		/* No request to play a standalone mp3 (A standalone mp3 is one that is
		 * being started by hitting enter over a song in file- or playlist
		 * manager; it can interrupt playlists)
		 */

		/* If we're in the playlist history, advance one position in the history */
		/*
		if (!history->atEnd())
		{
			debug("Advancing one entry in history\n");
			history->next();
		}
		*/

		/* If we're still in the playlist history, play a song from it. */
		if (!history->atEnd())
		{
			debug("Going to play song from history\n");
			const char *tmpsong = history->element();
			if (tmpsong)
			{
				song = new char[strlen(tmpsong) + 1];
				strcpy(song, tmpsong);
				debug("Select song from history (%s)\n", song);
			}
			history->next();
		}
		else //regular playlist
		{
			if ((song = determine_song()))
				history->add(song); //add to history.
		}

		if (!song)
			debug("Could not determine song in start_song()\n");
		playopts.playing_one_mp3 = 0;
		if (!song)
			return -1;
	}
	else //one_mp3
	{
		playopts.playing_one_mp3 = (was_playing ? 1 : 2);
		one_mp3 = 1;
		//let's not add standalone mp3's (those not in playlist) to the history.
		//it would be too confusing.
		//history->add(song);
	}

	set_one_mp3((const char*)NULL);

	void *init_args = NULL;
	Fileplayer::audiodriver_t audiodriver = Fileplayer::AUDIODRV_SDL;

	switch(globalopts.audio_driver)
	{
	case AUDIODEV_SDL: audiodriver = Fileplayer::AUDIODRV_SDL; break;
	case AUDIODEV_OSS: audiodriver = Fileplayer::AUDIODRV_OSS; break;
	case AUDIODEV_ESD: audiodriver = Fileplayer::AUDIODRV_ESD; break;
	case AUDIODEV_NAS: audiodriver = Fileplayer::AUDIODRV_NAS; break;
	//TODO: 'dump' driver which dumps output to a file! Better than current
	//crappy mp3->wav converter class..
	}

	if (is_wav(song))
		playopts.player = new Wavefileplayer(audiodriver);
#ifdef HAVE_SIDPLAYER
	else if (is_sid(song))
		playopts.player = new SIDfileplayer(audiodriver);
#endif
#ifdef INCLUDE_OGG
	else if (is_ogg(song))
		playopts.player = new Oggplayer(audiodriver);
#endif
	else if (is_audiofile(song)) //mp3 assumed
	{
		struct init_opts *opts = new init_opts;
		memset(opts->option, 0, 30);

#ifdef PTHREADEDMPEG
		int draadjes = globalopts.threads;
		if (draadjes)
			add_init_opt(opts, "threads", (void*)&(globalopts.threads));
#endif

		add_init_opt(opts, "scanmp3s", (void*)&(globalopts.scan_mp3s));

		//init options have been set?
		if (opts->option[0])
			init_args = (void *)opts;
		playopts.player = new Mpegfileplayer(audiodriver);
	}

	if (!playopts.player)
	{
		debug("start_song: No player object could be created!\n");
		stop_song();
		delete[] song;
		return 0;
	}
		
	if (!(playopts.player)->openfile(song, (globalopts.sound_device ?
		globalopts.sound_device : NULL)))
	{
		debug("start_song: openfile failed\n");
		//TODO: add warning here (geterrorcode())
		warning("Couldn't open file.");
		stop_song();
		delete[] song;
		return 0;
	}
		
	if (globalopts.downsample)
		(playopts.player)->setdownfrequency(1);
	if (globalopts.eightbits)
		(playopts.player)->set8bitmode();

	if ( !(playopts.player)->initialize(init_args) )
	{
		debug("start_song: player->initialize failed\n");
		stop_song();
		delete[] song;
		return 0;
	}

	playopts.song_played = 1;
	sp = chop_path(song);
	if (LOCK_NCURSES)
	{
		if (songinf.path)
			delete[] songinf.path;
		songinf.path = new char[strlen(sp)+1];
		strcpy(songinf.path, sp);
		songinf.songinfo = (playopts.player)->getsonginfo();
		if (globalopts.recode_table)
		{
			recode_string(songinf.songinfo.artist);
			recode_string(songinf.songinfo.songname);
			recode_string(songinf.songinfo.album);
			recode_string(songinf.songinfo.comment);
		}
		songinf.elapsed = 0;
		songinf.remain = songinf.songinfo.totaltime;
		songinf.status = PS_PLAY;
		songinf.update |= (1|2|4); //update time,songinfo,status
		UPDATE_CURSES;
		UNLOCK_NCURSES;
	}
	time(&(playopts.tyd));
	delete[] song;

	if (!one_mp3)
		update_playlist_history();
	else
		playopts.playing_one_mp3 = (was_playing ? 1 : 2);

	set_action(AC_NONE);

	return 1;
}

//Pre-condtion:
//  The global playmode and group modes must be set according to the wishes
//  of the user.
//  if playing_one_mp3 != NULL, then only that mp3 will be played, after which
//  the thread will exit.
void
play_one_mp3(const char *filename)
{
	set_one_mp3(filename);
	set_action(AC_ONE_MP3);
	lock_playing_mutex();
	wake_player();
	unlock_playing_mutex();
}

/* Function   : add_selected_file
 * Description: Used by recsel_files() to temporarily store files until
 *            : the recursive process has finished.
 * Parameters : file: file to add
 * Returns    : Nothing.
 * SideEffects: adds file to selected_files array.
 */
void
add_selected_file(const char *file)
{
	int
		offset;

	if (!file)
		return;

	selected_files = (char **)realloc(selected_files, (nselfiles + 1) *
		sizeof(char*) );
	offset = nselfiles;
	
	selected_files[offset] = NULL;
	selected_files[offset] = (char *)malloc((strlen(file) + 1) * sizeof(char));
	strcpy(selected_files[offset], file);
	++nselfiles;
}

/* Function   : add_selected_files
 * Description: Adds an array of files to the selected_files array.
 *            : It's much more efficient than repeatedly calling 
 *            : add_selected_file.
 * Parameters : files: array of char-pointers, each of which must be 
 *            :        allocated with malloc() or friends. Don't free them!
 *            : filecount: Number of pointers in 'files' array
 * Returns    : Nothing.
 * SideEffects: enlarges selected_files array & increases nselfiles
 */
void
add_selected_files(char **files, unsigned int fileCount)
{
	unsigned int i, offset;

	if (!files || !fileCount)
		return;
	
	selected_files = (char **)realloc(selected_files, (nselfiles + fileCount) *
		sizeof(char*) );
	offset = nselfiles;

	for (i = 0; i < fileCount; i++)
	{
		selected_files[i + offset] = files[i];
	}
	nselfiles += fileCount;
}

int
is_symlink(const char *path)
{
	struct stat
		*buf = (struct stat*)malloc( 1 * sizeof(struct stat) );
	int retval = 1;
	
	if (lstat(path, buf) < 0 || ((buf->st_mode) & S_IFMT) != S_IFLNK)
		retval = 0;

	/* forgot to free this. Thanks to Steve Kemp for pointing it out. */
	free(buf);

	return retval;
}

/* This function adds all mp3s recursively found in ``path'' to the current
 * group when called by F3: recursively add mp3's. 
 * When called by F5: Add dirs as groups (d2g), and d2g_init is not set,
 * all mp3's found in this dir will be added to a group with the name of
 * the dir. When d2f is set, 'files' and 'fileCount' should be NULL.
 * Only when d2g is NOT set:
 *   'files' must be a pointer to a char** array, in which all
 *   found files will be stored. The char** array will be allocated with 
 *   (re/m)alloc(), as will its members.
 */
void
recsel_files(const char *path, char ***files, unsigned int *fileCount, short d2g=0, int d2g_init=0)
{
	struct dirent
		*entry = NULL;
	DIR
		*dir = NULL;
	short 
		mp3_found = 0;
	mp3Win*
		d2g_groupindex = NULL;

	if (progmode != PM_FILESELECTION || !strcmp(path, "/dev") ||
		is_symlink(path))
		return;

	if ( !(dir = opendir(path)) )
		 return;

	char **dirfiles;
	char **dirs = NULL;
	int dirs_size = 0, dirfiles_size = 0;

	if (d2g) //don't add to files array
	{
		dirfiles = NULL;
		dirfiles_size = 0;
	}
	else //add to files array, so that total array can be sorted afterwards
	{
		dirfiles = *files;
		dirfiles_size = *fileCount;
	}


	/* first, find all files and chuck them in arrays with direntries and
	 * audiofile entries
	 */
	while ( (entry = readdir(dir)) )
	{
		DIR *dir2 = NULL;
		char *newpath = (char *)malloc((strlen(entry->d_name) + 2 + strlen(path)) *
			sizeof(char));

		PTH_YIELD;
		strcpy(newpath, path);
		if (path[strlen(path) - 1] != '/')
			strcat(newpath, "/");
		strcat(newpath, entry->d_name);

		if ( (dir2 = opendir(newpath)) )
		{
			int i = 0;
			short skip_dir = 0;
			const char *dummy = entry->d_name;
			const char *baddirs[] = {
				".", "..",
				".AppleDouble",
				".resources", //HPFS stuff
				".finderinfo",
				NULL,
			};

			closedir(dir2);

			while (baddirs[i])
			{
				if (!strcmp(dummy, baddirs[i++]))
				{
					skip_dir = 1;
					break;
				}
			}

			if (skip_dir)
			{
				free(newpath);
				continue;
			}

			dirs = (char**)realloc(dirs, ++dirs_size * sizeof(char*));
			dirs[dirs_size - 1] = newpath;
			continue;
		}
		else if (is_audiofile(newpath))
		{
			dirfiles = (char**)realloc(dirfiles, ++dirfiles_size * sizeof(char*));
			dirfiles[dirfiles_size -1] = newpath;
			continue;
		}
		else
			free(newpath);
	}
	closedir(dir);

	if (dirs_size)
		sort_files(dirs, dirs_size);

	PTH_YIELD;

	if (d2g && dirfiles_size)
		sort_files(dirfiles, dirfiles_size);
	if (!d2g)
	{
		*files = dirfiles;
		*fileCount = dirfiles_size;
	}

	int i;

	for (i = 0; i < dirs_size; i++)
	{
		PTH_YIELD;
		recsel_files(dirs[i], files, fileCount, d2g);
		free(dirs[i]);
	}
	free(dirs);
	dirs = NULL;

	if (!d2g)
		return;

	/* rest of code is only interesting for 'Add Dirs As Groups' */
	for (i = 0; i < dirfiles_size; i++)
	{
		PTH_YIELD;

		/* Add Dirs As Groups (not first call to recsel_files) ? */
		if (!d2g_init)
		{
			/* create new group if no mp3's have been found for this dir. */
			if (!mp3_found)
			{
				char const *npath = 0;
				npath = strrchr(path, '/');
				if (!npath)
					npath = path;
				else
				{
					if (strlen(npath) > 1)
						npath = npath + 1;
					else
						npath = path;
				}
				char rnpath[strlen(npath)+3];
				sprintf(rnpath, "[%s]", npath);
				d2g_groupindex = newgroup();
				d2g_groupindex->setTitle(rnpath);
				mp3_curwin->addGroup(d2g_groupindex, rnpath);
				/* If there are no entries in the current group,
				 * this group will be filled instead of skipping it
				 */
				mp3_found = 1;
			}
			if (d2g_groupindex)
			{
				const char *bla[4];
				short foo[2] = { 0, 1 };
				int idx;
				bla[0] = (const char*)dirfiles[i];
				bla[1] = chop_path(dirfiles[i]);
				bla[2] = NULL;
				bla[3] = NULL;
				idx=d2g_groupindex->addItem(bla, foo,
					CP_FILE_MP3);
				if (idx!=-1)
				{
					d2g_groupindex->changeItem(idx,&id3_filename,strdup(bla[0]),2);
				}
			}
			else
			{
				mw_settxt("Something's screwy with recsel_files");
				refresh_screen();
			}
		}

		free(dirfiles[i]);
	}
	free(dirfiles);
}

#ifdef PTHREADEDMPEG
void
change_threads()
{
	if ( (globalopts.threads += 50) > 500)
		globalopts.threads = 0;
	cw_draw_threads();
}

/* draws threads. Does not refresh screen. */
void
cw_draw_threads(short cleanit)
{
	int y, x;
	getmaxyx(stdscr, y,x);
	move(9, x - 4);
	if (!cleanit)
		printw("%03d", globalopts.threads);
	else
		addstr("   ");
}
#endif

/* returns a malloc()'d path */
char *
get_current_working_path()
{
	char
		*tmppwd = (char *)malloc(65 * sizeof(char));
	int
		size = 65;

	while ( !(getcwd(tmppwd, size - 1)) )
	{
		size += 64;
		tmppwd = (char *)realloc(tmppwd, size * sizeof(char));
	}

	tmppwd = (char *)realloc(tmppwd, strlen(tmppwd) + 2);
	
	if (strcmp(tmppwd, "/")) /* pwd != "/" */
		strcat(tmppwd, "/");
	
	return tmppwd;
}

void
clean_popup()
{
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	bkgdset(COLOR_PAIR(CP_DEFAULT));
	popup = 0;
	mw_settxt("");
	refresh_screen();
}

/* Return the command corresponding to the given keycode */
command_t
get_command(int kcode, unsigned short pm)
{
	int i = 0;
	command_t cmd = CMD_NONE;
	struct keybind_t k = keys[i];
	
	while (k.cmd != CMD_NONE)
	{
		if (k.key == kcode && (pm & k.pm))
		{
			cmd = k.cmd;
			break;
		}
		k = keys[++i];
	}
	return cmd;
}

/* handle_input reads input.
 * If no_delay = 0, then this function will block until input arrives.
 * Returns -1 when the program should finish, 0 or greater when everything's
 * okay.
 */
int
handle_input(short no_delay)
{
	int
		key = 0,
		retval = 0;
	fd_set fdsr;
	struct timeval tv;
	command_t cmd;
	mp3Win 
		*sw = mp3_curwin;
	fileManager
		*fm = file_window;

	//get a pointer to the main window, which depends on program mode.
	scrollWin *mainwindow = getMainWindow();

#ifdef LIBPTH
	pth_yield(NULL);
#endif

	if (no_delay)
	{
		fflush(stdin);
		FD_ZERO(&fdsr);
		FD_SET(0, &fdsr);  /* stdin file descriptor */
		tv.tv_sec = tv.tv_usec = 0;
#ifdef INCLUDE_LIRC
		if (lirc_cmd == CMD_NONE) {
			if (select(FD_SETSIZE, &fdsr, NULL, NULL, &tv) == 1) 
				key = getch();
			else
			{
				key = ERR;
			}
		}
#else
		if (select(FD_SETSIZE, &fdsr, NULL, NULL, &tv) == 1) 
			key = getch();
		else
		{
			key = ERR;
		}
#endif
	}
	else
	{
		debug("THREAD main waiting for blocking input.\n");
		nodelay(stdscr, FALSE);
#ifdef INCLUDE_LIRC
		if (lirc_cmd == CMD_NONE) key = getch();
#else
		key = getch();
#endif 
		debug("THREAD main woke up!\n");
	}

	if (LOCK_NCURSES);

	if (songinf.update)
	{
		update_status_file();
		update_ncurses(0);
	}
	
	UNLOCK_NCURSES;

	if (no_delay && key == ERR)
	{
#ifdef LIBPTH
		pth_yield(NULL);
		USLEEP(10000);
#else
		USLEEP(1000);
#endif

		return 0;
	}

	if (key == KEY_RESIZE) {
		handle_resize(0);
		return retval;
	}

#ifdef INCLUDE_LIRC
	if (lirc_cmd != CMD_NONE)
	{
		cmd = lirc_cmd;
		lirc_cmd = CMD_NONE;
	}
	else
	{
		cmd = get_command(key, progmode);
	}
#else
	cmd = get_command(key, progmode);
#endif

	if (input_mode == IM_SEARCH)
	{
		/* the user is searching for something */
		if (
			(key >= 'a' && key <= 'z') ||
			(key >= 'A' && key <= 'Z') ||
			(key >= '0' && key <= '9') ||
			strchr(" (){}[]<>,.?;:\"'=+-_!@#$%^&*", key)
		)
		{
			fw_search_next_char(key);
		}
		else if (key == 13)
		{
			fw_end_search();	
			input_mode = IM_DEFAULT;
		}
		retval = 1;
	}
	else if (input_mode == IM_INPUT)
	{
		/* The user is entering text into a text input field. */
		//TODO: make keys configurable.
		switch(key)
		{
		case KEY_LEFT:
			global_input->moveLeft(1);
			break;
		case KEY_RIGHT:
			global_input->moveRight(1);
			break;
		case KEY_UP:
		case KEY_DOWN:
		{
			short insmode = global_input->setInsertMode(global_input->getInsertMode() ^ 1);
			//insert_mode = 1;
			//TODO: This should be put in a more appriopriate place.
			mw_settxt((insmode ? "--  INSERT --" : "-- REPLACE --"));
			global_input->updateCursor();
			break;
		}
		case KEY_BACKSPACE:
			global_input->deleteChar(); 
			break;
		case KEY_DC:
			global_input->deleteChar(0);
			break;
		case KEY_HOME: global_input->moveToBegin(); break;
		case KEY_END: global_input->moveToEnd(); break;
		case 13: 
		{
			void (*callback_fun)(const char *, void*);
			//global_input->getCallbackFunction(&callback_fun);
			clean_popup();
			set_inout_opts();
			callback_fun = global_input->getCallbackFunction();
			callback_fun(global_input->getString(), global_input->getCallbackData());
			delete global_input;
			global_input = NULL;
			input_mode = IM_DEFAULT;
			mainwindow->swRefresh(1);
		}
		break;
		default:
			global_input->addChar(key);
		}

		if (global_input)
		{
			global_input->updateCursor();
		}
		//set_inout_opts();
		retval = 1;
	}

	if (retval)
	{
		return retval;
	}

	switch(cmd)
	{
		case CMD_PLAY_PLAY: set_action(AC_PLAY); wake_player(); break;
		case CMD_PLAY_NEXT: set_action(AC_NEXT); break;
		case CMD_PLAY_PREVIOUS: set_action(AC_PREV); break;
		case CMD_PLAY_FORWARD: set_action(AC_FORWARD); break;
		case CMD_PLAY_REWIND: set_action(AC_REWIND); break;
		//case CMD_PLAY_STOP: set_action(AC_STOP); break;
		case CMD_PLAY_STOP: set_action(AC_STOP_LIST); break;
		case CMD_PLAY_SKIPEND: set_action(AC_SKIPEND); break;
		case CMD_PLAY_NEXTGROUP: set_action(AC_NEXTGRP); break;
		case CMD_PLAY_PREVGROUP: set_action(AC_PREVGRP); break;
		//case 'i': sw->setDisplayMode(0); sw->swRefresh(2); break;
		//case 'I': sw->setDisplayMode(1); sw->swRefresh(2); break;
		case CMD_MOVE_AFTER:
			sw->moveSelectedItems(sw->sw_selection);
			sw->swRefresh(1);
		break;
		case CMD_MOVE_BEFORE:
			sw->moveSelectedItems(sw->sw_selection, 1);
			sw->swRefresh(1);
		break;
		case CMD_QUIT_PROGRAM: //quit mp3blaster
			retval = -1;
		break;
		case CMD_HELP:
			if (progmode == PM_HELP)
			{
				end_help();
				change_program_mode(PM_NORMAL);
			}
			else
			{
				change_program_mode(PM_HELP);
				show_help();
			}
		break;
		case CMD_DOWN:
			if (progmode == PM_NORMAL)
				sw->changeSelection(1);
			else if (progmode == PM_FILESELECTION)
				fm->changeSelection(1);
			else if (progmode == PM_HELP)
				bighelpwin->changeSelection(1);
		break;
		case CMD_UP:
			if (progmode == PM_NORMAL)
				sw->changeSelection(-1);
			else if (progmode == PM_FILESELECTION)
				fm->changeSelection(-1);
			else if (progmode == PM_HELP)
				bighelpwin->changeSelection(-1);
		break;
		case CMD_DEL: 
		case CMD_DEL_MARK:
		{
			mp3Win *tmpgroup;

			if (progmode != PM_NORMAL && cmd == CMD_DEL_MARK)
				break;

			if (sw->isPreviousGroup(sw->sw_selection))
				break;

			lock_playing_mutex();

			//don't delete a group that's playing right now (or contains a group
			//that's being played)
			if (sw->isGroup(sw->sw_selection) && (tmpgroup =
				sw->getGroup(sw->sw_selection)) && tmpgroup->isPlaying())
			{
				if (playopts.playing)
				{
					warning("Can't delete an active group.");
					unlock_playing_mutex();
			//don't delete a group that's playing right now (or contains a group
					break;
				}
				reset_playlist(1);
				mp3_groupwin = NULL;
			}

			/* Also mark file as bad? */
			if (cmd == CMD_DEL_MARK && is_audiofile(sw->getSelectedItem()))
				fw_markfilebad(sw->getSelectedItem());

			sw->delItem(sw->sw_selection);	
			reset_next_song();
			unlock_playing_mutex();
		}
		break;
		case CMD_NEXT_PAGE:
			if (progmode == PM_NORMAL)
				sw->pageDown();
			else if (progmode == PM_FILESELECTION)
				fm->pageDown();
			else if (progmode == PM_HELP)
				bighelpwin->pageDown();
		break;
		case CMD_PREV_PAGE:
			if (progmode == PM_NORMAL)
				sw->pageUp();
			else if (progmode == PM_FILESELECTION)
				fm->pageUp();
			else if (progmode == PM_HELP)
				bighelpwin->pageUp();
		break;
		case CMD_SELECT: 
			if (!sw->isGroup(sw->sw_selection))
			{
				sw->selectItem();
				sw->changeSelection(1);
			}
		break;
		case CMD_SELECT_ITEMS:
		{
			const char *label[] = {
				"Enter search pattern",
				NULL,
			};

			popup_win(label, select_files_by_pattern, (void*)mainwindow, NULL, 1, 256);
			break;
		}
		case CMD_DESELECT_ITEMS:
			mainwindow->deselectItems();
			mainwindow->swRefresh(0);
		break;
		case CMD_REFRESH: refresh_screen(); break; // C-l
		case CMD_ENTER: // play 1 mp3 or dive into a subgroup.
			if (sw->isGroup(sw->sw_selection))
			{
				/* Copy display mode */
				mp3_curwin = sw->getGroup(sw->sw_selection);
				mp3_curwin->setDisplayMode(sw->getDisplayMode());
				mp3_curwin->resetPan();
				mp3_curwin->swRefresh(2);
				cw_draw_group_mode();
				refresh();
			}
			else if (sw->getNitems() > 0)
				play_one_mp3(sw->getSelectedItem());
		break;
		case CMD_TOGGLE_DISPLAY:
			if (progmode == PM_NORMAL)
			{
				short dispmode = sw->getDisplayMode() + 1;
				if (dispmode > 2)
					dispmode = 0;
				sw->setDisplayMode(dispmode);
				sw->swRefresh(2); 
			}
			else if (progmode == PM_FILESELECTION)
			{
				fw_toggle_display();
				fm->swRefresh(2);
			}
		break;
		case CMD_SELECT_FILES: /* file-selection */
			fw_begin();
		break;
		case CMD_ADD_GROUP: quit_after_playlist = 0; add_group(); break; //add group
		//TODO: rewrite read/write_playlist
		case CMD_WRITE_PLAYLIST:
		{
			const char
				*label[] = {
				"Playlist will be saved in: ",
				globalopts.playlist_dir,
				"(Configuration keyword to set default dir: PlaylistDir)",
				"Example filename: myplaylist.lst",
				NULL
			};

			popup_win(label, playlist_write, NULL, NULL, 1, 40);
		}
		break;
		case CMD_LOAD_PLAYLIST:
			if (progmode == PM_NORMAL)
				fw_begin();
			if (globalopts.playlist_dir)
			{
				char *tmpStr = expand_path(globalopts.playlist_dir);
				if (tmpStr)
				{
					fw_changedir(tmpStr);
					free(tmpStr);
				}
				else
					warning("Couldn't load playlist (wrong path?)");
			}
		break;
		//	read_playlist((const char*)NULL); break; // read playlist
		//	write_playlist(); break; // write playlist
		case CMD_SET_GROUP_TITLE:
			if (sw->isGroup(sw->sw_selection) &&
				!sw->isPreviousGroup(sw->sw_selection))
			{
				const char *label[] = { "Enter Name:", NULL };
				popup_win(label,set_group_name, NULL, NULL, 1,48);
			}
		break; // change groupname
		case CMD_TOGGLE_REPEAT: //toggle repeat
			//TODO: repeat's not thread-safe..low priority
			globalopts.repeat = 1 - globalopts.repeat;
			if (globalopts.repeat)
				quit_after_playlist = 0;
			cw_draw_repeat();
			refresh();
		break;
		case CMD_TOGGLE_SHUFFLE: cw_toggle_group_mode(); reset_next_song(); break; 
		case CMD_TOGGLE_PLAYMODE: 
			lock_playing_mutex();
			if (!playopts.playing)
			{
				//TODO: is this safe? What if the users STOPS a song (!playing), and
				//later on continues the list? playing_group might be a dangling
				//pointer then...or not, because deleting a group that is being played
				//is caught in CMD_DEL. Or was there something else that could mess
				//up...have to check into this.
				cw_toggle_play_mode();
				reset_next_song();
				unlock_playing_mutex();
			}
			else
			{
				debug("Playing, can't toggle playmode\n");
				//TODO: FIX WARNING to not use sleep() in a threaded env.
				warning("Not possible during playback");
				unlock_playing_mutex();
				refresh_screen();
			}
		break;
		case CMD_FILE_SELECT: fm->selectItem(); fm->changeSelection(1); break;
		case CMD_FILE_UP_DIR: fw_changedir(".."); break;
		case CMD_FILE_START_SEARCH: fw_start_search(); break;
		case CMD_FILE_TOGGLE_SORT: fw_toggle_sort(); fw_changedir("."); break;
		case CMD_FILE_MARK_BAD:
		{
			char *fullpath = NULL;
			const char *halfpath = NULL;
			char **selitems = NULL;
			int itemcnt = 0, i = 0, moved_items_count = 0;

			if (progmode != PM_FILESELECTION)
				break;

			debug("Mark file(s) as bad\n");

			selitems = fm->getSelectedItems(&itemcnt);
			if (!itemcnt)
			{
				selitems = new char*[1];
				selitems[0] = new char[strlen(fm->getSelectedItem())+1];
				strcpy(selitems[0], fm->getSelectedItem());
				itemcnt = 1;
				debug("Used highlighted file to mark as bad.\n");
			}
			halfpath = fm->getPath();

			for (i = 0; i < itemcnt; i++)
			{
				fullpath = new char[strlen(halfpath) + strlen(selitems[i]) + 1];
				strcpy(fullpath, halfpath);
				strcat(fullpath, selitems[i]);
				debug(fullpath);
				if (is_audiofile(fullpath))
				{
					if (fw_markfilebad((const char *)fullpath))
						moved_items_count++;
				}
				delete[] fullpath;
				delete[] selitems[i];
				fullpath = NULL;
			}
			delete[] selitems;
			if (moved_items_count)
				fw_changedir(".");
		}
		case CMD_FILE_DELETE:
			fw_delete();
		break;
		case CMD_FILE_ENTER: /* change into dir, play soundfile, load playlist, 
		                      * end search */
			if (fm->isDir(fm->sw_selection))
				fw_changedir();
			else if (is_playlist(fm->getSelectedItem()))
			{
				char
					bla[strlen(fm->getPath()) + strlen(fm->getSelectedItem()) + 2];

				sprintf(bla, "%s/%s", fm->getPath(), fm->getSelectedItem());
				fw_end();
				read_playlist(bla);
				mp3_curwin->swRefresh(2);
				break;
			}
			else if (fm->getNitems() > 0)
				play_one_mp3(fm->getSelectedItem());
		break;
		case CMD_FILE_ADD_FILES: fw_end(); break;
		case CMD_FILE_INV_SELECTION: fm->invertSelection(); break;
		case CMD_FILE_RECURSIVE_SELECT:
		{
			char
				*tmppwd = get_current_working_path(),
				**files = NULL;
			unsigned int fileCount = 0;

			mw_settxt("Recursively selecting files..");
			recsel_files(tmppwd, &files, &fileCount); //TODO: move this to fileManager class?
			free(tmppwd);
			if (fileCount)
			{
				sort_files(files, fileCount);
				add_selected_files(files, fileCount); //copies allocated pointers
				free(files);
			}
			fw_end();
			mw_settxt("Added all mp3's in this dir and all subdirs " \
				"to current group.");
		}
		break;
		case CMD_FILE_SET_PATH:
		{
			const char *label[] = {
				"Enter path below",
				NULL,
			};
			
			popup_win(label, fw_getpath, NULL, NULL, 1, 1024);
			break;
		}
		case CMD_FILE_DIRS_AS_GROUPS:
		{
			char
				*tmppwd = get_current_working_path();
			
			mw_settxt("Adding groups..");
			recsel_files(tmppwd, NULL, NULL, 1, 1);
			free(tmppwd);
			fw_end();
			mw_settxt("Added dirs as groups.");
		}
		break;
		case CMD_FILE_MP3_TO_WAV: /* Convert mp3 to wav */
		{
			const char *label[] = { "Dir to put wavefiles:", startup_path, NULL };
			popup_win(label, fw_convmp3, NULL, NULL, 1, 1024);
		}
		break;
		case CMD_FILE_ADD_URL: /* Add HTTP url to play */
		{
			const char *label[] = { "URL:", NULL };
			popup_win(label, fw_addurl, NULL, NULL, 1, 2048);
		}
		break;
		case CMD_HELP_PREV: helpwin->pageUp(); repaint_help(); break;
		case CMD_HELP_NEXT: helpwin->pageDown(); repaint_help(); break;
#ifdef PTHREADEDMPEG
		case CMD_CHANGE_THREAD: change_threads(); break;
#endif
		case CMD_CLEAR_PLAYLIST:
			lock_playing_mutex();
			if (!playopts.playing)
			{
				mp3_rootwin->delItems();
				mp3_curwin = mp3_rootwin;
				mp3_rootwin->swRefresh(2);
			}
			unlock_playing_mutex();
		break;
		case CMD_FILE_RENAME:
		{
			const char *label[] = { "Filename:", NULL };

			if (!fm->isDir(fm->sw_selection) &&
				fm->getSelectedItem(0))
			{
				popup_win(label, fw_rename_file, NULL, 
					chop_path(fm->getSelectedItem(0)), 1, 2048);
			}
		}
		break;
		case CMD_LEFT:
			if (progmode == PM_NORMAL)
				sw->pan(-globalopts.pan_size);
			else if (progmode == PM_FILESELECTION)
				fm->pan(-globalopts.pan_size);
		break;
		case CMD_RIGHT:
			if (progmode == PM_NORMAL)
				sw->pan(globalopts.pan_size);
			else if (progmode == PM_FILESELECTION)
				fm->pan(globalopts.pan_size);
		break;
		case CMD_TOGGLE_WRAP:
			globalopts.wraplist = !globalopts.wraplist;
		break;
		case CMD_JUMP_TOP:
			if (progmode == PM_NORMAL)
				sw->jumpTop();
			else if (progmode == PM_FILESELECTION)
				fm->jumpTop();
			else if (progmode == PM_HELP)
				bighelpwin->jumpTop();
		break;
		case CMD_JUMP_BOT:
			if (progmode == PM_NORMAL)
				sw->jumpBottom();
			else if (progmode == PM_FILESELECTION)
				fm->jumpBottom();
			else if (progmode == PM_HELP)
				bighelpwin->jumpBottom();
		break;
		case CMD_NONE: break;
		default:
			if (mixer)
				mixer->ProcessKey(key);
	}
	
	return retval;
}

/* Function   : show_help
 * Description: Displays a help screen in the main window. "progmode" should
 *            : be in PM_HELP.
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
show_help()
{
	if (!bighelpwin)
	{
		int height, width, y, x;
		chtype b1, b2, b3, b4, b5, b6, b7, b8;
		char **lines = NULL;
		char *configfile = NULL;
		int linecount = 0;
		//short h=(COLS/2)+1;

		get_mainwin_size(&height, &width, &y, &x);
		get_mainwin_borderchars(&b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8);

		configfile = new char[strlen(MP3BLASTER_DOCDIR) + 20];
		sprintf(configfile, "%s/%s", MP3BLASTER_DOCDIR, "commands.txt");
		if (read_file(configfile, &lines, &linecount))
		{
			int i;

			bighelpwin = new scrollWin(height, width, y, x, NULL, 0,
				CP_DEFAULT, 1);
			bighelpwin->setWrap(globalopts.wraplist);
			for (i = 0; i < linecount; i++)
			{
				char *newline = strrchr(lines[i], '\n');
				if (newline)
					newline[0] = '\0';

				bighelpwin->addItem(lines[i]);
				free(lines[i]);
			}
			free(lines);
		}
		else
		{
			bighelpwin = new scrollWin(height, width, y, x, lines, linecount,
				CP_DEFAULT, 1);
			bighelpwin->setWrap(globalopts.wraplist);
			bighelpwin->addItem("Couldn't read mp3blaster helpfile:");
			bighelpwin->addItem(configfile);
		}
		bighelpwin->setBorder(b1, b2, b3, b4, b5, b6, b7, b8);

		delete[] configfile;
	}

	mw_settxt("Press '?' to exit help screen.");
	bighelpwin->swRefresh(2);
}

void
end_help()
{
	//simpy 'hide' bighelpwin..
	mp3_curwin->swRefresh(2);
	mw_settxt("Visit http://mp3blaster.sourceforge.net/");
}

void
cw_draw_filesort(short cleanit)
{
	if (!cleanit)
	{
		if (progmode == PM_FILESELECTION)
			fw_draw_sortmode(globalopts.fw_sortingmode);
	}
	else
	{
		//clear 'Sorting mode' line.
		move(6, 2);
		int stextlen = (COLS > 16 ? COLS - 16 : 1);
		char emptyline[stextlen + 1];
		memset(emptyline, ' ', stextlen * sizeof(char));
		emptyline[stextlen] = '\0';
		addstr(emptyline);
	}
}

void
draw_settings(int cleanit)
{
	cw_draw_group_mode();
	if (progmode == PM_FILESELECTION)
		cw_draw_filesort(cleanit);
	else if (progmode == PM_NORMAL)
		cw_draw_play_mode(cleanit);
	cw_draw_repeat();
#ifdef PTHREADEDMPEG
		cw_draw_threads(cleanit);
#endif
	refresh();
}

void
fw_convmp3(const char *tmp, void *args)
{
	char **selitems;
	int nselected;
	char *dir2write;

	if (args);

	if (tmp && !strlen(tmp))
	{
		const char *tmp2 = file_window->getPath();
		dir2write = strdup(tmp2);
	}
	else if (tmp)
		dir2write = expand_path(tmp);
	else
	{
		warning("Invalid dir entered.");
		refresh();
		return;
	}

	if (!dir2write || !is_dir(dir2write))
	{
		warning("Invalid path entered.");
		refresh();
		free(dir2write);
		return;
	}

	if  (progmode != PM_FILESELECTION)
	{
		free(dir2write);
		return;
	}

	selitems = file_window->getSelectedItems(&nselected);

	if (!nselected && file_window->getSelectedItem())
	{
		selitems = new char*[1];
		selitems[0] = new char[strlen(file_window->getSelectedItem()) + 1];
		strcpy(selitems[0], file_window->getSelectedItem());
		nselected = 1;
	}
	else if (!nselected)
	{
		warning("No item[s] selected.");
		refresh();
		free(dir2write);
		return;
	}

	for (int i = 0; i < nselected; i++)
	{
		const char
			*pwd = file_window->getPath();
		char
			*file = new char[(strlen(pwd) + strlen(selitems[i]) +
				2) * sizeof(char)],
			*file2write = new char[(strlen(dir2write) + strlen(selitems[i]) +
				6)];
		int len;

		strcpy(file, pwd);
		if (pwd[strlen(pwd) - 1] != '/')
			strcat(file, "/");
		strcat(file, selitems[i]);

		strcpy(file2write, dir2write);
		if (pwd[strlen(dir2write) - 1] != '/')
			strcat(file2write, "/");
		strcat(file2write, selitems[i]);
		if ((len = strlen(file2write)) > 4 && !fnmatch(".[mm][pP][23]",
			(file2write + (len - 4)), 0))
			bcopy(".wav", (void*)(file2write + (len - 4)), 4);
		else
			strcat(file2write, ".wav");

		if (is_audiofile(file))
		{
			char bla[strlen(file)+80];
			Mpegfileplayer *decoder = NULL;

			if (!(decoder = new Mpegfileplayer(Fileplayer::AUDIODRV_OSS)) ||
				!decoder->openfile(file,
				file2write, WAV) || !decoder->initialize(NULL))
			{
				sprintf(bla, "Decoding of %s failed.", selitems[i]);
				warning(bla);
				refresh();
				if (decoder)
					delete decoder;
			}
			else
			{
				sprintf(bla, "Converting '%s' to wavefile, please wait.", 
					selitems[i]);
				mw_settxt(bla);
				while (decoder->run(10));
				int decode_error = decoder->geterrorcode();
				decoder->stop();
				if (decode_error != SOUND_ERROR_OK && 
					decode_error != SOUND_ERROR_FINISH)
				{
					warning("Decode error: %s", get_error_string(decode_error));
				}
				else
					mw_settxt("Conversion(s) finished without errors.");
				delete decoder;
			}
		}
		else
		{
			warning("Skipping non-audio file");
			refresh();
		}
		delete[] selitems[i];
		delete[] file;
		delete[] file2write;
	}
	if (selitems)
		delete[] selitems;
	free(dir2write);
}

void
fw_addurl(const char *urlname, void *args)
{
	mp3Win
		*sw;

	if (args);

	if (!urlname || progmode != PM_FILESELECTION)
	{
		return;
	}

	sw = mp3_curwin;

	add_selected_file(urlname);
}

void
fw_set_search_timeout(int timeout=2)
{
	struct itimerval tijd, dummy;
	tijd.it_value.tv_sec = tijd.it_interval.tv_sec = timeout;
	tijd.it_value.tv_usec = tijd.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &tijd, &dummy);
}

void
fw_end_search()
{
	if (fw_searchstring)
		delete[] fw_searchstring;

	fw_searchstring = NULL;
	//signal(SIGALRM, SIG_IGN);
	mw_settxt("");
}

/* called when sigalarm is triggered (i.e. when, in searchmode in filemanager,
 * a key has not been pressed for a few seconds (default: 2). This will
 * disable the searchmode.
 */
void
fw_search_timeout(int blub)
{
	if (blub);	//no warning this way ;)
	if (input_mode != IM_SEARCH)
		return;

	fw_end_search();
	input_mode = IM_DEFAULT;
}

/* called when someone presses [a-zA-Z0-9] in searchmode in filemanager
 * or in playlist
 */
void
fw_search_next_char(char nxt)
{
	char *tmp;
	short foundmatch = 0;
	scrollWin *sw = file_window;
	if (progmode == PM_NORMAL)
		sw = mp3_curwin;
	if (!fw_searchstring)
	{
		tmp = new char[2];
		tmp[0] = nxt;
		tmp[1] = '\0';
	}
	else
	{
		tmp = new char[strlen(fw_searchstring)+2];
		strcpy(tmp, fw_searchstring);
		strncat(tmp, &nxt, 1);
	}
	for (int i = 0; i < sw->getNitems(); i++)
	{
		const char *item = sw->getItem(i);
		if (progmode == PM_NORMAL)
			item = chop_path(item);
		if (!strncmp(item, tmp, strlen(tmp)))
		{
			sw->setItem(i);
			foundmatch = 1;
			break;
		}
	}
	fw_set_search_timeout(2);
	if (!foundmatch)
		beep();
	else
	{
		if (fw_searchstring)
			delete[] fw_searchstring;
		fw_searchstring = new char[strlen(tmp) + 1];
		strcpy(fw_searchstring, tmp);
		char bla[strlen(fw_searchstring)+20];
		sprintf(bla, "Search: %s", fw_searchstring);
		mw_settxt(bla);
	}
}

/* called when someone presses '/' in filemanager */
void
fw_start_search(int timeout)
{
	signal(SIGALRM, &fw_search_timeout);
	fw_set_search_timeout(timeout);
	mw_settxt("Search:");
	input_mode = IM_SEARCH;
}

void
set_sound_device(const char *devname)
{
	if (!devname)
		return;

	if (globalopts.sound_device)
		delete[] globalopts.sound_device;

	globalopts.sound_device = new char[strlen(devname)+1];
	strcpy(globalopts.sound_device, devname);
}

void
set_mixer_device(const char *devname)
{
	if (!devname)
		return;

	if (globalopts.mixer_device)
		delete[] globalopts.mixer_device;

	globalopts.mixer_device = new char[strlen(devname)+1];
	strcpy(globalopts.mixer_device, devname);
}


short
set_fpl(int fpl)
{
	if (fpl < 1 || fpl > 10)
		return 0;
	globalopts.fpl = fpl;
	return 1;
}

#ifdef PTHREADEDMPEG
short
set_threads(int t)
{
	if (t < 0 || t > 500 || t%50 != 0)
		return 0;
	else
		globalopts.threads = t;
	//t = 0; //prevent compile warning
	return 1;
}
#endif

short
set_audiofile_matching(const char **matches, int nmatches)
{
	if (!matches || nmatches < 1)
		return 0;
	if (globalopts.extensions)
	{
		int i=0;

		while (globalopts.extensions[i])
		{
			delete[] globalopts.extensions[i];
			i++;
		}
		delete[] globalopts.extensions;
	}

	globalopts.extensions = new char*[nmatches+1];
	for (int i=0; i < nmatches; i++)
	{
		globalopts.extensions[i] = new char[strlen(matches[i])+1];
		strcpy(globalopts.extensions[i], matches[i]);
	}
	globalopts.extensions[nmatches] = NULL;

	return 1;
}

short
set_playlist_matching(const char **matches, int nmatches)
{
	if (!matches || nmatches < 1)
		return 0;
	if (globalopts.plist_exts)
	{
		int i=0;

		while (globalopts.plist_exts[i])
		{
			delete[] globalopts.plist_exts[i];
			i++;
		}
		delete[] globalopts.plist_exts;
	}

	globalopts.plist_exts = new char*[nmatches+1];
	for (int i=0; i < nmatches; i++)
	{
		globalopts.plist_exts[i] = new char[strlen(matches[i])+1];
		strcpy(globalopts.plist_exts[i], matches[i]);
	}
	globalopts.plist_exts[nmatches] = NULL;

	return 1;
}

short
set_warn_delay(unsigned int seconds)
{
	if (seconds > 60)
		return 0;
	
	globalopts.warndelay = seconds;
	return 1;
}

short
set_skip_frames(unsigned int frames)
{
	if (frames > 1000 || frames < 1) /* get real */
		return 0;
	
	globalopts.skipframes = frames;
	return 1;
}	

void
set_default_colours()
{
	globalopts.colours.default_fg = COLOR_WHITE;
	globalopts.colours.default_bg = COLOR_BLACK;
	globalopts.colours.popup_fg = COLOR_WHITE;
	globalopts.colours.popup_bg = COLOR_BLUE;
	globalopts.colours.popup_input_fg = COLOR_WHITE;
	globalopts.colours.popup_input_bg = COLOR_MAGENTA;
	globalopts.colours.error_fg = COLOR_WHITE;
	globalopts.colours.error_bg = COLOR_RED;
	globalopts.colours.button_fg = COLOR_MAGENTA;
	globalopts.colours.button_bg = COLOR_BLACK;
	globalopts.colours.shortcut_fg = COLOR_MAGENTA;
	globalopts.colours.shortcut_bg = COLOR_BLUE;
	globalopts.colours.label_fg = COLOR_BLACK;
	globalopts.colours.label_bg = COLOR_YELLOW;
	globalopts.colours.number_fg = COLOR_YELLOW;
	globalopts.colours.number_bg = COLOR_BLACK;
	globalopts.colours.file_mp3_fg = COLOR_GREEN;
	globalopts.colours.file_dir_fg = COLOR_BLUE;
	globalopts.colours.file_lst_fg = COLOR_YELLOW;
	globalopts.colours.file_win_fg = COLOR_MAGENTA;
}

mp3Win *
newgroup()
{
	mp3Win *tmp;

	tmp = new mp3Win(calc_mainwindow_height(LINES), 
		calc_mainwindow_width(COLS), 10, 0, NULL, 0, CP_DEFAULT, 1);

	tmp->drawTitleInBorder(1);
	tmp->setBorder(ACS_VLINE,ACS_VLINE,ACS_HLINE,ACS_HLINE,
		ACS_LTEE, ACS_PLUS, ACS_LTEE, ACS_PLUS);
	
	tmp->setDisplayMode(globalopts.display_mode);
	tmp->setWrap(globalopts.wraplist);
	return tmp;
}

void
set_action(_action bla)
{
	lock_playing_mutex();
	action = bla;
	if (bla == AC_STOP)
		quit_after_playlist = 0;
	unlock_playing_mutex();
}

/* determine_song returns a pointer to the song to play (or NULL if there is
 * none). delete[] it after use.
 * Argument set_played = 0 => don't set it to 'played' status!
 */
char*
determine_song(short set_played)
{
	const char *mysong = NULL;
	char *song;
	int total_songs;

	total_songs = mp3_rootwin->getUnplayedSongs();
	if (!total_songs)
		return NULL;
	
	switch(globalopts.play_mode)
	{
	case PLAY_GROUPS_RANDOMLY:
		return NULL;
	break;
	case PLAY_NONE:
	break;
	case PLAY_SONGS: //all songs in random order
		if (next_song > -1)	
			mysong = mp3_rootwin->getUnplayedSong(next_song, set_played);
		else
			mysong = mp3_rootwin->getUnplayedSong(myrand(total_songs), set_played);
		next_song = (total_songs - 1 ? myrand(total_songs - 1) : -1);
	break;

	case PLAY_GROUP: //current group:
	case PLAY_GROUPS:
	{
		if (globalopts.play_mode == PLAY_GROUPS && !mp3_groupwin)
		{
			mp3_groupwin = mp3_curwin;
			playing_group = NULL;
		}
			
		while (!playing_group || !playing_group->getUnplayedSongs())
		{
			if (playing_group) //this means that there are no unplayed songs in it
			{
				playing_group->setNotPlaying();
				playing_group = NULL;
				if (globalopts.play_mode == PLAY_GROUP)
					//finished playing group
					break;
			}

			if (globalopts.play_mode == PLAY_GROUPS)
			{
				if (!mp3_groupwin) //shouldn't happen...
				{
					reset_playlist(0);
					break;
				}
				
				//getUnplayedGroups counts the group itself as well if it contains
				//unplayed songs. Just so you know. I forgot this myself ;-)
				int 
					tg = mp3_groupwin->getUnplayedGroups();

				if (!tg) //no more groups left to play!
				{
					reset_playlist(0);
					//mp3_groupwin = NULL;
					break;
				}

				//determine which group to play.
				if (mp3_groupwin->getPlaymode())
					playing_group = mp3_groupwin->getUnplayedGroup(myrand(tg));
				else
					playing_group = mp3_groupwin->getUnplayedGroup(0);
					
				if (playing_group->getUnplayedSongs(0))
				{
					playing_group->setPlayed();
					playing_group->setPlaying();
				}
				else
				{
					playing_group = NULL;
					mp3_groupwin = NULL;
				}
			}
			else
			{
				playing_group = mp3_curwin;
				playing_group->setPlaying();
			}
		}

		if (!playing_group) //no valid group could be found.
			return NULL;

		if ((total_songs = playing_group->getUnplayedSongs(0)))
		{
			if (!playing_group->getPlaymode()) //normal playback
			{
				if (next_song > -1)
					mysong = playing_group->getUnplayedSong(next_song, set_played, 0);
				else
					mysong = playing_group->getUnplayedSong(0, set_played, 0);
				next_song = 0;
			}
			else //random playback
			{
				if (next_song > -1)
					mysong = playing_group->getUnplayedSong(next_song, set_played, 0);
				else
					mysong = playing_group->getUnplayedSong(myrand(total_songs), set_played, 0);
				if (total_songs - 1)
					next_song = myrand(total_songs - 1);
				else
					next_song = -1;
			}
		}
		else
			reset_playlist(1); //only reset playing group
	}
	break;
	}
	if (mysong)
	{
		if (next_song > -1)
			set_next_song(next_song);
		song = new char[strlen(mysong)+1];
		strcpy(song, mysong);
		return song;
	}
	return NULL;
}

void
update_status_file()
{
	if (status_file == NULL)
		return;
	if (songinf.update & (1 | 4 | 16 | 32))
	{
		// song info, song status, song status too (?), song change
		write_status_file();
	}
}

void
write_status_file()
{
	FILE *file;
	struct song_info &si = songinf.songinfo; // mpeg internal song info

	file = fopen(status_file, "w");
	if (file == NULL)
		return;
	if (songinf.path != NULL)
		fprintf(file, "path %s\n", songinf.path);
	switch (songinf.status)
	{
		case PS_PLAY: fprintf(file, "status playing\n"); break;
		case PS_PAUSE: fprintf(file, "status paused\n"); break;
		case PS_REWIND: fprintf(file, "status rewinding\n"); break;
		case PS_FORWARD: fprintf(file, "status fast forwarding\n"); break;
		case PS_PREV: fprintf(file, "status selecting previous song\n"); break;
		case PS_NEXT: fprintf(file, "status selecting next song\n"); break;
		case PS_STOP: fprintf(file, "status stopped\n"); break;
		case PS_RECORD: fprintf(file, "status recording\n"); break;
		case PS_NONE: break;
	}
	
	if (si.songname[0] != '\0')
		fprintf(file, "title %s\n", si.songname);
	if (si.artist[0] != '\0')
		fprintf(file, "artist %s\n", si.artist);
	if (si.album[0] != '\0')
		fprintf(file, "album %s\n", si.album);
	if (si.year[0] != '\0')
		fprintf(file, "year %s\n", si.year);
	if (si.comment[0] != '\0')
		fprintf(file, "comment %s\n", si.comment);
	if (si.mode[0] != '\0')
		fprintf(file, "mode %s\n", si.mode);
	fprintf(file, "format MPEG %d layer %d\n", si.mp3_version + 1,
			si.mp3_layer);
	fprintf(file, "bitrate %d\n", si.bitrate);
	fprintf(file, "samplerate %d\n", si.samplerate);
	fprintf(file, "length %d\n", si.totaltime);
	if (songinf.next_song)
		fprintf(file, "next %s\n", songinf.next_song);
	fclose(file);
}

void
set_next_song(int next_song)
{
	const char *ns = NULL;
	
	switch(globalopts.play_mode)
	{
	case PLAY_SONGS:
		ns = mp3_rootwin->getUnplayedSong(next_song, 0); break;
	case PLAY_GROUP:
	case PLAY_GROUPS:
		if (!playing_group)
			break;
		ns = playing_group->getUnplayedSong(next_song, 0, 0);
		if (globalopts.play_mode == PLAY_GROUPS && !ns)
			ns = "??? (can't know yet)";
		break;
	case PLAY_NONE: break;
	case PLAY_GROUPS_RANDOMLY:
		ns = "You shouldn't see this!";
	}
	//don't draw next_song on the screen yet, because this function can be
	//called from different threads.
	if (!ns)
		return;

	if (LOCK_NCURSES)
	{
		songinf.update = 32;
		if (songinf.next_song)
			free(songinf.next_song);
		songinf.next_song = strdup(chop_path(ns));
		UPDATE_CURSES;
		UNLOCK_NCURSES;
	}
}

void
set_song_info(const char *fl, struct song_info si)
{
	int maxy, maxx, r, i;
	bool show_hours = (si.totaltime >= 3600);
	char bla[64];
	int
		hours = si.totaltime / 3600,
		minutes = (si.totaltime % 3600) / 60,
		seconds  = si.totaltime % 60;
	
	draw_static();
	getmaxyx(stdscr, maxy, maxx);
	r = maxx - 12;
	//total time
	move(13,r+8);addch(':');
	move(14,r+8);addch(':');
	if (show_hours)
	{
		move(14, r+5);
		addch(':');
	}
	move(6,r); addstr("Mpg  Layer");
	move(7,r); addstr("  Khz    kb");
	attrset(COLOR_PAIR(CP_NUMBER)|A_BOLD);
	if (si.totaltime >= 3600)
	{
		move(14, r+3);
		printw("%2d", hours);
		move(14,r+6); printw("%02d", minutes);
	}
	else
	{
		move(14, r+3);
		addstr("   ");
		move(14,r+6); printw("%2d", minutes);
	}
	move(14,r+9); printw("%02d", seconds);
	attroff(A_BOLD);
	//mp3 info
	move(8,r); addstr("           ");
	move(8,r); addnstr(si.mode,11);
	move(6,r+3); printw("%1d", si.mp3_version + 1);
	move(6,r+10); printw("%1d", si.mp3_layer);
	move(7,r); printw("%2d", (si.samplerate > 1000 ? si.samplerate / 1000 :
		si.samplerate));
	move(7,r+6); printw("%-3d", si.bitrate);
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	//clear songname
	for (i = 4; i < maxx - 14; i++)
	{
		move(maxy-3, i); addch('x');
		move(maxy-3, i); addch(' ');
	}

	move(maxy-3,4);
	if (!strlen(si.artist))
		addnstr(fl, maxx - 18);
	else
		printw("%s - %s", (strlen(si.artist) ? si.artist :
		"<Unknown Artist>"), (strlen(si.songname) ? si.songname : 
		"<Unknown Songname>") );	
	sprintf(bla, "%s (%s)", (si.album && strlen(si.album) ? si.album :
		"<Unknown Album>"), (si.comment && strlen(si.comment) ? si.comment : 
		"no comments"));
	mw_settxt(bla);
	refresh();
}

void
set_song_status(playstatus_t s)
{
	int maxy, maxx;
	
	getmaxyx(stdscr,maxy,maxx);
	move(maxy-3,1);
	switch(s)
	{
	case PS_PLAY: addstr("|>");break;
	case PS_PAUSE: addstr("||");break;
	case PS_REWIND: addstr("<<");break;
	case PS_FORWARD: addstr(">>");break;
	case PS_PREV: addstr("|<");break;
	case PS_NEXT: addstr(">|");break;
	case PS_STOP: addstr("[]"); mw_clear(); break;
	case PS_RECORD: addstr("()");break;
	default: break;
	}
	refresh();
}

void
set_song_time(int elapsed, int remaining)
{
	int maxy, maxx, r;
	bool show_hours = (elapsed >= 3600);
	int
		seconds = elapsed %60,
		minutes = (elapsed % 3600) / 60,
		hours = (elapsed / 3600);

	if (remaining); //suppress warning

	getmaxyx(stdscr, maxy, maxx);
	r = maxx - 12;
	//print elapsed time
	attrset(COLOR_PAIR(CP_NUMBER)|A_BOLD);
	if (show_hours)
	{
		move(13,r+3); printw("%2d", hours);
		move(13,r+6); printw("%02d", minutes);
	}
	else
	{
		move(13,r+3); addstr("   ");
		move(13,r+6); printw("%2d", minutes);
	}
	move(13,r+9); printw("%02d", seconds);
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	move(13,r+8); addch(':');
	if (show_hours)
	{
		move(13, r+5); addch(':');
	}
	refresh();
}

int
myrand(int max_nr)
{
	return (int)(((max_nr * 1.0) * rand()) / (RAND_MAX + 1.0));
}

void
reset_playlist(int full)
{
	debug("reset_playlist(%d)\n", full);

	if (!full) //only reset currently played group
	{
		if (playing_group)
		{
			playing_group->resetSongs(0); //reset SONGS in played group
			playing_group->setNotPlaying(); //reset playing status of played group
			playing_group->setNotPlayed(); //reset PLAYLIST status of played group
			playing_group = NULL;
		}
	}
	else
	{
		if (playing_group)
			playing_group->setNotPlaying();
		playing_group = NULL;
		mp3_groupwin = NULL;
		mp3_rootwin->resetSongs();
		mp3_rootwin->resetGroups();
	}

	//reset history as well
	if (history)
	{
		delete history;
		history = new History();
	}
}

void
cw_draw_repeat()
{
	move(12,COLS-11);
	if (globalopts.repeat)
		addch('X');
	else
		addch(' ');
}
 
/* Function   : init_helpwin
 * Description: sets up the helpwin that's on top of the ncurses window.
 *            : Do this whenever you change program mode (normal mode to
 *            : file manager mode e.g.)
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
init_helpwin()
{
	int i = 0, item_count = 0;
	char line[80], desc[27], keylabel[4];
	struct keybind_t k = keys[i];
	
	if (helpwin)
	{
		delete helpwin;
		helpwin = NULL;
	}

	memset(line, 0, sizeof(line));
	//TODO: scrollwin never uses first&last line if no border's used!!
	helpwin = new scrollWin(4, COLS > 2 ? COLS-2 : 1,1,1,NULL,0,CP_DEFAULT,0);
	helpwin->setWrap(globalopts.wraplist);
	helpwin->hideScrollbar();
	//content is afhankelijk van program-mode, dus bij verandereing van
	//program mode moet ook de commandset meeveranderen.
	while (k.cmd != CMD_NONE)
	{
		if (k.pm & progmode)
		{
			set_keylabel(k.key, keylabel);
			sprintf(desc, "[%3s] %-19s ", keylabel, k.desc);
			if (!(item_count%3))
				strcpy(line, desc);
			else
				strcat(line, desc);
			if (item_count%3 == 2)
			{
				helpwin->addItem(line);
				memset(line, 0, sizeof(line));
			}
			item_count++;
		}
		k = keys[++i];
	}
	if (item_count%3)
		helpwin->addItem(line);
	helpwin->swRefresh(2);
}

void
bindkey(command_t cmd, int kcode)
{
	int i = 0;
	struct keybind_t k = keys[i];
	while (k.cmd != CMD_NONE)
	{
		if (k.cmd == cmd)
		{
			keys[i].key = kcode;
			break;
		}
		k = keys[++i];
	}
}

/* puts the label corresponding to a command in 'label', which must have at
 * least size of 4.
 */
void
get_label(command_t cmd, char *label)
{
	int i = 0;
	struct keybind_t k = keys[i];

	while (k.cmd != cmd)
	{
		k = keys[i++];
	}
	set_keylabel(k.key, label);	
}

/* label must be alloc'd with size 4.
 * Produces a 3-character label + terminating 0 that corresponds to the given
 * keycode 'k'
 */
void
set_keylabel(int k, char *label)
{
	int i;
	struct keylabel_t kl;

	label[3] = '\0';

	if (k > 32 && k < 177)
	{
		sprintf(label, " %c ", k);
		return;
	}
	for (i = 0; i < 64; i++)
	{
		if (k == KEY_F(i))
		{
			sprintf(label, "F%2d", i);
			return;
		}
	}

	i = 0; kl = klabels[i];
	while (kl.key != ERR)
	{
		if (kl.key == k)
		{
			sprintf(label, "%s", kl.desc);
			return;
		}
		kl = klabels[++i];
	}

	sprintf(label, "s%02x", (unsigned int)k);
	return;
}

/* Draws the purple colour over the keylabels.
 * It's ugly, but there are (currently) no means to have scrollWin colour
 * only parts of a string.  TODO: implement that!
 */
void
repaint_help()
{
	int i, j;

	/* Redhat 7.2 ships with a buggy ncurses version, which will crash on 
	 * chgat. Therefore, they will not get funky colours in the help menu.
	 */
#if defined(NCURSES_VERSION_MAJOR) && (!defined(NCURSES_VERSION_PATCH) || !(NCURSES_VERSION_MAJOR == 5 && NCURSES_VERSION_MINOR == 2 && NCURSES_VERSION_PATCH == 20010714))
	for (i = 1; i < 5; i++)
	{
		for (j = 2; j < 55; j += 26)
		{
			move (i, j); 
			chgat(3, A_NORMAL, CP_BUTTON, NULL);
		}
	}
#endif
	touchline(stdscr, 1, 4);
	refresh();
	//refresh_screen();
}

/* mode == 0 (default): next song
 * mode == 1: jump to previous song
 * mode == 2: generate new 'next' song without jumping to another song
 * returns NULL if no next song to play could be determined..
char *
determine(short mode)
{
	if (!mode)
	{
		if (!(played_songs[++current_song])) //no next song.
			played_songs[current_song] = determine_song();

		//there's a need to realloc the played_songs array?
		if (current_song == nr_played_songs - 1)
		{
			played_songs = (char**)realloc(played_songs, ++nr_played_songs * 
				sizeof(char*));
			played_songs[current_song + 1] = NULL;
		}

		if (!played_songs[current_song]) //end of playlist probably
			return NULL;

		//determine next song to play
		if (!played_songs[current_song +1 ])
			played_songs[current_song + 1] = determine_song();
		return played_songs[current_song];
	}
	else if (mode == 1) // jump to previous song
	{
		if (current_song == 0) // this is the first song in the list
			return played_songs[current_song];
		else
			return played_songs[--current_song];
	}
	else if (mode == 2) // determine new 'next' song
	{
		if (current_song == nr_played_songs - 1)
		{
			played_songs = (char**)realloc(played_songs, ++nr_played_songs * 
				sizeof(char*));
			played_songs[current_song + 1] = NULL;
		}
		if (played_songs[current_song + 1])
		{
			delete[] played_songs[current_song + 1];
			played_songs[current_song + 1] = NULL;
		}

	}
}
 */

void
init_playopts()
{
	playopts.song_played = 0;
	debug("init_playopts():  allow_repeat = 0\n");
	playopts.allow_repeat = 0;
	playopts.playing_one_mp3 = 0;
	playopts.pause = 0;
	playopts.playing = 0;
	playopts.one_mp3 = NULL;
	playopts.player = NULL;
	playopts.quit_program = 0;
}

void
init_globalopts()
{
	globalopts.no_mixer = 0; /* mixer on by default */
	globalopts.fpl = 10; /* 5 frames are played between input handling */
	globalopts.sound_device = NULL; /* default sound device */
	globalopts.mixer_device = NULL; /* default mixer device */
	globalopts.downsample = 0; /* no downsampling */
	globalopts.eightbits = 0; /* 8bits audio off by default */
	globalopts.fw_sortingmode = FW_SORT_ALPHA; /* case insensitive dirlist */
	globalopts.fw_hideothers = 0; /* show all files */
	globalopts.play_mode = PLAY_GROUPS;
	globalopts.warndelay = 2; /* wait 2 seconds for a warning to disappear */
	globalopts.skipframes=10; /* skip 100 frames during music search */
	globalopts.pan_size = 5; /* pan left/right 5 chars at once */
	globalopts.debug = 0; /* no debugging info per default */
	globalopts.display_mode = 1; /* file display mode (1=filename, 2=id3,
	                              * not finished yet.. */
	globalopts.repeat = 0;
	globalopts.extensions = NULL;
	globalopts.plist_exts = NULL;
	globalopts.playlist_dir = get_homedir(NULL);
	globalopts.recode_table = NULL;
#ifdef PTHREADEDMPEG
#if !defined(__FreeBSD__)
	globalopts.threads = 100;
#else
	/* FreeBSD playback using threads is much less efficient for some reason */
	globalopts.threads = 0;
#endif
#endif
	globalopts.want_id3names = 0;
	globalopts.selectitems_unselectfirst = 0;
	globalopts.selectitems_searchusingregexp = 0;
	globalopts.selectitems_caseinsensitive = 1; //only works for regexp search
	globalopts.scan_mp3s = 0; //scan mp3's to calculate correct total time.
	globalopts.wraplist = true;
#if WANT_SDL
	globalopts.audio_driver = AUDIODEV_SDL; //recommended for hick-free playback
#elif WANT_OSS
	globalopts.audio_driver = AUDIODEV_OSS;
#else
	globalopts.audio_driver - AUDIODEV_ESD; //hicks too much, don't use or fix.
#endif
}

void
reset_next_song()
{
	int width = (COLS > 33 ? COLS - 33 : 1);
	char emptyline[width+1];

	next_song = -1;
	move(7,19);
	//clear old text
	memset(emptyline, ' ', width * sizeof(char));
	emptyline[width] = '\0';
	addstr(emptyline);
	refresh();
}

/* Draws next song to be played on the screen */
void
draw_next_song(const char *ns)
{
	int mywidth = (COLS > 33 ? COLS - 33 : 1);
	char empty[mywidth+1];

	if (!ns)
		return;
	
	memset((void*)empty, ' ', mywidth * sizeof(char));
	empty[mywidth] = '\0';

	move(7,19);
	addstr(empty);
	move(7,19);
	addnstr(ns, mywidth);
	refresh();
}

short
set_playlist_dir(const char *dirname)
{
	if (!dirname || !strlen(dirname))
		return 0;
	
	if (globalopts.playlist_dir)
	{
		free(globalopts.playlist_dir);
		globalopts.playlist_dir = NULL;
	}

	globalopts.playlist_dir = strdup(dirname);

	if (globalopts.playlist_dir)
		return 1;
	return 0;
}

/* Function   : fw_markfilebad
 * Description: Moves the given file to a subdir called 'baditems' in the same
 *            : dir as this file is in. If the subdir does not exist, one will
 *            : be created.
 * Parameters : file: The file to mark as bad.
 * Returns    : If the move was successful, 1, else 0.
 * SideEffects: If called from within PM_FILEMANAGER mode, you'll likely want
 *            : to refresh the current directory (fw_changedir(".")).
 */
short
fw_markfilebad(const char *file)
{
	//extract path from filename.
	//check/create subdir baditems
	//move file to subdir

	const char
		*baditem,
		*app_path = "baditems/";
	char
		*baddir = NULL,
		*oldpath = NULL;
	struct stat
		bdstat;
	short
		result = 0;

if (file){ debug("FILE: ");debug(file);debug("\n");}
	baditem = chop_path(file);
	oldpath = chop_file(file); //must be delete[]'d

	baddir = new char[strlen(oldpath) + strlen(app_path) + strlen(baditem) + 1];
	strcpy(baddir, oldpath);
	strcat(baddir, app_path);
	debug("baddir: "); debug(baddir); debug("\n");

	if (stat(baddir, &bdstat) < 0)
	{
		int trymkdir = mkdir(baddir, 0755);
		if (trymkdir < 0)
		{
			//warning("Could not make directory 'baditems' for bad audiofile\n");
			delete[] baddir;
			delete[] oldpath;
			return 0;
		}
	}
	else if (!is_dir(baddir))
	{
		/*
		warning("Can't create a dir 'baditems' because a file with that "\
			"name already exists\n");
		 */
		delete[] baddir;
		delete[] oldpath;
		return 0;
	}

	strcat(baddir, baditem);
	debug("baddir(2): ");
	debug(baddir);
	debug("\n");
	int rsuccess = rename(file, (const char*)baddir);
	if (rsuccess < 0)
		result = 0;
	else
		result = 1;
	//warning("Failed to mark file as bad.\n");
	
	delete[] baddir;
	delete[] oldpath;

	return result;
}

/* Function   : get_mainwin_size
 * Description: Stores the main window (the biggest square window on the screen
 *            : size and position in the provided int pointers.
 * Parameters : y,x: top-left coordinate of window on stdscr
 *            : height,width: You'll figure this out..
 * Returns    : Nothing. The values are stored in the provided pointers.
 * SideEffects: None.
 */
void
get_mainwin_size(int *height, int *width, int *y, int *x)
{
	*height = calc_mainwindow_height(LINES);
	*width = calc_mainwindow_width(COLS);
	*y = 10;
	*x = 0;
}

void
get_mainwin_borderchars(chtype *ls, chtype *rs, chtype *ts, chtype *bs, 
                        chtype *tl, chtype *tr, chtype *bl, chtype *br)
{
	*ls = ACS_VLINE;
	*rs = ACS_VLINE;
	*ts = ACS_HLINE;
	*bs = ACS_HLINE;
	*tl = ACS_LTEE;
	*tr = ACS_PLUS;
	*bl = ACS_LTEE;
	*br = ACS_PLUS;
}

void
change_program_mode(unsigned short pm)
{
	progmode = pm;
	if (pm != PM_NORMAL)
		draw_settings(1);
	else
		draw_settings();
	init_helpwin();
	repaint_help();
};

/* PRE: ncurses_mutex *must* be locked
 */
void
update_ncurses(int update_all)
{
	if (update_all || songinf.update & 1)
		set_song_info((const char *)songinf.path, songinf.songinfo);
	if (update_all || songinf.update & 2)
		set_song_time(songinf.elapsed, songinf.remain);
	if (update_all || songinf.update & 4)
		set_song_status(songinf.status);
	if ((songinf.update & 8 && songinf.warning))
	{
		mw_settxt((const char *)songinf.warning);
		delete[] songinf.warning; songinf.warning = NULL;
		refresh_screen();
	}
	if (update_all || songinf.update & 16)
		draw_play_key(1);
	if (update_all || songinf.update & 32) //draw next_song on screen.
	{
		if (songinf.next_song)
		{
			draw_next_song((const char*)(songinf.next_song));
			//free(songinf.next_song);
		}
		else
			reset_next_song();
		//songinf.next_song = NULL;
	}
	songinf.update = 0;
}

void
update_songinfo(playstatus_t status, short update)
{
	if (LOCK_NCURSES)
	{
		songinf.update = update;
		songinf.status = status;
		UPDATE_CURSES;
		UNLOCK_NCURSES;
	}
}

void
end_program()
{
	quit_after_playlist = 0;
	if (playopts.playing)
		stop_song();
	//prevent ncurses clash
	if (LOCK_NCURSES);
	endwin();
	UNLOCK_NCURSES;
	exit(0);
};

void
fw_toggle_sort()
{
	sortmodes_t newmode = globalopts.fw_sortingmode;

	switch(globalopts.fw_sortingmode)
	{
	case FW_SORT_ALPHA: newmode = FW_SORT_ALPHA_CASE; break;
	case FW_SORT_ALPHA_CASE: newmode = FW_SORT_MODIFY_NEW; break;
	case FW_SORT_MODIFY_NEW: newmode = FW_SORT_MODIFY_OLD; break;
	case FW_SORT_MODIFY_OLD: newmode = FW_SORT_SIZE_SMALL; break;
	case FW_SORT_SIZE_SMALL: newmode = FW_SORT_SIZE_BIG; break;
	case FW_SORT_SIZE_BIG: newmode = FW_SORT_NONE; break;
	case FW_SORT_NONE: newmode = FW_SORT_ALPHA; break;
	}
	globalopts.fw_sortingmode = newmode;
	fw_draw_sortmode(newmode);
}

void
fw_draw_sortmode(sortmodes_t sm)
{
	const char *dsc[] = {
		"Sort alphabetically, case-insensitive",
		"Sort alphabetically, case-sensitive",
		"Sort by modification date, newest first",
		"Sort by modification date, oldest first",
		"Sort by file size, smallest first",
		"Sort by file size, biggest first",
		"Don't sort",
		NULL,
	};
	short
		idx = 0;
	int maxlen = (COLS > 16 ? COLS - 16 : 1);
	
	char emptyline[maxlen + 1];
	memset(emptyline, ' ', maxlen * sizeof(char));
	emptyline[maxlen] = '\0';

	switch(sm)
	{
	case FW_SORT_ALPHA:      idx = 0; break;
	case FW_SORT_ALPHA_CASE: idx = 1; break;
	case FW_SORT_MODIFY_NEW: idx = 2; break;
	case FW_SORT_MODIFY_OLD: idx = 3; break;
	case FW_SORT_SIZE_SMALL: idx = 4; break;
	case FW_SORT_SIZE_BIG:   idx = 5; break;
	case FW_SORT_NONE:       idx = 6; break;
	}
	move(6, 2); addstr(emptyline);
	move(6, 2); printw("Sorting mode   : %s", dsc[idx]);
}

/* Function   : set_sort_mode
 * Description: Sets the sort mode for the file manager
 * Parameters : mstring: canonical name for sortmode
 * Returns    : -1 on failure, >=0 on success
 * SideEffects: None.
 */
short
set_sort_mode(const char *mstring)
{
	//TODO: make a nice struct with canonical name, enum, indexNR
	const char *dsc[] = {
		"alpha",
		"alpha-case",
		"modify-new",
		"modify-old",
		"size-small",
		"size-big",
		"none",
		NULL
	};
	unsigned int i = 0;
	short found_it = -1;

	while (dsc[i])
	{
		if (!strcasecmp(mstring, dsc[i]))
		{
			found_it = i;
			break;
		}
		i++;
	}
	if (found_it < 0)
		return -1;
	
	sortmodes_t smode = globalopts.fw_sortingmode;

	switch(found_it)
	{
	case 0: smode = FW_SORT_ALPHA; break;
	case 1: smode = FW_SORT_ALPHA_CASE; break;
	case 2: smode = FW_SORT_MODIFY_NEW; break;
	case 3: smode = FW_SORT_MODIFY_OLD; break;
	case 4: smode = FW_SORT_SIZE_SMALL; break;
	case 5: smode = FW_SORT_SIZE_BIG; break;
	case 6: smode = FW_SORT_NONE; break;
	}
	globalopts.fw_sortingmode = smode;
	if (progmode == PM_FILESELECTION)
		fw_draw_sortmode(smode);
	return found_it;
}

short
fw_toggle_display()
{
	if (!file_window)
		return 0;

	short dispmode = file_window->getDisplayMode() + 1;
	if (dispmode > 2)
		dispmode = 0;
	file_window->setDisplayMode(dispmode);
	//refresh fm after this function has been called.
	return 1;
}

/* delete currently selected file */
void
fw_delete()
{
	const char
		*file = file_window->getSelectedItem();
	
	if (file && !is_dir(file))
	{
		if (unlink(file) < 0)
			warning("Could not delete file.");
		else
		{
			fw_changedir(".");
			mw_settxt("File deleted.");
		}
	}
}

void
warning(const char *txt, ... )
{
	va_list ap;
	char buf[1025];

	mw_clear();
	move(LINES-2,1);
	attrset(COLOR_PAIR(CP_ERROR)|A_BOLD);
	va_start(ap, txt);
	vsnprintf(buf, 1024, txt, ap);
	va_end(ap);
	addnstr(buf, (COLS > 14 ? COLS - 14 : 1));
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	refresh();
}

void
unlock_playing_mutex()
{
#ifdef PTHREADEDMPEG
	pthread_mutex_unlock(&playing_mutex);
#elif defined(LIBPTH)
	pth_mutex_release(&playing_mutex);
#endif
}

void
lock_playing_mutex()
{
#ifdef PTHREADEDMPEG
	pthread_mutex_lock(&playing_mutex);
#elif defined(LIBPTH)
	pth_mutex_acquire(&playing_mutex, FALSE, NULL);
#endif
}

void
wake_player()
{	
	debug("wake_player\n");
#ifdef PTHREADEDMPEG
	pthread_cond_signal(&playing_cond);
#elif defined(LIBPTH)
	pth_cond_notify(&playing_cond, TRUE);
#endif
	debug("/wake_player\n");
}

void
add_init_opt(struct init_opts *myopts, const char *option, void *value)
{
	struct init_opts *tmp;

	//find the last init_opts, and add a new one. Unless the first one's 
	//empty (!tmp->option[0])
	tmp = myopts;
	if (tmp->option[0])
	{
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = new init_opts;
		tmp = tmp->next;
	}
	tmp->value = value;
	memset(tmp->option, 0, 30);
	strncpy(tmp->option, option, 29);
	tmp->next = NULL;
}

void
get_input(
	WINDOW *win, const char*defval, unsigned int size, unsigned int maxlen, int y, int x,
	void (*got_input)(const char *, void *), void *args
)
{
	if (global_input)
	{
		delete global_input;
		global_input = NULL;
	}
	
	global_input = new getInput(win, defval, size, maxlen, y, x);
	
	input_mode = IM_INPUT;
	global_input->setCallbackFunction(got_input, args);
	mw_settxt("Use up and down arrow keys to toggle insert mode");
}

void
set_inout_opts()
{
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	leaveok(stdscr, FALSE);
	curs_set(0);
}

/* Function   : select_files_by_pattern
 * Description: Select a bunch of files by giving a pattern. Function is called
 *            : from popup_win
 * Parameters : pattern: Pattern to search files for.
 *            : args:    pointer to scrollWin instance to search in
 * Returns    : Nothing.
 * SideEffects: Selects files in the scrollWin instance.
 */
void
select_files_by_pattern(const char *pattern, void *args)
{
	scrollWin
		*mainwindow = (scrollWin*)args;

	if (!pattern || !strlen(pattern))
	{
		warning("No pattern given.");
		return;
	}

	if (globalopts.selectitems_unselectfirst)
		mainwindow->deselectItems();

	mainwindow->selectItems(pattern,
		(globalopts.selectitems_searchusingregexp ? "regex" : "global"),
		(globalopts.selectitems_caseinsensitive ? SW_SEARCH_CASE_INSENSITIVE : 
		SW_SEARCH_NONE),
		mainwindow->getDisplayMode());
	mainwindow->swRefresh(0);
}

/* Function   : playlist_write
 * Description: called from popup_win, this function will write the current
 *            : playlist to a file given by the user.
 * Parameters : playlistfile
 *            :   name of the playlist file to write to. free() after use
 *            : args: unused
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
playlist_write(const char *playlistfile, void *args)
{
	const char
		*plistfile = NULL;
	char
		*org_path = NULL;
	char
		*tmp_pfile = NULL;

	if (args);

	if (!playlistfile || !strlen(playlistfile))
		return;

	if (!is_playlist(playlistfile))
	{
		tmp_pfile = (char*)malloc((strlen(playlistfile) + 5) * sizeof(char));
		sprintf(tmp_pfile, "%s.lst", playlistfile);
		plistfile = chop_path(tmp_pfile);
	}
	else
	{
		plistfile = chop_path(playlistfile);
	}

	if (!plistfile || !strlen(plistfile))
	{
		free(tmp_pfile);
		return;
	}

	org_path = get_current_working_path();
	char *tmpDir = expand_path(globalopts.playlist_dir);

	do {
		if (!tmpDir) {
			warning("Failed to change to playlist dir.");	
			break;
		}

		if (chdir(tmpDir) == -1) {
			warning("Failed to change to playlist dir.");
			break;
		}

		if (!write_playlist(plistfile))
			warning("Failed to write playlist.");
		else
			mw_settxt("Playlist written.");
	} while (0);

	if (tmpDir)
		free(tmpDir);

	if (chdir(org_path) == -1) {
		warning("Failed to change to original path.");
	}
		
	free(org_path);
	free(tmp_pfile);
}

void
fw_rename_file(const char *newname, void *args)
{
	fileManager
		*fm = file_window;

	if (args);

	if (progmode != PM_FILESELECTION || !fm)
		return;

	const char *base_path = fm->getPath();
	const char *oldname = fm->getSelectedItem(0);

	if (!oldname || !newname || !strlen(newname))
	{
		warning("Nothing to rename (to).");
		return;
	}
	char *new_path = new char[strlen(base_path) + 1 + strlen(newname) + 1];
	sprintf(new_path, "%s/%s", base_path, newname);

	struct stat tmpbuf;

	if (stat(new_path, &tmpbuf) == 0) //it exists!
	{
		warning("Cowardly refusing to overwrite existing file.");
		return;
	}
	
	int rename_result = rename(oldname, new_path);
	delete[] new_path;

	if (rename_result == -1)
	{
		const char *errormsg = (const char *)strerror(errno);
		if (!errormsg)
			errormsg = "Unknown error occurred";
		char *warningmsg = new char[strlen(errormsg) + 50];
		sprintf(warningmsg, "Rename failed: %s", errormsg);
		warning(warningmsg);
		debug(warningmsg);
		delete[] warningmsg;
	}
	else
	{
		fw_changedir("."); //refresh dir
		mw_settxt("File renamed.");
	}
	return;
}

int ctoi(char *s)
{
	char *foo;
	int res = strtoul(s, &foo, 0);
	if (*foo) /* parse error */
		return 0;
	return res;
}

short
read_recode_table(const char *charMapFileName)
{
	FILE *fp;
	unsigned char buf[512],*p,*q;
	int in,on,count;
	int line;
	int i;
	short result = 1;

	if ((fp=fopen((char *)charMapFileName,"r")) == NULL)
	{
		fprintf(stderr,"readRecodeTable: cannot open char map file \"%s\"\n", charMapFileName);
		return 0;
	}

	count=0;
	line = 0;

	if (globalopts.recode_table)
		free(globalopts.recode_table);
	globalopts.recode_table = (unsigned char *) malloc(sizeof(unsigned char) * 256);
	for (i = 0; i < 256; i++)
		globalopts.recode_table[i] = i;

	while (fgets((char*)buf,sizeof(buf),fp))
	{
		line++;
		p=(unsigned char *)strtok((char*)buf," \t\n#");
		q=(unsigned char *)strtok(NULL," \t\n#");

		if (p && q)
		{
			in = ctoi((char *)p);
			if (in > 255)
			{
				fprintf(stderr, "readRecodeTable: %s: line %d: char val too big\n", charMapFileName, line);
				result = 0;
				break;
			}

			on=ctoi((char *)q);
			if (in && on)
			{
				if ( count++ < 256 )
					globalopts.recode_table[in]=(char)on;
				else
				{
					fprintf(stderr,"readRecodeTable: char map table \"%s\" is big\n",charMapFileName);
					result = 0;
					break;
				}
			}
		}
	}

	fclose(fp);
	return result;
}

short
set_charset_table(const char *tabName)
{
	if (!strlen(tabName))
		return 0;
	return read_recode_table(tabName);
}

short
set_audio_driver(const char *driverName)
{
#ifdef WANT_OSS
	if (!strcasecmp(driverName, "oss"))
	{
		globalopts.audio_driver = AUDIODEV_OSS;
		return 1;
	}
#endif

#ifdef WANT_ESD
	if (!strcasecmp(driverName, "esd"))
	{
		globalopts.audio_driver = AUDIODEV_ESD;
		return 1;
	}
#endif

#ifdef WANT_SDL
	if (!strcasecmp(driverName, "sdl"))
	{
		globalopts.audio_driver = AUDIODEV_SDL;
		return 1;
	}
#endif

#ifdef WANT_NAS
	if (!strcasecmp(driverName, "nas"))
	{
		globalopts.audio_driver = AUDIODEV_NAS;
		return 1;
	}
#endif

	return 0; //bad value or unsupported audio driver
}

short
set_pan_size(int psize)
{
	if (psize < 1 || psize > 40)
		return 0;

	globalopts.pan_size = psize;
	return 1;
};

int
main(int argc, char *argv[], char *envp[])
{
	int
		c,
		long_index,
		key,
		options = 0,
		playmp3s_nr = 0;
	time_t t;
	char
		**playmp3s = NULL,
		*init_playlist = NULL,
		*chroot_dir = NULL,
		*config_file = NULL;
	struct _tmp
	{
		char *play_mode;
		char *sound_device;
		char *mixer_device;
		int fpl;
#ifdef PTHREADEDMPEG
		int threads;
#endif
	} tmp;

#ifdef LIBPTH
	pth_init();
#endif
	
	tmp.sound_device = NULL;
	tmp.mixer_device = NULL;
	tmp.play_mode = NULL;
	tmp.fpl = 5;
	tmp.threads = 10;
	
	environment = envp;

	songinf.update = 0;
	songinf.path = NULL;
	songinf.next_song = NULL;
	songinf.warning = NULL;
	playing_group = NULL;
	status_file = NULL;
	mixer = NULL;
	helpwin = NULL;
	bighelpwin = NULL;
	mp3_groupwin = NULL;
	history = new History();
	long_index = 0;
	played_songs = (char**)malloc(sizeof(char*));
	played_songs[0] = NULL;
	nr_played_songs = 1;
	current_song = -1;
	srand((unsigned int)time(&t));
	init_globalopts();
	init_playopts();
	set_default_colours(); // fill globalopts.colours with default values.
	/* parse arguments */
	while (1)
	{
		static struct option long_options[] = 
		{
			{ "downsample", 0, 0, '2'},
			{ "8bits", 0, 0, '8'},
			{ "autolist", 1, 0, 'a'},
			{ "config-file", 1, 0, 'c'},
			{ "debug", 0, 0, 'd'},
			{ "help", 0, 0, 'h'},
			{ "list", 1, 0, 'l'},
			{ "mixer-device", 1, 0, 'm' },
			{ "no-mixer", 0, 0, 'n'},
			{ "chroot", 1, 0, 'o'},
			{ "playmode", 1, 0, 'p'},
			{ "dont-quit", 0, 0, 'q'},
			{ "repeat", 0, 0, 'R'},
			{ "runframes", 1, 0, 'r'},
			{ "sound-device", 1, 0, 's'},
			{ "status-file", 1, 0, 'f'},
			{ "threads", 1, 0, 't'},
			{ "version", 0, 0, 'v'},
			{ 0, 0, 0, 0}
		};
		
		c = getopt_long(argc, argv, "28a:c:df:hl:m:no:p:qRr:s:t:v", long_options,
			&long_index);

		if (c == EOF)
			break;
		
		switch(c)
		{
		case ':':
		case '?':
			usage();
			break;
		case '2': /* downsample */
			options |= OPT_DOWNSAMPLE;
			break;
		case '8': /* 8bit audio */
			options |= OPT_8BITS;
			break;
		case 'a': /* load playlist and play */
			options |= OPT_AUTOLIST;
			if (init_playlist)
				delete[] init_playlist;
			init_playlist = new char[strlen(optarg)+1];
			strcpy(init_playlist, optarg);
			break;
		case 'c': /* configuration file */
			if (config_file) free(config_file);
			config_file = strdup(optarg);
			break;
		case 'd':
			options |= OPT_DEBUG;
			break;
		case 'f': /* status file */
			if (status_file)
				free(status_file);
			status_file = strdup(optarg);
			break;
		case 'h': /* help */
			usage();
			break;
		case 'l': /* load playlist */
			options |= OPT_LOADLIST;
			if (init_playlist)
				delete[] init_playlist;
			init_playlist = new char[strlen(optarg)+1];
			strcpy(init_playlist, optarg);
			break;
		case 'm': /* mixer device */
			options |= OPT_MIXERDEV;
			if (tmp.mixer_device)
				free(tmp.mixer_device);
			tmp.mixer_device = strdup(optarg);
			break;
		case 'n': /* disable mixer */
			options |= OPT_NOMIXER;
			break;
		case 'o': /* chroot */
			options |= OPT_CHROOT;
			if (chroot_dir)
				delete[] chroot_dir;
			chroot_dir = new char[strlen(optarg)+1];
			strcpy(chroot_dir, optarg);
			break;
		case 'p': /* set initial playmode */
			options |= OPT_PLAYMODE;
			tmp.play_mode = new char[strlen(optarg)+1];
			strcpy(tmp.play_mode, optarg);
			break;
		case 'q': /* don't quit after playing 1 mp3/playlist */
			options |= OPT_QUIT;
			break;
		case 'R': /* playlist repeat */
			options |= OPT_REPEAT;
			break;
		case 'r': /* numbers of frames to decode in 1 loop 1<=fpl<=10 */
			options |= OPT_FPL;
			tmp.fpl = atoi(optarg);
			break;
		case 's': /* sound-device other than the default /dev/{dsp,audio} ? */
			options |= OPT_SOUNDDEV;
			if (tmp.sound_device)
				free(tmp.sound_device);
			tmp.sound_device = strdup(optarg);
			break;
		case 't': /* threads */
			options |= OPT_THREADS;
#ifdef PTHREADEDMPEG
			tmp.threads = atoi(optarg);
#endif
			break;
		case 'v': /* version info */
			printf("%s version %s - http://mp3blaster.sourceforge.net/\n",
				argv[0], VERSION);
			printf("Supported audio formats: " BUILDOPTS_AUDIOFORMATS "\n");
			printf("Supported audio output drivers: " BUILDOPTS_AUDIODRIVERS "\n");
			printf("Build features: " BUILDOPTS_FEATURES "\n");
			exit(0);
			break;
		default:
			usage();
		}
	}

	if (options & OPT_DEBUG) 
	{
		globalopts.debug = 1;
		debug("Debugging of mp3blaster started.\n");
	}

	//read .mp3blasterrc
	if (!cf_parse_config_file(config_file) &&
		(config_file || cf_get_error() != NOSUCHFILE))
	{
		fprintf(stderr, "%s\n", cf_get_error_string());
		exit(1);
	}
	if (config_file)
		free(config_file);

	if (optind < argc) /* assume all other arguments are mp3's */
	{
		/* this is not valid if a playlist is loaded from commandline */
		if ( (options&OPT_LOADLIST) || (options&OPT_AUTOLIST) )
			usage();

		if (!(playmp3s = (char**)malloc((argc - optind) * sizeof(char*))))
		{
			fprintf(stderr, "Cannot allocate enough memory.\n"); exit(1);
		}
		/* Build a playlist from the remaining arguments */
		for (int i = optind ; i < argc; i++)
		{
			if (!is_audiofile(argv[i]))
			{
				fprintf(stderr, "%s is not a valid audio file!\n", argv[i]);
				usage();
			}
			playmp3s[i - optind] = (char*)malloc((strlen(argv[i])+1) * 
				sizeof(char));
			strcpy(playmp3s[i - optind], argv[i]);
		}
		options |= OPT_PLAYMP3;
	}	

	//signal(SIGALRM, SIG_IGN);

	//Initialize NCURSES
	initscr();
 	start_color();
	set_inout_opts();

	startup_path = get_current_working_path();

	if (false && (LINES < MIN_SCREEN_HEIGHT || COLS < MIN_SCREEN_WIDTH))
	{
		mvaddstr(0, 0, "You'll need at least an 80x23 screen, sorry.\n");
		getch();
		endwin();
		exit(1);
	}
	
	/* setup colours */
	init_pair(CP_DEFAULT, globalopts.colours.default_fg,
		globalopts.colours.default_bg);
	init_pair(CP_POPUP, globalopts.colours.popup_fg,
		globalopts.colours.popup_bg);
	init_pair(CP_POPUP_INPUT, globalopts.colours.popup_input_fg,
		globalopts.colours.popup_input_bg);
	init_pair(CP_ERROR, globalopts.colours.error_fg,
		globalopts.colours.error_bg);
	init_pair(CP_BUTTON, globalopts.colours.button_fg,
		globalopts.colours.button_bg);
	init_pair(CP_SHORTCUTS, globalopts.colours.shortcut_fg,
		globalopts.colours.shortcut_bg);
	init_pair(CP_LABEL, globalopts.colours.label_fg,
		globalopts.colours.label_bg);
	init_pair(CP_NUMBER, globalopts.colours.number_fg,
		globalopts.colours.number_bg);
	init_pair(CP_FILE_MP3, globalopts.colours.file_mp3_fg,
		globalopts.colours.default_bg);
	init_pair(CP_FILE_DIR, globalopts.colours.file_dir_fg,
		globalopts.colours.default_bg);
	init_pair(CP_FILE_LST, globalopts.colours.file_lst_fg,
		globalopts.colours.default_bg);
	init_pair(CP_FILE_WIN, globalopts.colours.file_win_fg,
		globalopts.colours.default_bg);

	/* initialize selection window */
	mp3_rootwin = newgroup();
	mp3_rootwin->setTitle("[Default]");
	mp3_rootwin->swRefresh(0);
	mp3_rootwin->drawBorder(); //to get the title 
	mp3_curwin = mp3_rootwin;

	progmode = PM_NORMAL;

	/* initialize helpwin */
	init_helpwin();
	repaint_help();
	draw_static();
	mw_settxt("Press '?' to get help on available commands");
	refresh();

	if (options & OPT_NOMIXER)
	{
		globalopts.no_mixer = 1;
	}
	if (options & OPT_SOUNDDEV)
	{
		set_sound_device(tmp.sound_device);
		free(tmp.sound_device);
	}
	if (options & OPT_MIXERDEV)
	{
		set_mixer_device(tmp.mixer_device);
		free(tmp.mixer_device);
	}
	if (options & OPT_REPEAT)
	{
		globalopts.repeat = 1;
	}
	if (options & OPT_PLAYMODE)
	{
		if (!set_play_mode(tmp.play_mode))
		{
			endwin();
			usage();
		}
		delete[] tmp.play_mode;
	}
	if (options & OPT_FPL)
	{
		if (!set_fpl(tmp.fpl))
		{
			endwin();
			usage();
		}
	}

	if (options & OPT_THREADS)
	{
#ifdef PTHREADEDMPEG
		if (!set_threads(tmp.threads))
#else
		if (1) //threading is not enabled..
#endif
		{
			endwin();
			usage();
		}
	}

	if (options & OPT_CHROOT) {
		if (chroot(chroot_dir) < 0) {
			endwin();
			perror("chroot");
			fprintf(stderr, "Could not chroot to %s! (are you root?)\n",
				chroot_dir);
			exit(1);
		}

		if (-1 == chdir("/")) {
			fprintf(stderr, "Failed to change to root directory\n");
			exit(1);
		}
		delete[] chroot_dir;
	}

	if (options & OPT_DOWNSAMPLE)
		globalopts.downsample = 1;
	if (options & OPT_8BITS)
		globalopts.eightbits = 1;

	/* create nmixer interface */
	init_mixer();

	/* All options from configfile and commandline have been read. */
	draw_settings();
	
	/* Accept signal for redrawing the interface */
	signal(SIGWINCH, handle_resize);

/*\
|*|  very rude hack caused by nasplayer lib that outputs rubbish to stderr
\*/
	//fclose(stderr);
/*\
|*|  EORH
\*/
	
	action = AC_NONE;

	if (options & OPT_PLAYMP3)
	{
		const char *bla[4];
		short idx;
		short foo[2] = { 0, 1 };
		for (int i = 0; i < (argc - optind); i++)
		{
			bla[0] = (const char*)playmp3s[i];
			if (is_httpstream(bla[0]))
				bla[1] = bla[0];
			else
				bla[1] = chop_path(playmp3s[i]);
			bla[2] = NULL;
			bla[3] = NULL;

			idx=mp3_curwin->addItem(bla, foo, CP_FILE_MP3);
			if (idx!=-1)
				mp3_curwin->changeItem(idx,&id3_filename,strdup(bla[0]),2);
			delete[] playmp3s[i];
		}
		delete[] playmp3s; playmp3s_nr = 0;
		mp3_curwin->swRefresh(0);
		set_action(AC_PLAY);
		if (!(options & OPT_QUIT))
			quit_after_playlist = 1;
	}
	else if ((options & OPT_LOADLIST) || (options & OPT_AUTOLIST))
	{
		read_playlist(init_playlist);
		mp3_curwin->swRefresh(1);
		delete[] init_playlist;
		if (options & OPT_AUTOLIST)
			set_action(AC_PLAY);
	}

#ifdef PTHREADEDMPEG
	pthread_create(&thread_playlist, NULL, play_list, NULL);
# ifdef INCLUDE_LIRC
	 pthread_create(&thread_lirc, NULL, lirc_loop, NULL);
# endif
#elif defined(LIBPTH)
	warning("Setting up PTH thread");

	pth_mutex_init(&playing_mutex);
	pth_mutex_init(&ncurses_mutex);
	pth_mutex_init(&onemp3_mutex);
	pth_cond_init(&playing_cond);

	thread_playlist = pth_spawn(PTH_ATTR_DEFAULT, play_list, NULL);
	pth_yield(NULL);
#endif /* PTHREADEDMPEG */

	/* read input */
	while ( (key = handle_input(1)) >= 0)
	{
	}
	
	/* signal running threads to terminate themselves */
	playopts.quit_program = 1;
	wake_player();

#ifdef PTHREADEDMPEG
	pthread_join(thread_playlist, NULL);
	//the lirc thread blocks..sod it.
#elif defined (LIBPTH)
	pth_join(thread_playlist, NULL);
#endif

	endwin();
	return 0;
}


