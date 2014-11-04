/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) Bram Avontuur (brama@stack.nl)
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
 * This is an MP3 (Mpeg layer 3 audio) player. Although there are many 
 * mp3-players around for almost any platform, none of the others have the
 * ability of dividing a playlist into groups. Having groups is very useful,
 * since now you can randomly shuffle complete ALBUMS, without mixing numbers
 * of one album with another album. (f.e. if you have a rock-album and a 
 * new-age album on one CD, you probably wouldn't like to mix those!). With
 * this program you can shuffle the entire playlist, play groups(albums) in
 * random order, play in a predefined order, etc. etc.
 * Thanks go out to Jung woo-jae (jwj95@eve.kaist.ac.kr) and some other people
 * who made the mpegsound library used by this program. (see source-code from
 * splay-0.8)
 * If you like this program, or if you have any comments, tips for improve-
 * ment, money, etc, you can e-mail me at brama@stack.nl or visit my
 * homepage (http://www.stack.nl/~brama/).
 * If you want this program for another platform (like Win95) there's some
 * hope. I'm currently trying to get the hang of programming interfaces for
 * that OS (may I be forgiven..). Maybe other UNIX-flavours will be supported
 * too (FreeBSD f.e.). If you'd like to take a shot at converting it to such
 * an OS plz. contact me! I don't think the interface needs a lot of 
 * changing, but about the actual mp3-decoding algorithm I'm not sure since
 * I didn't code it and don't know how sound's implemented on other OS's.
 * Current version is 2.0b6. It probably contains a lot of bugs, but I've
 * completely rewritten the code from the 1.* versions into somewhat readable
 * C++ classes. It's not yet as object-oriented as I want it, but it's a
 * start..
 * KNOWN BUGS:
 * -Layout and/or functionality is screwed up on several stock distributions
 *  of ncurses. Notably the distributions on RedHat-5.2 and debian-2.0 are
 *  horrible. To be sure everything works as it should, UPGRADE to at least
 *  ncurses-4.2. (lib *and* headerfiles)
 * -Might coredump when an attempt to play an unsupported mp3 is made..
 *  although I think it plays all mp3's now. If you can reproduce a bug,
 *  PLEASE e-mail me.
 */
#include "mp3blaster.h"
#include NCURSES
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "scrollwin.h"
#include "gstack.h"
#include "mp3stack.h"
#include "fileman.h"
#include "mp3play.h"

/* paranoia define[s] */
#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

/* values for global var 'window' */
enum program_mode { PM_NORMAL, PM_FILESELECTION, PM_MP3PLAYING }
	progmode;

/* External functions */
extern void debug(const char *);

/* Prototypes */
void set_header_title(const char*);
void refresh_screen(void);
void Error(const char*);
int is_mp3(const char*);
int handle_input(short);
void cw_set_fkey(short, const char*);
void set_default_fkeys(program_mode pm);
void cw_toggle_group_mode(short notoggle);
void cw_toggle_play_mode(short notoggle);
void mw_clear();
void mw_settxt(const char*);
void read_playlist(const char *);
void write_playlist();
void play_list();
#ifdef PTHREADEDMPEG
void change_threads();
#endif
void add_selected_file(const char*);
char *get_current_working_path();
void play_one_mp3(const char*);
void usage();
void show_help();
void draw_extra_stuff(int file_mode=0);
char *gettext(const char *label, short display_path=0);
short fw_getpath();

#define OPT_LOADLIST 1
#define OPT_DEBUG 2
#define OPT_PLAYMP3 4
#define OPT_NOMIXER 8
#define OPT_AUTOLIST 16
#define OPT_QUIT 32
#define OPT_CHROOT 64

/* global vars */

playmode
	play_mode = PLAY_GROUPS;
WINDOW
	*command_window = NULL, /* on-screen command window */
	*header_window = NULL, /* on-screen header window */
	*message_window = NULL; /* on-screen message window (bottom window) */
mp3Stack
	*group_stack = NULL; /* groups available */
scrollWin
	*group_window = NULL; /* on-screen group-window */
fileManager
	*file_window = NULL; /* window to select files with */
int
	current_group, /* selected group (group_stack[current_group - 1]) */
	no_mixer,
#ifdef PTHREADEDMPEG
	threads = 100, /* amount of threads for playing mp3's */
#endif
	nselfiles = 0;
FILE
	*debug_info = NULL; /* filedescriptor of debug-file. */
int
	fpl = 5; /* frames to be run in 1 loop: Higher -> less 'hicks' on slow
	          * CPU's. Lower -> interface (while playing) reacts faster.
			  * 1<=fpl<=10
			  */

char
	*playmodes_desc[] = {
	"",									/* PLAY_NONE */
	"Play current group",				/* PLAY_GROUP */
	"Play all groups in normal order",	/* PLAY_GROUPS */
	"Play all groups in random order",	/* PLAY_GROUPS_RANDOMLY */
	"Play all songs in random order"	/* PLAY_SONGS */
	},
	**selected_files = NULL,
	*startup_path = NULL;

int
main(int argc, char *argv[])
{
	int
		c,
		long_index,
		key,
		options = 0;
	char
		*grps[] = { "01" },
		*dummy = NULL,
		*play1mp3 = NULL,
		*init_playlist = NULL,
		*chroot_dir = NULL;
	scrollWin
		*sw;

	long_index = 0;
	no_mixer = 0;
	/* parse arguments */
	while (1)
	{
#ifdef HAVE_GETOPT_H
		static struct option long_options[] = 
		{
			{ "autolist", 1, 0, 'a'},
			{ "chroot", 1, 0, 'c'},
			{ "debug", 0, 0, 'd'},
			{ "file" , 1, 0, 'f'},
			{ "help", 0, 0, 'h'},
			{ "list", 1, 0, 'l'},
			{ "no-mixer", 0, 0, 'n'},
			{ "playmode", 1, 0, 'p'},
			{ "quit", 0, 0, 'q'},
			{ "runframes", 1, 0, 'r'},
			{ 0, 0, 0, 0}
		};
		
		c = getopt_long_only(argc, argv, "a:c:df:hl:np:qr:", long_options,
			&long_index);
#else
		c = getopt(argc, argv, "a:c:df:hl:np:qr:");
#endif

		if (c == EOF)
			break;
		
		switch(c)
		{
		case ':':
		case '?':
			usage();
			break;
		case 'd':
			options |= OPT_DEBUG;
			break;
		case 'f': /* play one mp3 and exit */
			options |= OPT_PLAYMP3;
			if (play1mp3)
				delete[] play1mp3;
			play1mp3 = new char[strlen(optarg)+1];
			strcpy(play1mp3, optarg);
			break;
		case 'l': /* load playlist */
			options |= OPT_LOADLIST;
			if (init_playlist)
				delete[] init_playlist;
			init_playlist = new char[strlen(optarg)+1];
			strcpy(init_playlist, optarg);
			break;
		case 'n': /* disable mixer */
			options |= OPT_NOMIXER;
			break;
		case 'a': /* load playlist and play */
			options |= OPT_AUTOLIST;
			if (init_playlist)
				delete[] init_playlist;
			init_playlist = new char[strlen(optarg)+1];
			strcpy(init_playlist, optarg);
			break;
		case 'p': /* set initial playmode */
			if (!strcmp(optarg, "onegroup"))
				play_mode = PLAY_GROUP;
			else if (!strcmp(optarg, "allgroups"))
				play_mode = PLAY_GROUPS;
			else if (!strcmp(optarg, "groupsrandom"))
				play_mode = PLAY_GROUPS_RANDOMLY;
			else if (!strcmp(optarg, "allrandom"))
				play_mode = PLAY_SONGS;
			else
				usage();
			break;
		case 'q': /* quit after playing 1 mp3/playlist */
			options |= OPT_QUIT;
			break;
		case 'r': /* numbers of frames to decode in 1 loop 1<=fpl<=10 */
		{
			int torun = atoi(optarg);
			if (torun < 1)
				fpl = 1;
			if (torun > 10)
				fpl = 10;
			else
				fpl = torun;
		}
			break;
		case 'c': /* chroot */
			options |= OPT_CHROOT;
			if (chroot_dir)
				delete[] chroot_dir;
			chroot_dir = new char[strlen(optarg)+1];
			strcpy(chroot_dir, optarg);
			break;
		case 'h': /* help */
			usage();
			break;
		default:
			usage();
		}
	}
		
	initscr();
 	start_color();
	cbreak();
	noecho();
	nonl();
	//intrflush(stdscr, FALSE);
	//keypad(stdscr, TRUE);

	startup_path = get_current_working_path();

	if (LINES < 24 || COLS < 80)
	{
		mvaddstr(0, 0, "You'll need at least an 80x24 screen, sorry.\n");
		getch();
		endwin();
		exit(1);
	}
	
	/* setup colours */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_WHITE, COLOR_RED);
	init_pair(4, COLOR_WHITE, COLOR_BLUE);
	init_pair(5, COLOR_WHITE, COLOR_MAGENTA);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);

	/* initialize command window */
	command_window = newwin(LINES - 2, 24, 0, 0);
	wbkgd(command_window, ' '|COLOR_PAIR(1)|A_BOLD);
	leaveok(command_window, TRUE);
	wborder(command_window, 0, ' ', 0, 0, 0, ACS_HLINE, ACS_LTEE, ACS_HLINE);
	set_default_fkeys(PM_NORMAL);

	/* initialize header window */
	header_window = newwin(2, COLS - 27, 0, 24);
	leaveok(header_window, TRUE);
	wbkgd(header_window, ' '|COLOR_PAIR(1)|A_BOLD);
	wborder(header_window, 0, 0, 0, ' ', ACS_TTEE, ACS_TTEE, ACS_VLINE,
		ACS_VLINE);
	wrefresh(header_window);

	/* initialize selection window */
	sw = new scrollWin(LINES - 4, COLS - 27, 2, 24, NULL, 0, 0, 1);
	wbkgd(sw->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
	sw->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS, ACS_BTEE, ACS_PLUS);
	sw->swRefresh(0);
	sw->setTitle("Default");
	dummy = new char[11];
	strcpy(dummy, "01:Default");
	set_header_title(dummy);
	delete[] dummy;

	/* initialize group window */
	group_stack = new mp3Stack;
	group_stack->add(sw); /* add default group */
	current_group = 1; /* == group_stack->entry(current_group - 1) */

	group_window = new scrollWin(LINES - 4, 3, 2, COLS - 3, grps, 1, 0, 0);
	wbkgd(group_window->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
	group_window->setBorder(' ', 0, 0, 0, ACS_HLINE, ACS_RTEE, ACS_HLINE,
		ACS_RTEE);
	group_window->swRefresh(0);

	draw_extra_stuff();

	/* initialize message window */
	message_window = newwin(2, COLS - 3, LINES - 2, 0);
	wbkgd(message_window, ' '|COLOR_PAIR(1)|A_BOLD);
	leaveok(message_window, TRUE);
	wborder(message_window, 0, 0, ' ', 0, ACS_VLINE, ACS_VLINE, 0, ACS_BTEE);
	mw_settxt("Press 'h' to get a screen with key functions.");
	wrefresh(message_window);

	/* display play-mode in command_window */
	mvwaddstr(command_window, 13, 1, "Current group's mode:");
	cw_toggle_group_mode(1);
	mvwaddstr(command_window, 16, 1, "Current play-mode: ");
	cw_toggle_play_mode(1);

	progmode = PM_NORMAL;
#ifdef PTHREADEDMPEG
	change_threads(); /* display #threads in command_window */
#endif
	debug_info = NULL;

	if (options & OPT_DEBUG) 
	{
		char *homedir = getenv("HOME");
		const char flnam[] = ".mp3blaster";
		char *to_open = (char*)malloc((strlen(homedir) + strlen(flnam) +
			2) * sizeof(char));
		sprintf(to_open, "%s/%s", homedir, flnam);
		if (!(debug_info = fopen(to_open, "a")))
			warning("Couldn't open debuginfo-file!");
		free(to_open);
	}

	if (options & OPT_NOMIXER)
	{
		no_mixer = 1;
	}

	if (options & OPT_CHROOT)
	{
		if (chroot(chroot_dir) < 0)
		{
			endwin();
			perror("chroot");
			fprintf(stderr, "Could not chroot to %s! (are you root?)\n",
				chroot_dir);
			exit(1);
		}
		chdir("/");
		delete[] chroot_dir;
	}

	if (options & OPT_PLAYMP3)
	{
		play_one_mp3(play1mp3);
		delete[] play1mp3;
		if (options & OPT_QUIT)
		{
			endwin();
			exit(0);
		}
	}
	else if ((options & OPT_LOADLIST) || (options & OPT_AUTOLIST))
	{
		read_playlist(init_playlist);
		delete[] init_playlist;
		if (options & OPT_AUTOLIST)
		{
			play_list();
			if (options & OPT_QUIT)
			{
				endwin();
				exit(0);
			}
		}
	}

	/* read input from keyboard */
	while ( (key = handle_input(0)) >= 0);

	endwin();
	return 0;
}

/* Function Name: usage
 * Description  : Warn user about this program's usage and then exit nicely
 *              : Program must be in ncurses-mode when this function is
 *              : called.
 */
void
usage()
{
	fprintf(stderr, "%s",
#ifdef HAVE_GETOPT_H
		"Usage:\n" \
		"\tmp3blaster [options]\n"\
		"\tmp3blaster [options] --file/-f <filename.mp3>\n"\
		"\t\tPlay one mp3 (and exit with --quit).\n"\
		"\tmp3blaster [options] --list/-l <playlist.lst>\n"\
		"\t\tLoad a playlist but don't start playing.\n"\
		"\tmp3blaster [options] --autolist/-a <playlist.lst>\n"\
		"\t\tLoad a playlist and start playing.\n"\
		"\nOptions:\n"\
		"\t--help/-h: This help screen.\n"\
		"\t--no-mixer/-n: Don't start the built-in mixer.\n"\
		"\t--debug/-d: Log debug-info in $HOME/.mp3blaster.\n"\
		"\t--chroot/-c=<rootdir>: Set <rootdir> as mp3blaster's root dir.\n"\
		"\t\tThis affects *ALL* file operations in mp3blaster!!(including\n"\
		"\t\tplaylist reading&writing!) Note that only users with uid 0\n"\
		"\t\tcan use this option (yet?). This feature will change soon.\n"\
		"\t--quit/-q: Quit after playing mp3[s] (only makes sense in \n"\
		"\t\tcombination with --autolist or --file)\n"\
		"\t--playmode/-p={onegroup,allgroups,groupsrandom,allrandom}\n"\
		"\t\tDefault playing mode is resp. Play first group only, Play\n"\
		"\t\tall groups in given order(default), Play all groups in random\n"\
		"\t\torder, Play all songs in random order.\n"\
		"\t--runframes/-r=<number>: Number of frames to decode in one loop.\n"\
		"\t\tRange: 1 to 10 (default=5). A low value means that the\n"\
		"\t\tinterface (while playing) reacts faster but slow CPU's might\n"\
		"\t\thick. A higher number implies a slow interface but less\n"\
		"\t\thicks on slow CPU's.\n"\
		"");
#else
		"Usage:\n" \
		"\tmp3blaster [options]\n"\
		"\tmp3blaster [options] -f <filename.mp3>\n"\
		"\t\tPlay one mp3 (and exit with -q).\n"\
		"\tmp3blaster [options] -l <playlist.lst>\n"\
		"\t\tLoad a playlist but don't start playing.\n"\
		"\tmp3blaster [options] -a <playlist.lst>\n"\
		"\t\tLoad a playlist and start playing.\n"\
		"\nOptions:\n"\
		"\t-h: This help screen.\n"\
		"\t-n: Don't start the built-in mixer.\n"\
		"\t-d: Log debug-info in $HOME/.mp3blaster.\n"\
		"\t-c <rootdir>: Set <rootdir> as mp3blaster's root dir.\n"\
		"\t\tThis affects *ALL* file operations in mp3blaster!!(including\n"\
		"\t\tplaylist reading&writing!) Note that only users with uid 0\n"\
		"\t\tcan use this option (yet?). This feature will change soon.\n"\
		"\t-q: Quit after playing mp3[s] (only makes sense in \n"\
		"\t\tcombination with -a or -f)\n"\
		"\t-p {onegroup,allgroups,groupsrandom,allrandom}\n"\
		"\t\tDefault playing mode is resp. Play first group only, Play\n"\
		"\t\tall groups in given order(default), Play all groups in random\n"\
		"\t\torder, Play all songs in random order.\n"\
		"\t-r=<number>: Number of frames to decode in one loop.\n"\
		"\t\tRange: 1 to 10 (default=5). A low value means that the\n"\
		"\t\tinterface (while playing) reacts faster but slow CPU's might\n"\
		"\t\thick. A higher number implies a slow interface but less\n"\
		"\t\thicks on slow CPU's.\n"\
		"");
#endif

	exit(1);
}

