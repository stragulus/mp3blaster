/* Copyright (C) Bram Avontuur (brama@stack.nl)
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
#include <sys/time.h>
#include <regex.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "global.h"
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
extern short cf_parse_config_file(const char *);
extern cf_error cf_get_error();
extern const char *cf_get_error_string();

/* Prototypes */
void set_header_title(const char*);
void refresh_screen(void);
void Error(const char*);
int is_mp3(const char*);
int is_sid(const char *);
int is_audiofile(const char *);
int handle_input(short);
void cw_set_fkey(short, const char*);
void set_default_fkeys(program_mode pm);
void cw_toggle_group_mode(short notoggle);
void cw_draw_play_mode();
short set_play_mode(const char*);
void cw_toggle_play_mode();
void mw_clear();
void mw_settxt(const char*);
void read_playlist(const char *);
void write_playlist();
void play_list();
#ifdef PTHREADEDMPEG
void change_threads(int dont=0);
short set_threads(int);
#endif
void add_selected_file(const char*);
char *get_current_working_path();
void usage();
void show_help();
void draw_extra_stuff(int file_mode=0);
void draw_settings(int cleanit=0);
char *gettext(const char *label, short display_path=0);
short fw_getpath();
void fw_convmp3();
void fw_addurl();
void fw_search_next_char(char);
void fw_start_search(int timeout=2);
void fw_end_search();
void set_sound_device(const char *);
short set_fpl(int);

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

/* global vars */

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
	nselfiles = 0,
	fw_searchmode = 0; /* non-zero if searchmode during file selection is on */
FILE
	*debug_info = NULL; /* filedescriptor of debug-file. */
struct _globalopts
	globalopts;

char
	*playmodes_desc[] = {
	"",									/* PLAY_NONE */
	"Play current group",				/* PLAY_GROUP */
	"Play all groups in normal order",	/* PLAY_GROUPS */
	"Play all groups in random order",	/* PLAY_GROUPS_RANDOMLY */
	"Play all songs in random order"	/* PLAY_SONGS */
	},
	**selected_files = NULL,
	*startup_path = NULL,
	*fw_searchstring,
	**extensions = NULL;