/* Function Name: set_header_title
 * Description  : sets the title of the current on-screen selection window
 *              : (group_stack[current_group-1]) or from the current file-
 *              : selector-window. (file_window). 
 * Arguments	: title: title to set.
 */
void
set_header_title(const char *title)
{
	int i, y, x, middle;

	getmaxyx(header_window, y, x);
	if ((int)strlen(title) > (x-2))
		middle = 1;
	else
		middle = (x - strlen(title)) / 2;

	for (i = 1; i < x - 1; i++)
		mvwaddch(header_window, 1, i, ' ');
	char *temp = new char[strlen(title)+1];
	strcpy(temp, title);
	mvwaddnstr(header_window, 1, middle, temp, x - 2);
	delete temp;
	wrefresh(header_window);
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
	scrollWin
		*sw = group_stack->entry(current_group - 1);

	if (progmode != PM_FILESELECTION) /* or: !file_window */
		return;

	selitems = file_window->getSelectedItems(&nselected); 
	
	for ( i = 0; i < nselected; i++)
	{
		if (!is_mp3(selitems[i]))
			continue;

		char *path = file_window->getPath();
		char *itemname = (char *)malloc(strlen(selitems[i]) +
			strlen(path) + 2);
		if (path[strlen(path) - 1] != '/')
			sprintf(itemname, "%s/%s", path, selitems[i]);
		else
			sprintf(itemname, "%s%s", path, selitems[i]);
	
		add_selected_file(itemname);
		free(itemname);
		delete[] path;
	}

	if (nselected)
	{
		for (i = 0; i < nselected; i++)
			delete[] selitems[i];
		delete[] selitems;
	}

	if (selected_files)
	{
		for (i = 0; i < nselfiles; i++)
		{
			sw->addItem(selected_files[i]);
			free(selected_files[i]);
		}
		free(selected_files);
		selected_files = NULL;
		nselfiles = 0;
	}

	delete file_window;
	file_window = NULL;
	progmode = PM_NORMAL;

	sw->swRefresh(1);
	char 
		*title = sw->getTitle(),
		*dummy = new char[strlen(title) + 10];
	sprintf(dummy, "%02d:%s", current_group, title);
	set_header_title(dummy);
	delete[] dummy;
	delete[] title;

	set_default_fkeys(progmode);
	draw_extra_stuff();
}

void
fw_changedir(const char *newpath = 0)
{
	if (progmode != PM_FILESELECTION)
		return;

	int
		nselected = 0;
	char 
		*path,
		**selitems;

	if (!newpath)
		path = file_window->getSelectedItem();
	else
	{
		path = new char[strlen(newpath) + 1];
		strcpy(path, newpath);
	}

	selitems = file_window->getSelectedItems(&nselected);

	/* if not changed to current dir and files have been selected add them to
	 * the selection list 
	 */
	if (strcmp(path, ".") && nselected)
	{
		int i;

		for (i = 0; i < nselected; i++)
		{
			char
				*pwd = file_window->getPath(),
				*file = (char *)malloc((strlen(pwd) + strlen(selitems[i]) +
					2) * sizeof(char));

			strcpy(file, pwd);
			if (pwd[strlen(pwd) - 1] != '/')
				strcat(file, "/");
			strcat(file, selitems[i]);

			if (is_mp3(file))
				add_selected_file(file);

			free(file);
			delete[] pwd;
			delete[] selitems[i];
		}
		delete[] selitems;
	}
	
	file_window->changeDir(path);
	wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
	file_window->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS,
		ACS_BTEE, ACS_PLUS);
	wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(6));
	char *pruts = file_window->getPath();
	set_header_title(pruts);
	delete[] pruts;
	delete[] path;
}