int
main(int argc, char *argv[])
{
	int
		c,
		long_index,
		key,
		options = 0,
		playmp3s_nr = 0;
	char
		*grps[] = { "01" },
		*dummy = NULL,
		**playmp3s = NULL,
		*init_playlist = NULL,
		*chroot_dir = NULL;
	scrollWin
		*sw;
	struct _tmp
	{
		char *play_mode;
		char *sound_device;
		int fpl;
#ifdef PTHREADEDMPEG
		int threads;
#endif
	} tmp;
	tmp.sound_device = NULL;
	tmp.play_mode = NULL;

	long_index = 0;
	globalopts.no_mixer = 0; /* mixer on by default */
	globalopts.fpl = 5; /* 5 frames are played between input handling */
	globalopts.sound_device = NULL; /* default sound device */
	globalopts.downsample = 0; /* no downsampling */
	globalopts.eightbits = 0; /* 8bits audio off by default */
	globalopts.fw_sortingmode = 0; /* case insensitive dirlist */
	globalopts.play_mode = PLAY_GROUPS;
	globalopts.warndelay = 2; /* wait 2 seconds for a warning to disappear */
#ifdef PTHREADEDMPEG
	globalopts.threads = 100;
#endif

	/* parse arguments */
	while (1)
	{
#if 1
		static struct option long_options[] = 
		{
			{ "8bits", 0, 0, '8'},
			{ "autolist", 1, 0, 'a'},
			{ "chroot", 1, 0, 'c'},
			{ "debug", 0, 0, 'd'},
			{ "downsample", 0, 0, '2'},
			{ "help", 0, 0, 'h'},
			{ "list", 1, 0, 'l'},
			{ "no-mixer", 0, 0, 'n'},
			{ "playmode", 1, 0, 'p'},
			{ "no-quit", 0, 0, 'q'},
			{ "runframes", 1, 0, 'r'},
			{ "sound-device", 1, 0, 's'},
			{ "threads", 1, 0, 't'},
			{ 0, 0, 0, 0}
		};
		
		c = getopt_long_only(argc, argv, "28a:c:dhl:np:qr:s:t:", long_options,
			&long_index);
#else
		c = getopt(argc, argv, "a:c:dhl:np:qr:s:");
#endif

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
		case 'd':
			options |= OPT_DEBUG;
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
			options |= OPT_PLAYMODE;
			tmp.play_mode = new char[strlen(optarg)+1];
			strcpy(tmp.play_mode, optarg);
			break;
		case 'q': /* don't quit after playing 1 mp3/playlist */
			options |= OPT_QUIT;
			break;
		case 'r': /* numbers of frames to decode in 1 loop 1<=fpl<=10 */
			options |= OPT_FPL;
			tmp.fpl = atoi(optarg);
			break;
		case 's': /* sound-device other than the default /dev/{dsp,audio} ? */
			options |= OPT_SOUNDDEV;
			if (tmp.sound_device)
				delete[] tmp.sound_device;
			tmp.sound_device = new char[strlen(optarg)+1];
			strcpy(tmp.sound_device, optarg);
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
		case 't': /* threads */
			options |= OPT_THREADS;
#ifdef PTHREADEDMPEG
			tmp.threads = atoi(optarg);
#endif
			break;
		default:
			usage();
		}
	}

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

	if (options & OPT_DEBUG) 
	{
		char *homedir = get_homedir(NULL);
		if (!homedir)
		{
			fprintf(stderr, "What, you don't have a homedir!? No debugging " \
				"info for you!\n");
			fflush(stderr);
		}
		else
		{
			const char flnam[] = ".mp3blaster";
			char *to_open = (char*)malloc((strlen(homedir) + strlen(flnam) +
				2) * sizeof(char));
			sprintf(to_open, "%s/%s", homedir, flnam);
			if (!(debug_info = fopen(to_open, "a")))
			{
				fprintf(stderr, "Couldn't open debuginfo-file!\n");
				fflush(stderr);
			}
			free(to_open);
			debug("Debugging messages enabled. Hang on to yer helmet!\n");
		}
	}

	//read .mp3blasterrc
	if (!cf_parse_config_file(NULL) && cf_get_error() != NOSUCHFILE)
	{
			fprintf(stderr, "%s\n", cf_get_error_string());
			exit(1);
	}

	signal(SIGALRM, SIG_IGN);
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
	globalopts.current_group = 1; /* == group_stack->entry(current_group - 1) */

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
	mw_settxt("Press '?' to get a screen with key functions.");
	wrefresh(message_window);

	progmode = PM_NORMAL;
	draw_settings();

	if (options & OPT_NOMIXER)
	{
		globalopts.no_mixer = 1;
	}
	if (options & OPT_SOUNDDEV)
	{
		set_sound_device(tmp.sound_device);
		delete[] tmp.sound_device;
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

	if (options & OPT_DOWNSAMPLE)
		globalopts.downsample = 1;
	if (options & OPT_8BITS)
		globalopts.eightbits = 1;

/*\
|*|  very rude hack caused by nasplayer lib that outputs rubbish to stderr
\*/
	//fclose(stderr);
/*\
|*|  EORH
\*/

	if (options & OPT_PLAYMP3)
	{
		for (int i = 0; i < (argc - optind); i++)
		{
			sw->addItem(playmp3s[i]);
			delete[] playmp3s[i];
		}
		delete[] playmp3s; playmp3s_nr = 0;
		sw->swRefresh(0);
		play_list();

		if ( !(options & OPT_QUIT))
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
			if (!(options & OPT_QUIT))
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
 *              : Program must NOT be in ncurses-mode when this function is
 *              : called.
 */
void
usage()
{
	fprintf(stderr, "Mp3blaster v%s (C)1997 - 1999 Bram Avontuur.\n%s", \
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
		"\t--help/-h: This help screen.\n"\
		"\t--no-mixer/-n: Don't start the built-in mixer.\n"\
		"\t--debug/-d: Log debug-info in $HOME/.mp3blaster.\n"\
		"\t--downsample/-2: Downsample (44->22Khz etc)\n"\
		"\t--8bits/-8: 8bit audio (autodetected if your card can't handle 16 bits)\n"\
		"\t--chroot/-c=<rootdir>: Set <rootdir> as mp3blaster's root dir.\n"\
		"\t\tThis affects *ALL* file operations in mp3blaster!!(including\n"\
		"\t\tplaylist reading&writing!) Note that only users with uid 0\n"\
		"\t\tcan use this option (yet?). This feature will change soon.\n"\
		"\t--playmode/-p={onegroup,allgroups,groupsrandom,allrandom}\n"\
		"\t\tDefault playing mode is resp. Play first group only, Play\n"\
		"\t\tall groups in given order(default), Play all groups in random\n"\
		"\t\torder, Play all songs in random order.\n"\
		"\t--quit/-q: Don't quit after playing mp3[s] (only makes sense in \n"\
		"\t\tcombination with --autolist or files from command-line)\n"\
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
		"");

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
		*sw = group_stack->entry(globalopts.current_group - 1);

	if (progmode != PM_FILESELECTION) /* or: !file_window */
		return;

	selitems = file_window->getSelectedItems(&nselected); 
	
	for ( i = 0; i < nselected; i++)
	{
		if (!is_audiofile(selitems[i]) || is_dir(selitems[i]))
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
	sprintf(dummy, "%02d:%s", globalopts.current_group, title);
	set_header_title(dummy);
	delete[] dummy;
	delete[] title;

	set_default_fkeys(progmode);
	draw_extra_stuff();
	draw_settings();
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

			if (is_audiofile(file))
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
		*npath = gettext("Enter absolute path:"),
		*newpath;
	
	if (!npath || !(newpath = expand_path(npath)))
		return 0;

	free(npath);

	if ( !(dir = opendir(newpath))) 
	{
		free(newpath);
		return 0;
	}
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
			*sw = group_stack->entry(globalopts.current_group - 1);
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

int
is_sid(const char *filename)
{
#ifdef HAVE_SIDPLAYER
	char *ext = strrchr(filename, '.');
	if (ext) {
		if (!strcasecmp(ext, ".psid")) return 1;
		if (!strcasecmp(ext, ".sid")) return 1;
		if (!strcasecmp(ext, ".dat")) return 1;
		if (!strcasecmp(ext, ".inf")) return 1;
		if (!strcasecmp(ext, ".info")) return 1;
	}
#else
	if(filename); //prevent warning
#endif
	return 0;
}

int
is_httpstream(const char *filename)
{
	if (!strncasecmp(filename, "http://", 7))
		return 1;
	return 0;
}

int
is_audiofile(const char *filename)
{
	if (extensions)
	{
		int i = 0;
		while (extensions[i] != NULL)
		{
			int doesmatch = 0;
			regex_t dum;
			regcomp(&dum, extensions[i], 0);
			doesmatch = regexec(&dum, filename, 0, 0, 0);
			regfree(&dum);
			if (!doesmatch) //regexec returns 0 for a match.
				return 1;
			i++;
		}
		return 0;
	}
	return (is_mp3(filename) || is_sid(filename) || is_httpstream(filename));
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
	free(fkeydesc); /* thanks to Steven Kemp for pointing out I forgot this */
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

	globalopts.current_group = groupnr;

	/* update group_window & its contents */
	sw = group_stack->entry(globalopts.current_group - 1);
	sw->swRefresh(1);
	touchwin(group_window->sw_win);
	group_window->setItem(globalopts.current_group - 1);

	char
		*title = sw->getTitle(),
		*dummy = new char[strlen(title) + 10];

	sprintf(dummy, "%02d:%s", globalopts.current_group, title);
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
	if ((unsigned)globalopts.current_group > group_stack->stackSize())
		globalopts.current_group--; /* last entry was deleted */
	select_group(globalopts.current_group);

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

	sprintf(tmpname, "%02d:%s", globalopts.current_group, name);	
	sw->setTitle(name);
	if (sw == group_stack->entry(globalopts.current_group - 1))
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
				success = sscanf(line, "PLAYMODE: %[^\n]", tmp);	

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
					
				sscanf(line, "%[^\n]", songname);

				if (is_audiofile(songname))
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
	globalopts.current_group = group_stack->stackSize();
	group_stack->entry(globalopts.current_group - 1)->swRefresh(2);
	group_window->changeSelection(group_window->getNitems() - 1 - 
		group_window->sw_selection);
	group_window->swRefresh(2);
	char *bla = group_stack->entry(globalopts.current_group - 1)->getTitle();
	char *title = new char[strlen(bla)+4];
	sprintf(title, "%02d:%s", globalopts.current_group, bla);
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
		*sw = group_stack->entry(globalopts.current_group - 1);
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
	else if (!strcasecmp(pm, "groupsrandom"))
		globalopts.play_mode = PLAY_GROUPS_RANDOMLY;
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
		case PLAY_GROUP:  globalopts.play_mode = PLAY_GROUPS; break;
		case PLAY_GROUPS: globalopts.play_mode = PLAY_GROUPS_RANDOMLY; break;
		case PLAY_GROUPS_RANDOMLY: globalopts.play_mode = PLAY_SONGS; break;
		case PLAY_SONGS: globalopts.play_mode = PLAY_GROUP; break;
		default: globalopts.play_mode = PLAY_GROUP;
	}
	cw_draw_play_mode();
}

/* redraw playmode */
void
cw_draw_play_mode()
{
	unsigned int
		i,
		maxy, maxx;
	char
		*desc;

	getmaxyx(command_window, maxy, maxx);
	
	for (i = 1; i < maxx - 1; i++)
	{	
		mvwaddch(command_window, 17, i, ' ');
		mvwaddch(command_window, 18, i, ' ');
	}

	desc = playmodes_desc[globalopts.play_mode];

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
		mvwaddstr(command_window, 17, 1, playmodes_desc[globalopts.play_mode]);
	wrefresh(command_window);
}

void
play_list()
{
	unsigned int
		nmp3s = 0;
	char
		**mp3s = group_stack->getShuffledList(globalopts.play_mode, &nmp3s);
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
	player->setThreads(globalopts.threads);
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
	if (!filename || !is_audiofile(filename))
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
	player->setThreads(globalopts.threads);
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
		cw_set_fkey(6, "Convert MP3 to WAV");
		cw_set_fkey(7, "Add URL");
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
	int retval = 1;
	
	if (lstat(path, buf) < 0 || ((buf->st_mode) & S_IFMT) != S_IFLNK)
		retval = 0;

	/* forgot to free this. Thanks to Steve Kemp for pointing it out. */
	free(buf);

	return retval;
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
		else if (is_audiofile(newpath))
		{
			if (d2g && !d2g_init)
			{
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
					/* If there are no entries in the current group,
					 * this group will be filled instead of skipping it
					 */
					scrollWin *sw = group_stack->entry(
						globalopts.current_group - 1);
					if (sw->getNitems())
						d2g_groupindex = (add_group(npath) - 1);
					else
					{
						d2g_groupindex = globalopts.current_group - 1;
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
change_threads(int dont)
{
	if (!dont)
		if ( (globalopts.threads += 50) > 500)
			globalopts.threads = 0;
	mvwprintw(command_window, 20, 1, "Threads: %03d", globalopts.threads);
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
		tmp = (group_stack->entry(globalopts.current_group - 1))->sw_win;

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
			*sw = group_stack->entry(globalopts.current_group - 1);
		switch(key)
		{
			case 'q': retval = -1; break;
			case '?': show_help(); break;
			case KEY_DOWN: case 'j': sw->changeSelection(1); break;
			case KEY_UP: case 'k': sw->changeSelection(-1); break;
			case 'd':
			case KEY_DC: sw->delItem(sw->sw_selection); break;
			case KEY_NPAGE: sw->pageDown(); break;
			case KEY_PPAGE: sw->pageUp(); break;
			case ' ': sw->selectItem(); sw->changeSelection(1); break;
			case '+':
				select_group(globalopts.current_group + 1);
				break; //select next group
			case '-':
				select_group(globalopts.current_group - 1);
				break; //select prev. group
			case 'l': refresh_screen(); break; // C-l
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
				draw_settings(1);
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
				del_group(globalopts.current_group); break; //del group
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
				cw_toggle_play_mode(); break;
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
		if (fw_searchmode &&
			((key >= 'a' && key <= 'z') ||
			(key >= 'A' && key <= 'Z') ||
			(key >= '0' && key <= '9')))
		{
			fw_search_next_char(key);
			break;
		}
		switch(key)
		{
			case 'q': retval = -1; break;
			case '?': show_help(); break;
			case KEY_DOWN: case 'j': sw->changeSelection(1); break;
			case KEY_UP: case 'k': sw->changeSelection(-1); break;
			case KEY_NPAGE: sw->pageDown();
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
				file_window->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS,
					ACS_BTEE, ACS_PLUS);
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(6));
				break;
			case KEY_PPAGE: sw->pageUp(); 
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(1)|A_BOLD);
				file_window->setBorder(0, 0, 0, 0, ACS_LTEE, ACS_PLUS,
					ACS_BTEE, ACS_PLUS);
				wbkgdset(file_window->sw_win, ' '|COLOR_PAIR(6));
				break;
			case ' ': sw->selectItem(); sw->changeSelection(1); break;
			case KEY_BACKSPACE: case 'h': fw_changedir(".."); break;
			case '/': fw_start_search(); break;
			case 12: refresh_screen(); break; // C-l
			case 13: /* change into dir, play soundfile, end search */
				if (fw_searchmode)
					fw_end_search();	
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
			{
				mw_settxt("Adding groups..");
				char *tmppwd = get_current_working_path();
				recsel_files(tmppwd, 1, 1);
				free(tmppwd);
				fw_end();
				mw_settxt("Added dirs as  groups.");
			}
				break;
			case KEY_F(6): case '6': /* Convert mp3 to wav */
				fw_convmp3();
				break;
			case KEY_F(7): case '7': /* Add HTTP url to play */
				fw_addurl();
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

void
draw_settings(int cleanit)
{
	/* display play-mode in command_window */
	if (!cleanit)
	{
		mvwaddstr(command_window, 13, 1, "Current group's mode:");
		mvwaddstr(command_window, 16, 1, "Current play-mode: ");
		cw_toggle_group_mode(1);
		cw_draw_play_mode();
#ifdef PTHREADEDMPEG
		change_threads(1); /* display #threads in command_window */
#endif
	}
	else
	{
		mvwaddstr(command_window, 13, 1, "                     ");
		mvwaddstr(command_window, 14, 1, "                     ");
		mvwaddstr(command_window, 16, 1, "                   ");
		mvwaddstr(command_window, 17, 1, "                       ");
		mvwaddstr(command_window, 18, 1, "                       ");
#ifdef PTHREADEDMPEG
		mvwaddstr(command_window, 20, 1, "              ");
#endif
	}
	wrefresh(command_window);		
}

void
fw_convmp3()
{
	char **selitems;
	int nselected;

	if  (progmode != PM_FILESELECTION)
		return;

	selitems = file_window->getSelectedItems(&nselected);

	if (!nselected)
	{
		warning("No files have been selected.");
		refresh_screen();
		return;
	}

	for (int i = 0; i < nselected; i++)
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
		{
			char bla[strlen(file)+80];
			Mpegfileplayer *decoder = NULL;
			char *file2write;

			file2write = gettext("File to write to:", 0);

			if (!file2write || !(decoder = new Mpegfileplayer) ||
				!decoder->openfile(file,
				file2write, WAV))
			{
				sprintf(bla, "Decoding of %s failed.", file);
				warning(bla);
				refresh_screen();
				if (decoder)
					delete decoder;
			}
			else
			{
				sprintf(bla, "Converting to wavefile, please wait.");
				mw_settxt(bla);
				decoder->playing(0);
				delete decoder;
				free(file2write);
			}
		}

		mw_settxt("Conversion(s) done!");
		free(file);
		delete[] pwd;
		delete[] selitems[i];
	}
	delete[] selitems;
}

void
fw_addurl()
{
	scrollWin
		*sw;
	char
		*urlname;

	if (progmode != PM_FILESELECTION)
		return;

	sw = group_stack->entry(globalopts.current_group - 1);
	urlname = gettext("URL:", 0);

	if (!urlname)
		return;

	add_selected_file(urlname);
	sw->swRefresh(0);
	free(urlname);
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
	fw_searchmode = 0;
	signal(SIGALRM, SIG_IGN);
	mw_settxt("");
}

/* called when sigalarm is triggered (i.e. when, in searchmode in filemanager,
 * a key has not been pressed for a few seconds (default: 2). This will
 * disable the searchmode.
 */
void
fw_search_timeout(int blub)
{
	if(blub);	//no warning this way ;)
	if (!fw_searchmode)
		return;

	fw_end_search();
}

/* called when someone presses [a-zA-Z0-9] in searchmode in filemanager */
void
fw_search_next_char(char nxt)
{
	char *tmp;
	short foundmatch = 0;
	scrollWin *sw = file_window;

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
		char *item = sw->getItem(i);
		if (!strncmp(item, tmp, strlen(tmp))) 
		{
			delete[] item;
			sw->setItem(i);
			foundmatch = 1;
			break;
		}
		delete[] item;
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
fw_start_search(int timeout=2)
{
	signal(SIGALRM, &fw_search_timeout);
	fw_set_search_timeout(timeout);
	mw_settxt("Search:");
	fw_searchmode = 1;
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
	if (extensions)
	{
		int i=0;

		while (extensions[i])
		{
			delete[] extensions[i];
			i++;
		}
		delete[] extensions;
	}

	extensions = new char*[nmatches+1];
	for (int i=0; i < nmatches; i++)
	{
		extensions[i] = new char[strlen(matches[i])+1];
		strcpy(extensions[i], matches[i]);
	}
	extensions[nmatches] = NULL;

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