/* PRE: Be in PM_FILESELECTION
 * POST: Returns 1 if path was succesfully changed into.
 */
short
fw_getpath()
{
	DIR
		*dir = NULL;
	char
		*newpath = gettext("Enter absolute path:");
	
	if (!newpath || !(dir = opendir(newpath)))
		return 0;
	closedir(dir);
	fw_changedir(newpath);
	free(newpath);
	return 1;
}

void
draw_extra_stuff(int file_mode)
{
	WINDOW *extra = newwin(2, 3, 0, COLS-3);
	wbkgd(extra, ' '|COLOR_PAIR(1)|A_BOLD);
	mvwaddch(extra, 0, 0, ACS_HLINE);
	mvwaddch(extra, 0, 1, ACS_HLINE);
	mvwaddch(extra, 0, 2, ACS_URCORNER);
	if (!file_mode)
		mvwaddch(extra, 1, 1, '-');
	else
		mvwaddch(extra, 1, 1, ' ');
	mvwaddch(extra, 1, 2, ACS_VLINE);
	leaveok(extra, TRUE);
	wrefresh(extra);
	delwin(extra);
	WINDOW *ex2 = newwin(2, 3, (LINES-2), (COLS-3));
	wbkgd(ex2, ' '|COLOR_PAIR(1)|A_BOLD);
	if (!file_mode)
		mvwaddch(ex2, 0, 1, '+');
	else
		mvwaddch(ex2, 0, 1, ' ');
	mvwaddch(ex2, 0, 2, ACS_VLINE);
	mvwaddch(ex2, 1, 0, ACS_HLINE);
	mvwaddch(ex2, 1, 1, ACS_HLINE);
	mvwaddch(ex2, 1, 2, ACS_LRCORNER);
	leaveok(ex2, TRUE);
	wrefresh(ex2);
	delwin(ex2);
}

void
refresh_screen()
{
	wclear(stdscr);
	touchwin(stdscr);
	touchwin(command_window);
	wnoutrefresh(command_window);
	touchwin(header_window);
	wnoutrefresh(header_window);
	touchwin(group_window->sw_win);
	wnoutrefresh(group_window->sw_win);
	touchwin(message_window);
	wnoutrefresh(message_window);
	if (progmode == PM_FILESELECTION)
	{
		touchwin(file_window->sw_win);
		wnoutrefresh(file_window->sw_win);
		draw_extra_stuff(1);
	}
	else if (progmode == PM_NORMAL)
	{
		scrollWin
			*sw = group_stack->entry(current_group - 1);
		touchwin(sw->sw_win);
		wnoutrefresh(sw->sw_win);
		draw_extra_stuff();
	}
	doupdate();
}

/* Now some Nifty(tm) message-windows */
void
Error(const char *txt)
{
	int
		offset,
		i, x, y;

	getmaxyx(message_window, y, x);

	offset = (x - 2 - strlen(txt));
	if (offset <= 1)
		offset = 1;
	else
		offset = offset / 2;

	/* 21 jan 1999: Hmm..what was this check for???? 
	if (offset > 40)
	{
		char a[100];
		sprintf(a, "offset=%d,strlen(txt)=%d",offset,strlen(txt));
		mw_settxt(a);
		wgetch(message_window);
		offset = 0;
	} */

	for (i = 1; i < x - 1; i++)
	{
		chtype tmp = '<';
		
		if (i < offset)
			tmp = '>';
		mvwaddch(message_window, 0, i, tmp);
	}

	char *temp = new char[strlen(txt)+1];
	strcpy(temp, txt);
	mvwaddnstr(message_window, 0, offset, temp, x - offset - 1);
	delete temp;

	mvwchgat(message_window, 0, 1, x - 2, A_BOLD, 3, NULL);
	wrefresh(message_window);
	wgetch(message_window);
	for (i = 1; i < x - 1; i++)
		mvwaddch(message_window, 0, i, ' ');
	
	wrefresh(message_window);
}

int
is_mp3(const char *filename)
{
	int len;
	
	if (!filename || (len = strlen(filename)) < 5)
		return 0;

	if (fnmatch(".[mM][pP][23]", (filename + (len - 4)), 0))
		return 0;
	//if (strcasecmp(filename + (len - 4), ".mp3"))
	//	return 0;
	
	return 1;
}

void
cw_set_fkey(short fkey, const char *desc)
{
	int
		maxy, maxx;
	char
		*fkeydesc = NULL;
		
	if (fkey < 1 || fkey > 12)
		return;
	
	/* y-pos of fkey-desc = fkey */
	
	getmaxyx(command_window, maxy, maxx);
	//fprintf(stderr, "(%d,%d)\n", maxy, maxx); fflush(stderr);
	
	fkeydesc = (char *)malloc((5 + strlen(desc) + 1) * sizeof(char));
	sprintf(fkeydesc, "F%2d: %s", fkey, desc);
	mvwaddnstr(command_window, fkey, 1, "                              ",
		(maxx - 1));
	if (strlen(desc))
		mvwaddnstr(command_window, fkey, 1, fkeydesc, maxx - 1);
	touchwin(command_window);
	wrefresh(command_window);
}

/* Selects group group_window->entry(groupnr - 1).
 * (i.e. groupnr - 1 == index of the groupstack)
 */
void
select_group(int groupnr)
{
	scrollWin
		*sw;

	if (!(group_window->getNitems())) //no groups yet (shouldn't happen)
		return;

	if (groupnr > (group_window->getNitems()))
		groupnr = 1;
	if (groupnr < 1)
		groupnr = group_window->getNitems();

	current_group = groupnr;

	/* update group_window & its contents */
	sw = group_stack->entry(current_group - 1);
	sw->swRefresh(1);
	touchwin(group_window->sw_win);
	group_window->setItem(current_group - 1);

	char
		*title = sw->getTitle(),
		*dummy = new char[strlen(title) + 10];

	sprintf(dummy, "%02d:%s", current_group, title);
	set_header_title(dummy);
	delete[] dummy;
	delete[] title;
	cw_toggle_group_mode(1); /* display this group's playmode */
}

/* Adds a new group to the end of the list, and returns the group's nr
 * (where group_stack[index] == group_stack[group's nr - 1])
 */
int
add_group(const char *newgroupname=0)
{
	char
		gnr[10];
	int
		ngroups = group_window->getNitems() + 1;
	scrollWin
		*sw;

	sw = new scrollWin(LINES - 4, COLS - 27, 2, 24, NULL, 0, 0, 1);
	sw->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS, ACS_BTEE, ACS_PLUS);
	wbkgd(sw->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
	if (newgroupname)
		sw->setTitle(newgroupname);
	else
		sw->setTitle("Default");
	group_stack->add(sw);

	sprintf(gnr, "%02d", ngroups);
	group_window->addItem(gnr);
	group_window->swRefresh(1);

	select_group(ngroups);
	return ngroups;
}

/* This function deletes group_stack->entry(gnr-1);
 */
short
del_group(int gnr)
{
	int
		ngroups = group_window->getNitems();

	if (ngroups < 2) /* 1 group is required to exist */
		return 0;
	if (!(group_stack->del(gnr - 1)))
		return 0;

	group_window->delItem(gnr - 1);
	ngroups--;

	//update group_window;
	for (int i = gnr; i <= ngroups; i++)
	{
		char newtitle[10];
		sprintf(newtitle, "%02d", i);
		group_window->changeItem(i - 1, newtitle);
	}

	group_window->swRefresh(2);
	if ((unsigned)current_group > group_stack->stackSize())
		current_group--; /* last entry was deleted */
	select_group(current_group);

	return 1;	
}

void
set_group_name(scrollWin *sw)
{
	WINDOW
		*gname = newwin(5, 64, (LINES - 5) / 2, (COLS - 64) / 2);
	char
		name[49], tmpname[55];

	leaveok(gname, TRUE);
	keypad(gname, TRUE);
	wbkgd(gname, COLOR_PAIR(3)|A_BOLD);
	wborder(gname, 0, 0, 0, 0, 0, 0, 0, 0);

	mvwaddstr(gname, 2, 2, "Enter name:");
	mvwchgat(gname, 2, 14, 48, A_BOLD, 5, NULL);
	touchwin(gname);
	wrefresh(gname);
	wmove(gname, 2, 14);
	echo();
	wattrset(gname, COLOR_PAIR(5)|A_BOLD);
	wgetnstr(gname, name, 48);
	noecho();

	sprintf(tmpname, "%02d:%s", current_group, name);	
	sw->setTitle(name);
	if (sw == group_stack->entry(current_group - 1))
		set_header_title(tmpname);

	delwin(gname);
	refresh_screen();
}

void
mw_clear()
{
	int i, x, y;
	getmaxyx(message_window, y, x);
	
	for (i = 1; i < (x-1); i++)
		mvwaddch(message_window, 0, i, ' ');
	wrefresh(message_window);
}

void
mw_settxt(const char *txt)
{
	int y, x;
	mw_clear();

	getmaxyx(message_window, y, x);
	char *temp = new char[strlen(txt)+1];
	strcpy(temp, txt);
	mvwaddnstr(message_window, 0, 1, temp, x - 2);
	delete temp;

	wrefresh(message_window);
}

/* PRE: f is open 
 * Function returns NULL when EOF(f) else the next line read from f, terminated
 * by \n. The line is malloc()'d so don't forget to free it ..
 */
char *
readline(FILE *f)
{
	short
		not_found_endl = 1;
	char *
		line = NULL;

	while (not_found_endl)
	{
		char tmp[256];
		char *index;
		
		if ( !(fgets(tmp, 255, f)) )
			break;

		index = (char*)rindex(tmp, '\n');
		
		if (index && strlen(index) == 1) /* terminating \n found */
			not_found_endl = 0;

		if (!line)	
		{
			line = (char*)malloc(sizeof(char));
			line[0] = '\0';
		}
			
		line = (char*)realloc(line, (strlen(line) + strlen(tmp) + 1) *
			sizeof(char));
		strcat(line, tmp);
	}

	if (line && line[strlen(line) - 1] != '\n') /* no termin. \n! */
	{
		line = (char*)realloc(line, (strlen(line) + 2) * sizeof(char));
		strcat(line, "\n");
	}

	return line;
}

/* PRE: f is opened
 * POST: if a group was successfully read from f, non-zero will be returned
 *       and the group is added.
 */
int
read_group_from_file(FILE *f)
{
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
				success = sscanf(line, "GROUPNAME: %[^\n]s", tmp);	

			if (success < 1) /* bad group-header! */
				read_ok = 0;
			else /* valid group - add it */
			{
				char
					*title;

				group_ok = 1;
				sw = group_stack->entry(current_group - 1);

				/* if there are no entries in the current group, the first
				 * group will be read into that group.
				 */
				if (sw->getNitems())
				{	
					add_group();
					sw = group_stack->entry(group_stack->stackSize() - 1);
				}
				title = (char*)malloc((strlen(tmp) + 16) * sizeof(char));
				sprintf(title, "%02d:%s", current_group, tmp);
				sw->setTitle(tmp);
				sw->swRefresh(0);
				set_header_title(title);
				free(title);
			}

			free(tmp);
		}
		else if (linecount == 1) /* read play order from file */
		{
			char
				*tmp = new char[strlen(line) + 1];
			int
				success = sscanf(line, "PLAYMODE: %[^\n]s", tmp);	

			if (success < 1) /* bad group-header! */
			{
				read_ok = 0; group_ok = 0;
			}
			else
				sw->sw_playmode = (atoi(tmp) ? 1 : 0);

			delete[] tmp;
		}
		else
		{
			if (!strcmp(line, "\n")) /* line is empty, great. */
				read_ok = 0;
			else
			{
				char *songname = new char[strlen(line) + 1];
					
				sscanf(line, "%[^\n]s", songname);

				if (is_mp3(songname))
					sw->addItem(songname);

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
}

/* PRE : txt = NULL, An inputbox asking for a filename will be given to the
 *       user.
 * Post: txt contains text entered by the user. txt is malloc()'d so don't
 *       forget to free() it.
 * TODO: change it so question/max txt-length can be given as parameter
 */
char *
gettext(const char *label, short display_path)
{
	WINDOW
		*gname;
	char
		name[53],
		*txt = NULL;
	short
		y_offset = (display_path ? 1 : 0);

	gname = newwin(5 + y_offset, 69, (LINES - 6) / 2, (COLS - 69) / 2);

	leaveok(gname, TRUE);

	keypad(gname, TRUE);
	wbkgd(gname, COLOR_PAIR(4)|A_BOLD);
	wborder(gname, 0, 0, 0, 0, 0, 0, 0, 0);

	if (display_path)
	{
		char
			pstring[strlen(startup_path) + 7];

		strcpy(pstring, "Path: ");
		strcat(pstring, startup_path);
		mvwaddnstr(gname, 2, 2, pstring, 65);
	}

	mvwaddstr(gname, 2 + y_offset, 2, label);
	mvwchgat(gname, 2 + y_offset, 3 + strlen(label),
		67 - (strlen(label)+3), A_BOLD, 5, NULL);
	touchwin(gname);
	wrefresh(gname);
	wmove(gname, 2 + y_offset, 3 + strlen(label));
	echo();
	wattrset(gname, COLOR_PAIR(5)|A_BOLD);
	wgetnstr(gname, name, 48);
	noecho();
	delwin(gname);
	refresh_screen();
	txt = (char*)malloc((strlen(name) + 1) * sizeof(char));
	strcpy(txt, name);
	return txt;
}

void
read_playlist(const char *filename)
{
	FILE
		*f;
	char 
		*name;
		
	if(!filename)
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
		//fprintf(stderr, "New path: %s\n", name);
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
			old_mode = play_mode;

		if (!line)
		{
			fclose(f); return;
		}

		char tmp[strlen(line)+1];
		if ( sscanf(line, "GLOBALMODE: %[^\n]s", tmp) == 1)
		{
			if (!strcmp(tmp, "onegroup"))
				play_mode = PLAY_GROUP;
			else if (!strcmp(tmp, "allgroups"))
				play_mode = PLAY_GROUPS;
			else if (!strcmp(tmp, "groupsrandom"))
				play_mode = PLAY_GROUPS_RANDOMLY;
			else if (!strcmp(tmp, "allrandom"))
				play_mode = PLAY_SONGS;
			else
				header_ok = 0;
		}
		else
			header_ok = 0;

		if (header_ok && old_mode != play_mode)
			cw_toggle_play_mode(1);
		free(line);
	}	
		
	if (header_ok)
	{
		while (read_group_from_file(f))
			;
		cw_toggle_group_mode(1);
	}
	else
	{
		warning("No global playmode in playlist found.");
		refresh_screen();
	}	

	fclose(f);
	//mw_settxt("Added playlist!");

#if 0
	/* Why the F*CK did I ever add this crap??????????????? */
	/* this code is ugly, because the way groups are added is ugly.. */
	current_group = group_stack->stackSize();
	group_stack->entry(current_group - 1)->swRefresh(2);
	group_window->changeSelection(group_window->getNitems() - 1 - 
		group_window->sw_selection);
	group_window->swRefresh(2);
	char *bla = group_stack->entry(current_group - 1)->getTitle();
	char *title = new char[strlen(bla)+4];
	sprintf(title, "%02d:%s", current_group, bla);
	set_header_title(title);
	delete[] bla;
	delete[] title;
#endif
}

void
write_playlist()
{
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

	if (!(group_stack->writePlaylist(name, play_mode)))
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
}

void
cw_toggle_group_mode(short notoggle)
{
	char
		*modes[] = {
			"Play in normal order",
			"Play in random order"
			};
	scrollWin
		*sw = group_stack->entry(current_group - 1);
	int
		i, maxy, maxx;

	if (!notoggle)
		sw->sw_playmode = 1 - sw->sw_playmode; /* toggle between 0 and 1 */

	getmaxyx(command_window, maxy, maxx);
	
	for (i = 1; i < maxx - 1; i++)
		mvwaddch(command_window, 14, i, ' ');

	mvwaddstr(command_window, 14, 1, modes[sw->sw_playmode]);
	wrefresh(command_window);
}

void
cw_toggle_play_mode(short notoggle)
{
	unsigned int
		i,
		maxy, maxx;
	char
		*desc;

	if (!notoggle)
	{
		switch(play_mode)
		{
			case PLAY_GROUP:  play_mode = PLAY_GROUPS; break;
			case PLAY_GROUPS: play_mode = PLAY_GROUPS_RANDOMLY; break;
			case PLAY_GROUPS_RANDOMLY: play_mode = PLAY_SONGS; break;
			case PLAY_SONGS: play_mode = PLAY_GROUP; break;
			default: play_mode = PLAY_GROUP;
		}
	}

	getmaxyx(command_window, maxy, maxx);
	
	for (i = 1; i < maxx - 1; i++)
	{	
		mvwaddch(command_window, 17, i, ' ');
		mvwaddch(command_window, 18, i, ' ');
	}

	desc = playmodes_desc[play_mode];

	if (strlen(desc) > (maxx - 2))
	{
		/* desc must contain a space such that the text before that space fits
		 * in command-window's width, and the text after that spce as well.
		 * Otherwise this algorithm will fail miserably!
		 */
		char
			*desc2 = (char *)malloc( (maxx) * sizeof(char)),
			*desc3;
		int index;
		
		strncpy(desc2, desc, maxx - 1);
		desc2[maxx - 1] = '\0'; /* maybe strncpy already does this */
		desc3 = strrchr(desc2, ' ');
		index = strlen(desc) - (strlen(desc3) + (strlen(desc) - 
			strlen(desc2))) + 1;
		mvwaddnstr(command_window, 17, 1, desc, index - 1);
		mvwaddstr(command_window, 18, 1, desc + index);
		free(desc2);
	}
	else
		mvwaddstr(command_window, 17, 1, playmodes_desc[play_mode]);
	wrefresh(command_window);
}

void
play_list()
{
	unsigned int
		nmp3s = 0;
	char
		**mp3s = group_stack->getShuffledList(play_mode, &nmp3s);
	program_mode
		oldprogmode = progmode;

	if (!nmp3s)
		return;

	progmode = PM_MP3PLAYING;
	set_default_fkeys(progmode);
	//mw_settxt("Use 'q' to return to the playlist-editor.");
	mp3Play
		*player = new mp3Play((const char**)mp3s, nmp3s);
#ifdef PTHREADEDMPEG
	player->setThreads(threads);
#endif
	player->playMp3List();
	progmode = oldprogmode;
	set_default_fkeys(progmode);
	refresh_screen();
	return;
}

void
play_one_mp3(const char *filename)
{
	if (!filename || !is_mp3(filename))
	{
		warning("Invalid filename (should end with .mp[23])");
		refresh_screen();
		return;
	}

	char
		**mp3s = new char*[1];
	program_mode
		oldprogmode = progmode;

	mp3s[0] = new char[strlen(filename) + 1];
	strcpy(mp3s[0], filename);
	progmode = PM_MP3PLAYING;
	set_default_fkeys(progmode);
	//mw_settxt("Use 'q' to return to the playlist-editor.");
	mp3Play
		*player = new mp3Play((const char **)mp3s, 1);
#ifdef PTHREADEDMPEG
	player->setThreads(threads);
#endif
	player->playMp3List();
	progmode = oldprogmode;
	set_default_fkeys(progmode);
	refresh_screen();
	return;
}

void
set_default_fkeys(program_mode peem)
{
	switch(peem)
	{
	case PM_NORMAL:
		cw_set_fkey(1, "Select files");
		cw_set_fkey(2, "Add group");
		cw_set_fkey(3, "Delete group");
		cw_set_fkey(4, "Set group's title");
		cw_set_fkey(5, "load/add playlist");
		cw_set_fkey(6, "Write playlist");
		cw_set_fkey(7, "Toggle group mode");
		cw_set_fkey(8, "Toggle play mode");
		cw_set_fkey(9, "Play list");
#ifdef PTHREADEDMPEG
		cw_set_fkey(10, "Change #threads");
#else
		cw_set_fkey(10, "");
#endif
		cw_set_fkey(11, "");
		cw_set_fkey(12, "");
		break;
	case PM_FILESELECTION:
		cw_set_fkey(1, "Add files to group");
		cw_set_fkey(2, "Invert selection");
		cw_set_fkey(3, "Recurs. select all");
		cw_set_fkey(4, "Enter pathname");
		cw_set_fkey(5, "Add dirs as groups");
		cw_set_fkey(6, "");
		cw_set_fkey(7, "");
		cw_set_fkey(8, "");
		cw_set_fkey(9, "");
		cw_set_fkey(10, "");
		cw_set_fkey(11, "");
		cw_set_fkey(12, "");
		break;
	case PM_MP3PLAYING:
		cw_set_fkey(1, "");
		cw_set_fkey(2, "");
		cw_set_fkey(3, "");
		cw_set_fkey(4, "");
		cw_set_fkey(5, "");
		cw_set_fkey(6, "");
		cw_set_fkey(7, "");
		cw_set_fkey(8, "");
		cw_set_fkey(9, "");
		cw_set_fkey(10, "");
		cw_set_fkey(11, "");
		cw_set_fkey(12, "");
		break;
	}
}

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

int
is_symlink(const char *path)
{
	struct stat
		*buf = (struct stat*)malloc( 1 * sizeof(struct stat) );
	
	if (lstat(path, buf) < 0)
		return 0;
	
	if ( ((buf->st_mode) & S_IFMT) != S_IFLNK) /* not a symlink. Yippee */
		return 0;
	
	return 1;
}

/* This function adds all mp3s recursively found in ``path'' to the current
 * group when called by F3: recursively add mp3's. 
 * When called by F4: Add dirs as groups, and d2g_init is not set, all mp3's
 * found in this dir will be added to a group with the name of the dir.
 */
void
recsel_files(const char *path, short d2g=0, int d2g_init=0)
{
	struct dirent
		*entry = NULL;
	DIR
		*dir = NULL;
	char
		*txt = NULL,
		header[] = "Recursively selecting in: ";
	short 
		mp3_found = 0;
	int
		d2g_groupindex = -1;

	if (progmode != PM_FILESELECTION || !strcmp(path, "/dev") ||
		is_symlink(path))
		return;

	if ( !(dir = opendir(path)) )
	{
		/* fprintf(stderr, "Error opening directory!\n");
		 * exit(1);
		 */
		 //Error("Error opening directory!");
		 return;
	}

	txt = (char *)malloc((strlen(path) + strlen(header) + 1) * sizeof(char));
	strcpy(txt, header);
	strcat(txt, path);
	mw_settxt(txt);
	//Error(txt);
	free(txt);

	while ( (entry = readdir(dir)) )
	{
		DIR *dir2 = NULL;
		char *newpath = (char *)malloc((entry->d_reclen + 2 + strlen(path)) *
			sizeof(char));

		strcpy(newpath, path);
		if (path[strlen(path) - 1] != '/')
			strcat(newpath, "/");
		strcat(newpath, entry->d_name);

		if ( (dir2 = opendir(newpath)) )
		{
			char *dummy = entry->d_name;

			closedir(dir2);

			if (!strcmp(dummy, ".") || !strcmp(dummy, ".."))
				continue; /* next while-loop */
		
			recsel_files(newpath, d2g);
		}
		else if (is_mp3(newpath))
		{
			if (d2g && !d2g_init)
			{
				if (!mp3_found)
				{
					char *npath = 0;
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
					/* If there are no entries in the current group,
					 * this group will be filled instead of skipping it
					 */
					scrollWin *sw = group_stack->entry(current_group - 1);
					if (sw->getNitems())
						d2g_groupindex = (add_group(npath) - 1);
					else
					{
						d2g_groupindex = current_group - 1;
						sw->setTitle(npath);
					}
					mp3_found = 1;
				}
				if (d2g_groupindex > -1 && group_stack->stackSize() >
					(unsigned int)d2g_groupindex)
					(group_stack->entry(d2g_groupindex))->addItem(newpath);
				else
				{
					warning("Something's screwy with recsel_files");
					refresh_screen();
				}
			}
			if (!d2g)
				add_selected_file(newpath);
		}

		free(newpath);
	}
	
	closedir(dir);
}

#ifdef PTHREADEDMPEG
void
change_threads()
{
	if ( (threads += 50) > 500)
		threads = 0;
	mvwprintw(command_window, 20, 1, "Threads: %03d", threads);
	wrefresh(command_window);
}
#endif

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

/* handle_input reads a key from the keyboard. If no_delay is non-zero,
 * the program will not wait for the user to press a key.
 * RETURNS -1 when (in delay-mode) the program is to be terminated or (in
 * non_delay-mode) when no key has been pressed, 0 if everything went OK
 * and no further actions need to be taken and >0 return the value of the
 * read key for further use by the function which called this one.
 */
int
handle_input(short no_delay)
{
	int
		key,
		retval = 0;
	WINDOW
		*tmp = NULL;

	if (progmode == PM_FILESELECTION)
		tmp = file_window->sw_win;
	else if (progmode == PM_NORMAL)
		tmp = (group_stack->entry(current_group - 1))->sw_win;

	if (no_delay)
	{
		nodelay(tmp, TRUE);
		key = wgetch(tmp);
		nodelay(tmp, FALSE);
	}
	else
	{
		nodelay(tmp, FALSE);
		key=wgetch(tmp);
	}

	switch(progmode)
	{
	case PM_NORMAL:
	{
		scrollWin 
			*sw = group_stack->entry(current_group - 1);
		switch(key)
		{
			case 'q': retval = -1; break;
			case 'h': show_help(); break;
			case KEY_DOWN: sw->changeSelection(1); break;
			case KEY_UP: sw->changeSelection(-1); break;
			case 'd':
			case KEY_DC: sw->delItem(sw->sw_selection); break;
			case KEY_NPAGE: sw->pageDown(); break;
			case KEY_PPAGE: sw->pageUp(); break;
			case ' ': sw->selectItem(); sw->changeSelection(1); break;
			case '+':
				select_group(current_group + 1); break; //select next group
			case '-':
				select_group(current_group - 1); break; //select prev. group
			case 12: refresh_screen(); break; // C-l
			case 13: // play 1 mp3
				if (sw->getNitems() > 0)
				{
				 	char *file = sw->getSelectedItem();
					play_one_mp3(file);
					delete[] file;
				}
				break;
			case KEY_F(1): /* file-selection */
			case '1':
			{
				progmode = PM_FILESELECTION;
				set_default_fkeys(progmode);
				if (file_window)
					delete file_window;
				file_window = new fileManager(NULL, LINES - 4, COLS - 27,
					2, 24, 1, 1);
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
				file_window->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS,
					ACS_BTEE, ACS_PLUS);
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(6));
				file_window->swRefresh(0);
				char *pruts = file_window->getPath();
				set_header_title(pruts);
				draw_extra_stuff(1);
				delete[] pruts;
			}
			break;
			case KEY_F(2): case '2': 
				add_group(); break; //add group
			case KEY_F(3): case '3':
				del_group(current_group); break; //del group
				break;
			case KEY_F(4): case '4':
				set_group_name(sw); break; // change groupname
			case KEY_F(5): case '5':
				read_playlist((const char*)NULL); break; // read playlist
			case KEY_F(6): case '6':
				write_playlist(); break; // write playlist
			case KEY_F(7): case '7':
				cw_toggle_group_mode(0); break; 
			case KEY_F(8): case '8':
				cw_toggle_play_mode(0); break;
			case KEY_F(9): case '9':
				play_list(); break;
#ifdef PTHREADEDMPEG
			case KEY_F(10): case '0':
				change_threads(); break;
#endif
#ifdef DEBUG
			case 'c': sw->addItem("Hoei.mp3"); sw->swRefresh(1); break;
			case 'f': fprintf(stderr, "Garbage alert!"); break;
#endif
		}
	}
	break;
	case PM_FILESELECTION:
	{
		fileManager
			*sw = file_window;
		switch(key)
		{
			case 'q': retval = -1; break;
			case 'h': show_help(); break;
			case KEY_DOWN: sw->changeSelection(1); break;
			case KEY_UP: sw->changeSelection(-1); break;
			case KEY_NPAGE: sw->pageDown(); break;
			case KEY_PPAGE: sw->pageUp(); break;
			case ' ': sw->selectItem(); sw->changeSelection(1); break;
			case 12: refresh_screen(); break; // C-l
			case 13:
				if (sw->isDir(sw->sw_selection))
					fw_changedir();
				else if (sw->getNitems() > 0)
				{
					char *file = sw->getSelectedItem();
					play_one_mp3(file);
					delete[] file;
				}
				break;
			case KEY_F(1): case '1':
				fw_end(); break;
			case KEY_F(2): case '2':
				sw->invertSelection(); break;
			case KEY_F(3): case '3':
			{
				mw_settxt("Recursively selecting files..");
				char *tmppwd = get_current_working_path();
				recsel_files(tmppwd);
				free(tmppwd);
				fw_end();
				mw_settxt("Added all mp3's in this dir and all subdirs " \
					"to current group.");
			}
				break;
			case KEY_F(4): case '4':
				fw_getpath();
				break;
			case KEY_F(5): case '5':
				mw_settxt("Adding groups..");
				char *tmppwd = get_current_working_path();
				recsel_files(tmppwd, 1, 1);
				free(tmppwd);
				fw_end();
				mw_settxt("Added dirs as  groups.");
				break;
		}
	}
	break;
	case PM_MP3PLAYING:
		break;
	}
	
	return retval;
}

void
show_help()
{
	short h=(COLS/2)+1;
	WINDOW *helpwin = NULL;

	wclear(stdscr);
	touchwin(stdscr);
	helpwin = stdscr;
	wborder(helpwin, 0, 0, 0, 0, 0, 0, 0, 0);
	/* Playlist Editor keys */
	mvwaddstr(helpwin, 1, (COLS-15)/2, "Playlist Editor");
	mvwaddstr(helpwin, 2,  1, "F1/1  : Enter file browser");
	mvwaddstr(helpwin, 3,  1, "F2/2  : Add a group to playlist");
	mvwaddstr(helpwin, 4,  1, "F3/3  : Delete group from playlist");
	mvwaddstr(helpwin, 5,  1, "F4/4  : Set current group's title");
	mvwaddstr(helpwin, 6,  1, "F5/5  : Load/Add playlist");
	mvwaddstr(helpwin, 7,  1, "F6/6  : Write playlist");
	mvwaddstr(helpwin, 8,  1, "Space : Select MP3 (useless)");
	mvwaddstr(helpwin, 9,  1, "q     : Exit Mp3blaster");
	mvwaddstr(helpwin, 2,  h, "F7/7  : Toggle this group's play order");
	mvwaddstr(helpwin, 3,  h, "F8/8  : Toggle global play order");
	mvwaddstr(helpwin, 4,  h, "F9/9  : Enter Playing Mode");
	mvwaddstr(helpwin, 5,  h, "F10/0 : Change threads (buffering)");
	mvwaddstr(helpwin, 6,  h, "Enter : Play only highlighted MP3");
	mvwaddstr(helpwin, 7,  h, "Del/d : Remove hilited MP3 from group");
	mvwaddstr(helpwin, 8,  h, "+/-   : Select Next/Previous group");
	mvwaddstr(helpwin, 9,  h, "h     : This screen(exit by keypress)");
	
	/* File Browser keys */
	mvwaddstr(helpwin, 11, (COLS-12)/2, "File Browser");
	mvwaddstr(helpwin, 12,  1, "F1/1  : Add select files to group");
	mvwaddstr(helpwin, 13,  1, "        and return to Playlist Editor");
	mvwaddstr(helpwin, 14,  1, "F2/2  : Invert current selection");
	mvwaddstr(helpwin, 15,  1, "F3/3  : Recursively select&add MP3's");
	mvwaddstr(helpwin, 12,  h, "F4/4  : Enter pathname to change to");
	mvwaddstr(helpwin, 13,  h, "F5/5  : Add subdirs as groups");
	mvwaddstr(helpwin, 14,  h, "Enter : (if on dir) change directory");
	mvwaddstr(helpwin, 15,  h, "q     : Exit Mp3blaster");

	/* Play Mode keys */
	mvwaddstr(helpwin, 17, (COLS-12)/2, "Playing Mode");
	mvwaddstr(helpwin, 18,  1, "1..6  : CD-Style playlist control");
	mvwaddstr(helpwin, 19,  1, "Arrows: Operates Mixer (if enabled)");
	mvwaddstr(helpwin, 20,  1, "Space : Play next MP3 (same as '5')");
	mvwaddstr(helpwin, 18,  h, "q     : Return to Playlist Editor");

	mvwaddstr(helpwin, LINES-2, (COLS-24)/2, "Press a key to exit help");

	wrefresh(helpwin);
	getch();
	//delwin(helpwin);
	refresh_screen();
	return;
}
