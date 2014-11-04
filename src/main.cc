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
 * If you like this program, or if you have any comments, tips for improve-
 * ment, money, etc, you can e-mail me at brama@stack.nl or visit my
 * homepage (http://www.stack.nl/~brama/).
 *
 * May 21 2000: removed all calls to attr_get() since not all [n]curses 
 *            : versions use the same paramaters (sigh)
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
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fnmatch.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#error "No pthread, no mp3blaster"
#endif
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
#include "mp3win.h"
#include "fileman.h"
#ifdef HAVE_SIDPLAYER
#include "sidplayer.h"
#endif
#include <mpegsound.h>
#include <nmixer.h>
//load keybindings
#include "keybindings.h"

/* paranoia define[s] */
#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

enum playstatus_t { PS_PLAY, PS_PAUSE, PS_REWIND, PS_FORWARD, PS_PREV,
                    PS_NEXT, PS_STOP, PS_RECORD };

/* values for global var 'window' */
enum _action { AC_NONE, AC_REWIND, AC_FORWARD, AC_NEXT, AC_PREV,
	AC_SONG_ENDED, AC_STOP, AC_RESET, AC_START, AC_ONE_MP3, AC_PLAY } action;
/* External functions */
extern short cf_parse_config_file(const char *);
extern cf_error cf_get_error();
extern const char *cf_get_error_string();

/* Prototypes */
void refresh_screen(void);
int handle_input(short);
void cw_toggle_group_mode();
void cw_draw_group_mode(short cleanit=0);
void cw_draw_play_mode(short cleanit=0);
short set_play_mode(const char*);
void cw_toggle_play_mode();
void mw_clear();
void mw_settxt(const char*);
void read_playlist(const char *);
void write_playlist();
void *play_list(void *);
short start_song(short was_playing=0);
void set_one_mp3(char*);
void stop_song();
void stop_list();
void update_play_display();

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
char *gettext(const char *label, short display_path=0);
short fw_getpath();
void fw_convmp3();
void fw_addurl();
void fw_search_next_char(char);
void fw_start_search(int timeout=2);
void fw_end_search();
void set_sound_device(const char *);
short set_fpl(int);
void set_default_colours();
void set_action(_action bla);
char *determine_song(short set_played=1);
void set_song_info(const char *fl, struct song_info si);
void set_song_status(playstatus_t);
void set_song_time(int,int);
char *popup_win(const char **label, short want_input=0, int maxlen=0);
int myrand(int);
void reset_playlist(int);
void cw_draw_repeat();
void init_helpwin();
void repaint_help();
void get_label(command_t, char *);
void set_keylabel(int k, char *label);
void init_playopts();
void init_globalopts();
command_t get_command(int, program_mode);
void draw_next_song(const char*);
void set_next_song(int);
void reset_next_song();
mp3Win* newgroup();

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
	short pause;
	short playing;
	char * one_mp3;
	Fileplayer *player;
	time_t tyd;
	time_t newtyd;
} playopts;

program_mode
	progmode;
pthread_mutex_t
	playing_mutex = PTHREAD_MUTEX_INITIALIZER,
	ncurses_mutex = PTHREAD_MUTEX_INITIALIZER,
	onemp3_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t
	playing_cond = PTHREAD_COND_INITIALIZER;
pthread_t
	thread_playlist; //thread that handles playlist (main handles input)
mp3Win
	*mp3_curwin = NULL, /* currently displayed mp3 window */
	*mp3_rootwin = NULL, /* root mp3 window */
	*playing_group = NULL; /* in group-mode playback: group being played */
fileManager
	*file_window = NULL; /* window to select files with */
scrollWin
	*helpwin = NULL;
NMixer
	*mixer = NULL;
int
	nselfiles = 0,
	fw_searchmode = 0, /* non-zero if searchmode during file selection is on */
	current_song = -1,
	nr_played_songs = 1,
	next_song = -1;
short
	popup = 0; /* 1 when a `popup' window is onscreen */
struct _globalopts
	globalopts;
char
	**selected_files = NULL,
	*startup_path = NULL,
	*fw_searchstring,
	**played_songs = NULL;

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
		**playmp3s = NULL,
		*init_playlist = NULL,
		*chroot_dir = NULL,
		*config_file = NULL;
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
	
	songinf.update = 0;
	songinf.path = NULL;
	songinf.next_song = NULL;
	songinf.warning = NULL;
	playing_group = NULL;
	mixer = NULL;
	helpwin = NULL;
	long_index = 0;
	played_songs = (char**)malloc(sizeof(char*));
	played_songs[0] = NULL;
	nr_played_songs = 1;
	current_song = -1;
	init_globalopts();
	init_playopts();
	set_default_colours(); // fill globalopts.colours with default values.
	/* parse arguments */
	while (1)
	{
#if 1
		static struct option long_options[] = 
		{
			{ "downsample", 0, 0, '2'},
			{ "8bits", 0, 0, '8'},
			{ "autolist", 1, 0, 'a'},
			{ "config-file", 1, 0, 'c'},
			{ "debug", 0, 0, 'd'},
			{ "help", 0, 0, 'h'},
			{ "list", 1, 0, 'l'},
			{ "no-mixer", 0, 0, 'n'},
			{ "chroot", 1, 0, 'o'},
			{ "playmode", 1, 0, 'p'},
			{ "dont-quit", 0, 0, 'q'},
			{ "runframes", 1, 0, 'r'},
			{ "sound-device", 1, 0, 's'},
			{ "threads", 1, 0, 't'},
			{ 0, 0, 0, 0}
		};
		
		c = getopt_long_only(argc, argv, "28a:c:dhl:no:p:qr:s:t:", long_options,
			&long_index);
#else
		c = getopt(argc, argv, "28a:c:dhl:no:p:qr:s:t:");
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
	signal(SIGALRM, SIG_IGN);

	//Initialize NCURSES
	initscr();
 	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	leaveok(stdscr, TRUE);

	startup_path = get_current_working_path();

	if (LINES < 24 || COLS < 80)
	{
		mvaddstr(0, 0, "You'll need at least an 80x24 screen, sorry.\n");
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

	/* initialize mixer */
	if (!globalopts.no_mixer)
	{
		const int color_pairs[4] = { 13,14,15,16 };
		mixer = new NMixer(stdscr, (globalopts.sound_device &&
			strrchr(globalopts.sound_device, ':') ? "NAS" : NULL), COLS-12,
			LINES-3, 2, color_pairs, globalopts.colours.default_bg, 1);
		if (!(mixer->NMixerInit()))
		{
			delete mixer; mixer = NULL;
		}
		else
		{
			mixer->setMixerCommandKey(MCMD_NEXTDEV, 't');
			mixer->setMixerCommandKey(MCMD_PREVDEV, 'T');
			mixer->setMixerCommandKey(MCMD_VOLUP, '>');
			mixer->setMixerCommandKey(MCMD_VOLDN, '<');
		}
	}

	/* initialize helpwin */
	init_helpwin();
	repaint_help();
	draw_static();
	mw_settxt("Press 'h' to get help on available commands");
	refresh();

	progmode = PM_NORMAL;

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

	/* All options from configfile and commandline have been read. */
	draw_settings();
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
		const char *bla[3];
		short foo[2] = { 0, 1 };
		for (int i = 0; i < (argc - optind); i++)
		{
			bla[0] = (const char*)playmp3s[i];
			bla[1] = chop_path(playmp3s[i]);
			bla[2] = NULL;
			mp3_curwin->addItem(bla, foo, CP_FILE_MP3);
			delete[] playmp3s[i];
		}
		delete[] playmp3s; playmp3s_nr = 0;
		mp3_curwin->swRefresh(0);
		set_action(AC_START);
		//TODO: Global var that tells play_list to quit program if set.
		/*
		if ( !(options & OPT_QUIT))
		{
			endwin();
			exit(0);
		}
		*/
	}
	else if ((options & OPT_LOADLIST) || (options & OPT_AUTOLIST))
	{
		read_playlist(init_playlist);
		delete[] init_playlist;
		if (options & OPT_AUTOLIST)
			set_action(AC_START);
	}

	pthread_create(&thread_playlist, NULL, play_list, NULL);

	/* read input from keyboard */
	while ( (key = handle_input(1)) >= 0);

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
		"\t--downsample/-2: Downsample (44->22Khz etc)\n"\
		"\t--8bits/-8: 8bit audio (autodetected if your card can't handle 16 bits)\n"\
		"\t--config-file/-c=file: Use other config file than the default\n"\
		"\t\t~/.mp3blasterrc\n"\
		"\t--debug/-d: Log debug-info in $HOME/.mp3blaster.\n"\
		"\t--help/-h: This help screen.\n"\
		"\t--no-mixer/-n: Don't start the built-in mixer.\n"\
		"\t--chroot/-o=<rootdir>: Set <rootdir> as mp3blaster's root dir.\n"\
		"\t\tThis affects *ALL* file operations in mp3blaster!!(including\n"\
		"\t\tplaylist reading&writing!) Note that only users with uid 0\n"\
		"\t\tcan use this option (yet?). This feature will change soon.\n"\
		"\t--playmode/-p={onegroup,allgroups,groupsrandom,allrandom}\n"\
		"\t\tDefault playing mode is resp. Play first group only, Play\n"\
		"\t\tall groups in given order(default), Play all groups in random\n"\
		"\t\torder, Play all songs in random order.\n"\
		"\t--dont-quit/-q: Don't quit after playing mp3[s] (only makes sense\n"\
		"\t\tin combination with --autolist or files from command-line)\n"\
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
		const char *bla[3];

		for (i = 0; i < nselfiles; i++)
		{
			bla[0] = (const char*)selected_files[i];
			bla[1] = chop_path(selected_files[i]);
			bla[2] = NULL;
			sw->addItem(bla, foo, CP_FILE_MP3);
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

	draw_static();
	draw_settings();
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
	
	file_window->changeDir(path);
	refresh();
}

/* PRE: Be in PM_FILESELECTION
 * POST: Returns 1 if path was succesfully changed into.
 */
short
fw_getpath()
{
	DIR
		*dir = NULL;
	const char *label[] = {
		"Enter path below",
		NULL,
	};
	char
		*npath = popup_win(label, 1),
		*newpath;
	
	if (!npath || !(newpath = expand_path(npath)))
		return 0;

	delete[] npath;

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
draw_play_key(short do_refresh)
{
	char klabel[4];
	int maxy,maxx;
	int r;

	getmaxyx(stdscr, maxy, maxx);
	if (!globalopts.layout)
		r = 2;
	else
		r = maxx - 12;


	attrset(COLOR_PAIR(CP_LABEL));

	move(maxy-9,r+4);
	
	pthread_mutex_lock(&playing_mutex);
	if (!playopts.playing || (playopts.playing && playopts.song_played && playopts.pause))
		addstr(" |>");
	else
		addstr(" ||");
	pthread_mutex_unlock(&playing_mutex);

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
	if (!globalopts.layout)
		r = 2;
	else
		r = maxx - 12;

	if (file_mode);
	//First, the border
	box(stdscr, ACS_VLINE, ACS_HLINE);
	//Now, the line under the function keys and above the status window
	move(5,1); hline(ACS_HLINE, maxx-2);
	move(5,0); addch(ACS_LTEE); move(5,maxx-1); addch(ACS_RTEE);
	move(maxy-4,1); hline(ACS_HLINE, maxx-2);
	move(maxy-4,0); addch(ACS_LTEE); move(maxy-4,maxx-1); addch(ACS_RTEE);

	if (globalopts.layout == 1)
	{
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
	}

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
	if (!globalopts.layout)
	{
		move(maxy-4,maxx-13); addch(ACS_TTEE);
	}
	else
	{
		move(maxy-4,maxx-13); addch(ACS_PLUS);
	}
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
	move(8,2);
	addstr("Next Song      :");
	mw_settxt("Visit http://tutorial.mp3blaster.cx/");

	if (!globalopts.layout)
	{
		if (playopts.playing)
		{
			int r = maxx - 12;
			//stuff on the left
			move(10,2); addstr("Mpeg-");
			move(10,9); addstr("L");
			move(11,4); addstr("Khz");
			move(11,10); addstr("bit");
			move(12,6); addstr("kb/s");
			//stuff on the right
			move(10,r); addstr("Elapsed:");
			move(11,r+6); addstr("mins");
			move(13,r); addstr("Total Time:");
			move(14,r+6); addstr("mins");
			move(16,r); addstr("Remaining:");
			move(17,r+6); addstr("mins");
		}
		else
		{
			int r = maxx - 12;
			//stuff on the left
			move(10,2); addstr("     ");
			move(10,9); addstr(" ");
			move(11,4); addstr("   ");
			move(11,10); addstr("   ");
			move(12,6); addstr("    ");
			//stuff on the right
			move(10,r); addstr("        ");
			move(11,r+6); addstr("    ");
			move(13,r); addstr("           ");
			move(14,r+6); addstr("    ");
			move(16,r); addstr("          ");
			move(17,r+6); addstr("    ");
		}
	}
	refresh();
	return;
}

void
refresh_screen()
{
	//TODO: clear(); if set_song_info and set_song_status can be called at
	//      any time.
	touchwin(stdscr);
	wnoutrefresh(stdscr);

	helpwin->swRefresh(2);
	repaint_help();
	cw_draw_group_mode();
	cw_draw_play_mode();
#ifdef PTHREADEDMPEG
	//cw_draw_threads();
#endif
	//TODO: draw nextsong

	if (mixer)
		mixer->redraw();

	if (popup);
	else if (progmode == PM_FILESELECTION)
	{
		if (file_window)
			file_window->swRefresh(2);
		draw_static(1);
	}
	else if (progmode == PM_NORMAL)
	{
		mp3Win
			*sw = mp3_curwin;
		draw_static();
		sw->swRefresh(2);
	}
	doupdate();
}

/* Adds a new group to mp3_curwin.
 * TODO: Shouldn't be an int function, but void.
 */
int
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
	return 1;
}

void
set_group_name(mp3Win *sw, int index)
{
	const char
		*nm,
		*label[] = {
			"Enter Name:",
			NULL,
		},
		*name = popup_win(label,1,48);

	if (!sw || !(nm = sw->getItem(index)) || !strcmp(nm, "[..]"))
		return;

	if (!strcmp(name, ".."))
	{
		delete[] name;
		return;
	}
	char tmpname[strlen(name)+3];
	sprintf(tmpname, "[%s]", name);	
	sw->changeItem(index, tmpname);
	mp3Win *gr = sw->getGroup(index);
	if (gr)
		gr->setTitle(tmpname);
	refresh();
	delete[] name;
}

void
mw_clear()
{
	int width = COLS - 14;
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
	addnstr(txt, COLS-14);
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
					const char *bla[3];
					short foo[2] = { 0, 1 };
					bla[0] = (const char*)songname;
					bla[1] = chop_path(songname);
					bla[2] = NULL;
					sw->addItem(bla, foo, CP_FILE_MP3);
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
char *
popup_win(const char **label, short want_input, int maxlen)
{
	int maxx, maxy, width, height, by, bx, i, j;

	popup = 1;
	getmaxyx(stdscr, maxy, maxx);
	if (!globalopts.layout)
	{
		width = maxx - 30;
		height = maxy - 16;
		by = 11; bx = 15; //topleft corner.
	}
	else
	{
		width = maxx - 14;
		height = maxy - 15;
		by = 11; bx = 1;
	}
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
	char *text = new char[MIN(width-2,maxlen)+1];
	if (want_input)
	{
		attrset(A_BOLD|COLOR_PAIR(CP_POPUP_INPUT));
		for (i = 1; i < width -1; i++)
		{
			move(by + height - 2, bx + i); addch(' ');
		}
		move(by+height-2, bx+1);
		echo();
		leaveok(stdscr, FALSE);
		refresh();
		getnstr(text, MIN(width-2,maxlen));
		noecho();
		leaveok(stdscr, TRUE);
	}
	attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	bkgdset(COLOR_PAIR(CP_DEFAULT));
	popup = 0;
	refresh_screen();
	return text;
}

void
read_playlist(const char *filename)
{
	if(filename);
#if 0
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
}

void
write_playlist()
{
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
}

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
cw_draw_group_mode(short cleanit)
{
	char
		*modes[] = {
			"Group Playback : Songs will not be shuffled",
			"Group Playback : Songs will be shuffled",
			};
	mp3Win
		*sw = mp3_curwin;
	unsigned int i;
	int r = COLS - 12;

	if (!globalopts.layout)
	{
		for (i = 2; i < strlen(modes[0]) + 2; i++)
		{
			move(7, i);
			addch(' ');
		}
		if (!cleanit)
		{
			move(7, 2);
			addstr(modes[sw->getPlaymode()]);
		}
	}
	else
	{
		move(11, r+1);
		if (!sw->getPlaymode())
			addch(' ');
		else
			addch('X');
	}
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
	refresh();
}

/* redraw playmode, but does not update the screen. */
void
cw_draw_play_mode(short cleanit)
{
	unsigned int i;
	char *playmodes_desc[] = {
		/* PLAY_NONE */
		"",
		// PLAY_GROUP
		"Global Playback: Only play songs from the current group",
		//PLAY_GROUPS
		"Global Playback: Play all songs from all groups without shuffle",
		//PLAY_GROUPS_RANDOMLY
		"Global Playback: Play all subgroups in this group",
		//PLAY_SONGS
		"Global Playback: Play all songs from all groups in random order",
	};


	for (i = 2; i < strlen(playmodes_desc[2])+2; i++)
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

/*
void *
play_list(void *arg)
{
	Fileplayer *player = NULL;
	short song_played = 0;
	char *song = NULL, *prev_song = NULL, *next_song = NULL;
	int played_one_mp3 = 0;
	time_t tyd, newtyd;

	if(arg);

	//this loops controls the 'playing' variable. The 'action' variable is
	//controlled by the input thread.
	//playing=1: user wants to play
	//playing=0: user doesn't want to play
	while (1)
	{
		_action my_action;
		pthread_mutex_lock(&playing_mutex);
		if (!playing)
		{
			while (action != AC_START)
			{
				pthread_cond_wait(&playing_cond, &playing_mutex);
			}
		}
		my_action = action;
		if (action == AC_START)
		{
			my_action = AC_NEXT;
			action = AC_NONE;
			song = prev_song = next_song = NULL;
		}
		playing = 1;
		pthread_mutex_unlock(&playing_mutex);

		if (!song_played && my_action == AC_NONE)
			my_action = AC_NEXT;

		switch(my_action)
		{
		case AC_START:
		break;
		case AC_NEXT: //start the play!
			if (song_played)
			{
				if (player)
				{
					player->closefile();
					delete player; player = NULL;
				}
				song_played = 0;
				if (prev_song)
					delete[] prev_song;
				prev_song = song;
				song = NULL;
			}
			if (played_one_mp3)
			{
				played_one_mp3 = 0;
				if (!playing_one_mp3) //check for next 'playing_one_mp3' 
				{
					set_action(AC_STOP);
					reset_playlist(0);
					break;
				}
			}
			if (playing_one_mp3)
			{
				if (song)
					delete[] song;
				if (prev_song)
					delete[] prev_song;
				song = new char[strlen(playing_one_mp3)+1];
				strcpy(song, playing_one_mp3);
				playing_one_mp3 = NULL;
				played_one_mp3 = 1;
			}
			else
			{
				song = determine_song();
			}
			if (!song) //at the end of list (?)
			{
				debug("End of playlist\n");
				pthread_mutex_lock(&ncurses_mutex);
				songinf.status = PS_STOP;
				songinf.update |= 4;
				pthread_mutex_unlock(&ncurses_mutex);
				pthread_mutex_lock(&playing_mutex);
				playing = 0;
				action = AC_NONE;
				if (song)
					delete[] song;
				song = NULL;
				pthread_mutex_unlock(&playing_mutex);
				reset_playlist(0);
				break;
			}
			if (is_mp3(song))
				player = new Mpegfileplayer();
#ifdef HAVE_SIDPLAYER
			else if (is_sid(song))
				player = new SIDFileplayer();
#endif
			if (!player->openfile(song, (globalopts.sound_device ?
				globalopts.sound_device : NULL)) || !player->initialize())
			{
				int vaut = player->geterrorcode();
				pthread_mutex_lock(&ncurses_mutex);
				if (songinf.warning)
					delete[] songinf.warning;
				songinf.warning = new char[strlen(sound_errors[vaut])+1];
				strcpy(songinf.warning, sound_errors[vaut]);
				songinf.update |= 8;
				pthread_mutex_unlock(&ncurses_mutex);
				delete player; player = NULL;
				delete[] song; song = NULL;
				if (vaut == SOUND_ERROR_DEVOPENFAIL)
					playing = 0;
				break;
			}
			set_action(AC_NONE);
			song_played = 1;
			{
				const char *sp = chop_path(song);
				pthread_mutex_lock(&ncurses_mutex);
				if (songinf.path)
					delete[] songinf.path;
				songinf.path = new char[strlen(sp)+1];
				strcpy(songinf.path, sp);
				songinf.songinfo = player->getsonginfo();
				songinf.elapsed = 0;
				songinf.remain = songinf.songinfo.totaltime;
				songinf.status = PS_PLAY;
				songinf.update = 1|2|4; //update time,songinfo,status
				pthread_mutex_unlock(&ncurses_mutex);
			}
			//set_song_info(chop_path(song), player->getsonginfo());
			//set_song_status(PS_PLAY);
			time(&tyd);
		break;
		case AC_NONE: //continue playing
		{
			bool status = player->run(globalopts.fpl);
			if (!status)
			{
				int ec = player->geterrorcode();
				if (ec != SOUND_ERROR_OK && ec != SOUND_ERROR_FINISH)
				{
					pthread_mutex_lock(&ncurses_mutex);
					songinf.update |= 8;
					if (songinf.warning)
						delete[] songinf.warning;
					songinf.warning = new char[strlen(sound_errors[ec])+1];
					strcpy(songinf.warning, sound_errors[ec]);
					pthread_mutex_unlock(&ncurses_mutex);
				}
				player->closefile();
				delete player;
				song_played = 0;
				if (played_one_mp3)
				{
					played_one_mp3 = 0;
					pthread_mutex_lock(&playing_mutex);
					playing = 0;
					if (song)
						delete[] song;
					song = NULL;
					pthread_mutex_unlock(&playing_mutex);
				}
			}
			else
			{
				//update elapsed / remaining time?
				time(&newtyd);
				if (difftime(newtyd, tyd) >= 1)
				{
					pthread_mutex_lock(&ncurses_mutex);
					songinf.elapsed = player->elapsed_time();
					songinf.remain = player->remaining_time();
					songinf.update |= 2;
					pthread_mutex_unlock(&ncurses_mutex);
					tyd = newtyd;
				}
			}
		}
		break;
		case AC_PREV: //play previous song
		break;
		case AC_FORWARD: //skip forward in song
			if (player)
				player->skip(globalopts.skipframes);
			set_action(AC_NONE);
		break;
		case AC_REWIND: //rewind in song
			if (player)
				player->skip(-globalopts.skipframes);
			set_action(AC_NONE);
		break;
		case AC_STOP: //stop playback
			if (song_played)
			{
				if (player)
					player->closefile();
				delete player; player = NULL;
				song_played = 0;
			}
			pthread_mutex_lock(&playing_mutex);
			playing = 0;
			pthread_mutex_unlock(&playing_mutex);
			pthread_mutex_lock(&ncurses_mutex);
			songinf.status = PS_STOP;
			songinf.update |= 4;
			pthread_mutex_unlock(&ncurses_mutex);
			//TODO: This is a brute form of resetting the playlist. When a user
			//      presses stop, he might just want to stop _temporarily_
			reset_playlist(0);
			played_one_mp3 = 0;
			set_action(AC_NONE);
		break;
		case AC_PAUSE: //pause playback
			usleep(100000);
		break;
		//TODO: Clear the next 2 cases
		case AC_RESET: //reset all songs to unplayed
		break;
		case AC_SONG_ENDED: //current song's ready.
		break;
		}
	}
}
*/

void *
play_list(void *arg)
{
	if(arg);

	//this loops controls the 'playing' variable. The 'action' variable is
	//controlled by the input thread.
	//playing=1: user wants to play
	//playing=0: user doesn't want to play
	while (1)
	{
		_action my_action;
		pthread_mutex_lock(&playing_mutex);
		if (!playopts.playing)
		{
			debug("Not playing, waiting to start playlist\n");
			while (action != AC_START && action != AC_ONE_MP3 && action != AC_PLAY)
			{
				pthread_cond_wait(&playing_cond, &playing_mutex);
			}
			debug("Playlist started. Let the fun begin!\n");
			if (action == AC_START) //TODO: make difference between AC_START/AC_PLAY
			{
				my_action = AC_NONE;
				action = AC_NONE;
			}
		}
		my_action = action;
		playopts.playing = 1;
		pthread_mutex_unlock(&playing_mutex);

		switch(my_action)
		{
		case AC_STOP:
			debug("Player pressed stop!\n");
			stop_song(); //sets action to AC_NONE
			pthread_mutex_lock(&playing_mutex);
			playopts.playing = 0;
			pthread_mutex_unlock(&playing_mutex);
			pthread_mutex_lock(&ncurses_mutex);
			songinf.update |= 16;
			pthread_mutex_unlock(&ncurses_mutex);
			debug("End of case STOP\n");
		break;
		case AC_NEXT:
			stop_song();
			if (!start_song())
				stop_list();
			//TODO: add 'repear' functionality (but beware not to loop an empty
			//      playlist.....
		break;
		case AC_NONE:
			if (!playopts.pause && playopts.song_played)
			{
				bool mystatus = (playopts.player)->run(globalopts.fpl);
				if (!mystatus)
					stop_song(); //status => AC_NONE
				else
					update_play_display();
			}
			else if (!playopts.pause) //!song_played, let's start one then
			{
				debug("No song to play, let's find one!\n");
				if (!start_song())
				{
					debug("End of playlist reached?\n");
					stop_list();
				}
			}
			else //pause
				usleep(10000);
		break;
		case AC_ONE_MP3: //speelt 1 mp3 af, evt tijdens playlist(!)
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
		case AC_PLAY: //pause,unpause,na stop
			debug("Player pressed play\n");
			set_action(AC_NONE);
			if (playopts.song_played && playopts.pause)
			{
				playopts.pause = 0;
				pthread_mutex_lock(&ncurses_mutex);
				songinf.update |= 16;
				songinf.status = PS_PLAY;
				songinf.update |= 4;
				pthread_mutex_unlock(&ncurses_mutex);
			}
			else if (playopts.song_played && !playopts.pause)
			{
				playopts.pause = 1;
				pthread_mutex_lock(&ncurses_mutex);
				songinf.update |= 16;
				songinf.status = PS_PAUSE;
				songinf.update |= 4;
				pthread_mutex_unlock(&ncurses_mutex);
			}
			else if (!playopts.song_played)
			{
				if (!start_song())
					stop_list();
			}
		break;
		default:
			if(1);
		}
	}
}

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
		pthread_mutex_lock(&ncurses_mutex);
		songinf.elapsed = (playopts.player)->elapsed_time();
		songinf.remain = (playopts.player)->remaining_time();
		songinf.update |= 2;
		pthread_mutex_unlock(&ncurses_mutex);
		playopts.tyd = playopts.newtyd;
	}
}

void set_one_mp3(const char *mp3)
{
	pthread_mutex_lock(&onemp3_mutex);
	if (playopts.one_mp3)
		delete[] playopts.one_mp3;
	if (mp3)
	{
		playopts.one_mp3 = new char[strlen(mp3)+1];
		strcpy(playopts.one_mp3, mp3);
	}
	else
		playopts.one_mp3 = NULL;
	pthread_mutex_unlock(&onemp3_mutex);
}

/* POST: Delete[] returned char*-pointer when you don't use it anymore!! */
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
	//TODO: reset playlist.
	playopts.playing = 0;
	playopts.playing_one_mp3 = 0;
	pthread_mutex_lock(&ncurses_mutex);
	next_song = -1;
	songinf.update |= 32;
	pthread_mutex_unlock(&ncurses_mutex);
}

void
stop_song()
{
	int vaut;
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

	if (playopts.player)
		vaut = (playopts.player)->geterrorcode();
	else
		vaut = SOUND_ERROR_FINISH;

	//generate warning
	pthread_mutex_lock(&ncurses_mutex);
	if (songinf.warning)
	{
		delete[] songinf.warning;
		songinf.warning = NULL;
	}
	if (vaut != SOUND_ERROR_OK && vaut != SOUND_ERROR_FINISH)
	{
		songinf.warning = new char[strlen(sound_errors[vaut])+1];
		strcpy(songinf.warning, sound_errors[vaut]);
		songinf.update |= 8;
	}
	songinf.status = PS_STOP;
	songinf.update |= 4;
	pthread_mutex_unlock(&ncurses_mutex);

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
 * was_playing: if one mp3 is to be played, this determines if the playlist
 *              continues playing after the one mp3 (is was_playing = 1, it
 *              will)
 * POST: playopts.one_mp3 is always NULL
 */
short
start_song(short was_playing)
{
	char *song = NULL;
	short one_mp3 = 0;
	const char *sp = NULL;

	debug("start_song()\n");
	if (!(song = get_one_mp3()) && playopts.playing_one_mp3 == 2)
		return (playopts.playing_one_mp3 = 0);

	if (!song)
	{
		song = determine_song();
		if (song)
			debug("Song determined in start_song()\n");
		else
			debug("Could not determine song in start_song()\n");
		playopts.playing_one_mp3 = 0;
		if (!song)
			return 0;
	}
	else //one_mp3
	{
		playopts.playing_one_mp3 = (was_playing ? 1 : 2);
		one_mp3 = 1;
	}

	set_one_mp3((const char*)NULL);

	if (is_mp3(song))
		playopts.player = new Mpegfileplayer();
#ifdef HAVE_SIDPLAYER
	else if (is_sid(song))
		playopts.player = new SIDFileplayer();
#endif

	if (!playopts.player || !(playopts.player)->openfile(song,
		(globalopts.sound_device ? globalopts.sound_device : NULL)) ||
		!(playopts.player)->initialize())
	{
		stop_song();
		delete[] song;
		return 0;
	}

	playopts.song_played = 1;
	sp = chop_path(song);
	pthread_mutex_lock(&ncurses_mutex);
	if (songinf.path)
		delete[] songinf.path;
	songinf.path = new char[strlen(sp)+1];
	strcpy(songinf.path, sp);
	songinf.songinfo = (playopts.player)->getsonginfo();
	songinf.elapsed = 0;
	songinf.remain = songinf.songinfo.totaltime;
	songinf.status = PS_PLAY;
	songinf.update |= (1|2|4); //update time,songinfo,status
	pthread_mutex_unlock(&ncurses_mutex);
	//set_song_info(chop_path(song), player->getsonginfo());
	//set_song_status(PS_PLAY);
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
	pthread_cond_signal(&playing_cond); //wake up playlist
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
	mp3Win*
		d2g_groupindex = NULL;

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
	//mw_settxt(txt);
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
					const char *bla[3];
					short foo[2] = { 0, 1 };
					bla[0] = (const char*)newpath;
					bla[1] = chop_path(newpath);
					bla[2] = NULL;
					d2g_groupindex->addItem(bla, foo,
						CP_FILE_MP3);
				}
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
	move(y - 5, 2);
	if (!cleanit)
		printw("%03d", globalopts.threads);
	else
		addstr("   ");
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

/* handle_input reads input.
 * If no_delay = 0, then this function will block until input arrives.
 * Returns -1 when the program should finish, 0 or greater when everything's
 * okay.
 */
int
handle_input(short no_delay)
{
	int
		key,
		retval = 0;
	fd_set fdsr;
	struct timeval tv;
	command_t cmd;
	mp3Win 
		*sw = mp3_curwin;
	fileManager
		*fm = file_window;

	if (no_delay)
	{
		fflush(stdin);
		FD_ZERO(&fdsr);
		FD_SET(0, &fdsr);  /* stdin file descriptor */
		tv.tv_sec = tv.tv_usec = 0;
		if (select(FD_SETSIZE, &fdsr, NULL, NULL, &tv) == 1)
			key = getch();
		else
		{
			key = ERR;
		}
	}
	else
	{
		nodelay(stdscr, FALSE);
		key=getch();
	}

	//are ncurses updates needed from the playing thread?
	//TODO make a different thread for this, so that the input thread can
	//wait instead of looping (which is rather inefficient, hence the high
	//cpu usage on f.e. FreeBSD)
	pthread_mutex_lock(&ncurses_mutex);
	if (songinf.update & 1)
		set_song_info((const char *)songinf.path, songinf.songinfo);
	if (songinf.update & 2)
		set_song_time(songinf.elapsed, songinf.remain);
	if (songinf.update & 4)
		set_song_status(songinf.status);
	if (songinf.update & 8 && songinf.warning)
	{
		warning((const char *)songinf.warning);
		delete[] songinf.warning; songinf.warning = NULL;
		refresh_screen();
	}
	if (songinf.update & 16)
		draw_play_key(1);
	if (songinf.update & 32) //draw next_song on screen.
	{
		debug("songinf.update&32\n");
		if (songinf.next_song)
		{
			draw_next_song((const char*)(songinf.next_song));
			free(songinf.next_song);
		}
		else
			reset_next_song();
		songinf.next_song = NULL;
	}
	songinf.update = 0;
	pthread_mutex_unlock(&ncurses_mutex);

	if (no_delay && key == ERR)
	{
		usleep(10000);
		return 0;
	}

	cmd = get_command(key, progmode);

	if (fw_searchmode &&
		((key >= 'a' && key <= 'z') ||
		(key >= 'A' && key <= 'Z') ||
		(key >= '0' && key <= '9')))
	{
		fw_search_next_char(key);
		return 1;
	}

	switch(cmd)
	{
		case CMD_PLAY_PLAY: set_action(AC_PLAY); pthread_cond_signal(&playing_cond);
		break;
	//wake up playlist
		case CMD_PLAY_NEXT: set_action(AC_NEXT); break;
		case CMD_PLAY_PREVIOUS: set_action(AC_PREV); break;
		case CMD_PLAY_FORWARD: set_action(AC_FORWARD); break;
		case CMD_PLAY_REWIND: set_action(AC_REWIND); break;
		case CMD_PLAY_STOP: set_action(AC_STOP); break;
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
			pthread_mutex_lock(&playing_mutex);
			if (!playopts.playing)
				retval = -1;
			pthread_mutex_unlock(&playing_mutex);
		break;
		case CMD_HELP: show_help(); break;
		case CMD_DOWN: sw->changeSelection(1); break;
		case CMD_UP: sw->changeSelection(-1); break;
		case CMD_DEL: 
		{
			mp3Win *tmpgroup;
			pthread_mutex_lock(&playing_mutex);
			//don't delete a group that's playing right now (or contains a group
			//that's being played)
			if (sw->isGroup(sw->sw_selection) && (tmpgroup =
				sw->getGroup(sw->sw_selection)) && tmpgroup->isPlaying())
			{
				if (playopts.playing) //TODO: add warning here
				{
					pthread_mutex_unlock(&playing_mutex);
					break;
				}
				reset_playlist(1);
			}
			sw->delItem(sw->sw_selection);	
			reset_next_song();
			pthread_mutex_unlock(&playing_mutex);
		}
		break;
		case CMD_NEXT_PAGE: sw->pageDown(); break;
		case CMD_PREV_PAGE: sw->pageUp(); break;
		case CMD_SELECT: 
			if (!sw->isGroup(sw->sw_selection))
			{
				sw->selectItem();
				sw->changeSelection(1);
			}
		break;
		case CMD_REFRESH: refresh_screen(); break; // C-l
		case CMD_ENTER: // play 1 mp3 or dive into a subgroup.
			if (sw->isGroup(sw->sw_selection))
			{
				mp3_curwin = sw->getGroup(sw->sw_selection);
				mp3_curwin->swRefresh(2);
				cw_draw_group_mode();
				refresh();
			}
			else if (sw->getNitems() > 0)
				play_one_mp3(sw->getSelectedItem());
		break;
		case CMD_SELECT_FILES: /* file-selection */
		{
			progmode = PM_FILESELECTION;
			draw_settings(1);
			if (file_window)
				delete file_window;
			if (!globalopts.layout)
				file_window = new fileManager(NULL, LINES - 14, COLS - 28,
					10, 14, CP_DEFAULT, 1);
			else
				file_window = new fileManager(NULL, LINES - 13, COLS - 12, 10, 0,
					CP_DEFAULT, 1);
			file_window->drawTitleInBorder(1);
			if (!globalopts.layout)
				file_window->setBorder(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
					ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
			else
				file_window->setBorder(ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
					ACS_LTEE, ACS_PLUS, ACS_LTEE, ACS_PLUS);
			file_window->setDisplayMode(1);
			file_window->swRefresh(0);
			draw_static(1);
		}
		break;
		case CMD_ADD_GROUP: add_group(); break; //add group
		//TODO: rewrite read/write_playlist
		//case CMD_READ_PLAYLIST:
		//	read_playlist((const char*)NULL); break; // read playlist
		//case CMD_WRITE_PLAYLIST:
		//	write_playlist(); break; // write playlist
		case CMD_SET_GROUP_TITLE:
			if (sw->isGroup(sw->sw_selection) &&
				strcmp(sw->getItem(sw->sw_selection), "[..]"))
			{
				set_group_name(sw, sw->sw_selection);
				sw->swRefresh(0);
			}
		break; // change groupname
		case CMD_TOGGLE_REPEAT: //toggle repeat
			//TODO: repeat's not thread-safe..low priority
			globalopts.repeat = 1 - globalopts.repeat;
			cw_draw_repeat();
			refresh();
		break;
		case CMD_TOGGLE_SHUFFLE: cw_toggle_group_mode(); reset_next_song(); break; 
		case CMD_TOGGLE_PLAYMODE: 
			pthread_mutex_lock(&playing_mutex);
			if (!playopts.playing)
			{
				//TODO: is this safe? What if the users STOPS a song (!playing), and
				//later on continues the list? playing_group might be a dangling
				//pointer then...or not, because deleting a group that is being played
				//is caught in CMD_DEL. Or was there something else that could mess
				//up...have to check into this.
				cw_toggle_play_mode();
				reset_next_song();
			}
			else
			{
				//TODO: FIX WARNING to not use sleep() in a threaded env.
				warning("Not possible during playback");
				refresh_screen();
			}
			pthread_mutex_unlock(&playing_mutex);
		break;
		case CMD_START_PLAYLIST:
		{
			short myplay;
			pthread_mutex_lock(&playing_mutex);
			myplay = playopts.playing;
			if (!myplay)
			{
				reset_playlist(1);
				action = AC_START;
				pthread_mutex_unlock(&playing_mutex);
				pthread_cond_signal(&playing_cond); //wake up playlist
			}
			else
				set_action(AC_STOP);
		}
		break;
		case CMD_FILE_DOWN: fm->changeSelection(1); break;
		case CMD_FILE_UP: fm->changeSelection(-1); break;
		case CMD_FILE_NEXT_PAGE: fm->pageDown(); break;
		case CMD_FILE_PREV_PAGE: fm->pageUp(); break;
		case CMD_FILE_SELECT: fm->selectItem(); fm->changeSelection(1); break;
		case CMD_FILE_UP_DIR: fw_changedir(".."); break;
		case CMD_FILE_START_SEARCH: fw_start_search(); break;
		case CMD_FILE_ENTER: /* change into dir, play soundfile, load playlist, 
		                      * end search */
			if (fw_searchmode)
				fw_end_search();	
			else if (fm->isDir(fm->sw_selection))
				fw_changedir();
			else if (is_playlist(fm->getSelectedItem()))
			{
				char
					bla[strlen(fm->getPath()) + strlen(fm->getSelectedItem()) + 2];

				sprintf(bla, "%s/%s", fm->getPath(), fm->getSelectedItem());
				fw_end();
				read_playlist(bla);
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
				*tmppwd = get_current_working_path();

			mw_settxt("Recursively selecting files..");
			recsel_files(tmppwd); //TODO: move this to fileManager class?
			free(tmppwd);
			fw_end();
			mw_settxt("Added all mp3's in this dir and all subdirs " \
				"to current group.");
		}
		break;
		case CMD_FILE_SET_PATH: fw_getpath(); break;
		case CMD_FILE_DIRS_AS_GROUPS:
		{
			char
				*tmppwd = get_current_working_path();
			
			mw_settxt("Adding groups..");
			recsel_files(tmppwd, 1, 1);
			free(tmppwd);
			fw_end();
			mw_settxt("Added dirs as groups.");
		}
		break;
		case CMD_FILE_MP3_TO_WAV: /* Convert mp3 to wav */
			fw_convmp3();
		break;
		case CMD_FILE_ADD_URL: /* Add HTTP url to play */
			fw_addurl();
		break;
		case CMD_HELP_PREV: helpwin->pageUp(); repaint_help(); break;
		case CMD_HELP_NEXT: helpwin->pageDown(); repaint_help(); break;
#ifdef PTHREADEDMPEG
		case CMD_CHANGE_THREAD: change_threads(); break;
#endif
		case CMD_NONE: break;
		default:
			if (mixer)
				mixer->ProcessKey(key);
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
	cw_draw_group_mode(cleanit);
	cw_draw_play_mode(cleanit);
	cw_draw_repeat();
#ifdef PTHREADEDMPEG
		//cw_draw_threads(cleanit);
#endif
	refresh();
}

void
fw_convmp3()
{
	char **selitems;
	int nselected;

	if  (progmode != PM_FILESELECTION)
		return;

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
		return;
	}

	char *tmp, *dir2write;
	const char *label[] = { "Dir to put wavefiles:", startup_path, NULL };

	tmp = popup_win(label,1);
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
	free(tmp);

	if (!dir2write || !is_dir(dir2write))
	{
		warning("Invalid path entered.");
		refresh();
		if (dir2write)
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

			if (!(decoder = new Mpegfileplayer) || !decoder->openfile(file,
				file2write, WAV))
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
				decoder->playing();
				delete decoder;
			}
		}
		else
		{
			warning("Skipping non-audio file");
			refresh();
		}
		mw_settxt("Conversion(s) done!");
		delete[] selitems[i];
		delete[] file;
		delete[] file2write;
	}
	if (selitems)
		delete[] selitems;
	free(dir2write);
}

void
fw_addurl()
{
	mp3Win
		*sw;
	char
		*urlname;
	const char 
		*label[] = { "URL:", NULL };

	if (progmode != PM_FILESELECTION)
		return;

	sw = mp3_curwin;
	urlname = popup_win(label, 1);

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
		const char *item = sw->getItem(i);
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
	if (frames > 1000 || frames < 100) /* get real */
		return 0;
	
	globalopts.skipframes = frames;
	return 1;
}	

short
set_mini_mixer(short yesno)
{
	if (yesno)
		globalopts.minimixer = 1;
	else
		globalopts.minimixer = 0;

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

	if (!globalopts.layout)
		tmp = new mp3Win(LINES - 14, COLS - 28, 10, 14, NULL, 0, CP_DEFAULT, 1);
	else
		tmp = new mp3Win(LINES - 13, COLS - 12, 10, 0, NULL, 0, CP_DEFAULT, 1);

	tmp->drawTitleInBorder(1);
	if (!globalopts.layout)
		tmp->setBorder(ACS_VLINE,ACS_VLINE,ACS_HLINE,ACS_HLINE,
			ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	else
		tmp->setBorder(ACS_VLINE,ACS_VLINE,ACS_HLINE,ACS_HLINE,
			ACS_LTEE, ACS_PLUS, ACS_LTEE, ACS_PLUS);
	
	tmp->setDisplayMode(globalopts.display_mode);
	return tmp;
}

void
set_action(_action bla)
{
	pthread_mutex_lock(&playing_mutex);
	action = bla;
	pthread_mutex_unlock(&playing_mutex);
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

	debug("determine_song called\n");
	total_songs = mp3_rootwin->getUnplayedSongs();
	if (!total_songs)
		return NULL;
	
	switch(globalopts.play_mode)
	{
	case PLAY_NONE:
	break;
	case PLAY_GROUPS:
		if (next_song > -1)
			mysong = mp3_rootwin->getUnplayedSong(next_song, set_played);
		else
			mysong = mp3_rootwin->getUnplayedSong(0, set_played);
		next_song = (total_songs - 1 ? 0 : -1);
	break;

	case PLAY_SONGS: //all songs in random order
		if (next_song > -1)	
			mysong = mp3_rootwin->getUnplayedSong(next_song, set_played);
		else
			mysong = mp3_rootwin->getUnplayedSong(myrand(total_songs), set_played);
		next_song = (total_songs - 1 ? myrand(total_songs - 1) : -1);
	break;

	case PLAY_GROUP: //current group:
	case PLAY_GROUPS_RANDOMLY:
	{
		while (!playing_group || !playing_group->getUnplayedSongs())
		{
			if (playing_group) //this means that there are unplayed songs in it
			{
				playing_group->setNotPlaying();
				playing_group = NULL;
				if (globalopts.play_mode == PLAY_GROUP)
					//finished playing group
					break;
			}

			if (globalopts.play_mode == PLAY_GROUPS_RANDOMLY)
			{
				int 
					tg = mp3_rootwin->getUnplayedGroups();

				if (!tg) //no more groups left to play!
				{
					reset_playlist(0);
					break;
				}

				//determine which group to play.
				if (mp3_rootwin->getPlaymode())
					playing_group = mp3_rootwin->getUnplayedGroup(myrand(tg));
				else
					playing_group = mp3_rootwin->getUnplayedGroup(0);
					
				if (playing_group->getUnplayedSongs(0))
				{
					playing_group->setPlayed();
					playing_group->setPlaying();
				}
				else
					playing_group = NULL;
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
set_next_song(int next_song)
{
	const char *ns = NULL;
	
	switch(globalopts.play_mode)
	{
	case PLAY_GROUPS:
	case PLAY_SONGS:
		ns = mp3_rootwin->getUnplayedSong(next_song, 0); break;
	case PLAY_GROUP:
	case PLAY_GROUPS_RANDOMLY:
		if (!playing_group)
			break;
		ns = playing_group->getUnplayedSong(next_song, 0, 0); break;
	case PLAY_NONE: break;
	}
	//don't draw next_song on the screen yet, because this function can be
	//called from different threads.
	if (!ns)
		return;

	pthread_mutex_lock(&ncurses_mutex);
	songinf.update |= 32;
	if (songinf.next_song)
		free(songinf.next_song);
	songinf.next_song = strdup(chop_path(ns));
	debug(songinf.next_song);
	debug("!\n");
	pthread_mutex_unlock(&ncurses_mutex);
}

void
set_song_info(const char *fl, struct song_info si)
{
	int maxy, maxx, r, i;
	char bla[64];
	
	draw_static();
	getmaxyx(stdscr, maxy, maxx);
	r = maxx - 12;
	if (!globalopts.layout)
	{
		attrset(COLOR_PAIR(CP_NUMBER));
		move(10, 7); printw("%1d", si.mp3_version + 1);
		move(10,11); printw("%1d", si.mp3_layer);
		if (si.samplerate > 1000)
			si.samplerate = si.samplerate / 1000;
		move(11, 2); printw("%2d", si.samplerate);
		move(11, 8); printw("%2d", 16);
		move(12, 2); printw("%-3d", si.bitrate);
		move(13,2); printw("%s", si.mode);
		attron(A_BOLD);
		move(14, r); printw("%02d", si.totaltime/60);
		move(14, r+3); printw("%02d", si.totaltime%60);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
		move(14,r+2); addch(':');
	}
	else
	{
		//total time
		move(13,r+5);addch('/');
		move(13,r+8);addch(':');
		move(6,r); addstr("Mpg  Layer");
		move(7,r); addstr("  Khz    kb");
		attrset(COLOR_PAIR(CP_NUMBER)|A_BOLD);
		move(13,r+6); printw("%02d", si.totaltime/60);
		move(13,r+9); printw("%02d", si.totaltime%60);
		attroff(A_BOLD);
		//mp3 info
		move(8,r); addnstr(si.mode,11);
		move(6,r+3); printw("%1d", si.mp3_version + 1);
		move(6,r+10); printw("%1d", si.mp3_layer);
		move(7,r); printw("%2d", (si.samplerate > 1000 ? si.samplerate / 1000 :
			si.samplerate));
		move(7,r+6); printw("%-3d", si.bitrate);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
	}
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
		"<Unknown Album>"), (si.comment  && strlen(si.comment) ? si.comment : 
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
	}
	refresh();
}

void
set_song_time(int elapsed, int remaining)
{
	int maxy, maxx, r;

	getmaxyx(stdscr, maxy, maxx);
	r = maxx - 12;
	//print elapsed & remaining time
	if (!globalopts.layout)
	{
		attrset(COLOR_PAIR(CP_NUMBER)|A_BOLD);
		move(11, r); printw("%02d", elapsed/60);
		move(11, r+3); printw("%02d", elapsed%60);
		move(17, r); printw("%02d", remaining/60);
		move(17, r+3); printw("%02d", remaining%60);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
		move(11,r+2); addch(':');
		move(17,r+2); addch(':');
	}
	else
	{
		attrset(COLOR_PAIR(CP_NUMBER)|A_BOLD);
		move(13,r); printw("%02d", elapsed/60);
		move(13,r+3); printw("%02d", elapsed%60);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
		move(13,r+2); addch(':');
	}
	refresh();
}

int
myrand(int max_nr)
{
	time_t t;
	srand((unsigned int)time(&t));	
	return (int)(((max_nr * 1.0) * rand()) / (RAND_MAX + 1.0));
}

void
reset_playlist(int full)
{
	if (!full) //only reset currently played group
	{
		if (playing_group)
		{
			playing_group->resetSongs(0); //reset SONGS in played group
			playing_group->setNotPlaying(); //reset playing status of played group
			playing_group->setNotPlayed(); //reset playlist status of played GROUP
			playing_group = NULL;
		}
	}
	else
	{
		if (playing_group)
			playing_group->setNotPlaying();
		playing_group = NULL;
		mp3_rootwin->resetSongs();
		mp3_rootwin->resetGroups();
	}
}

void
cw_draw_repeat()
{
	if (globalopts.layout == 1)
	{
		move(12,COLS-11);
		if (globalopts.repeat)
			addch('X');
		else
			addch(' ');
	}
}
 
/* Function   : init_helpwin
 * Description: sets up the helpwin that's on top of the ncurses window.
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
init_helpwin()
{
	int i = 0;
	char line[COLS-1], desc[27], keylabel[4];
	struct keybind_t k = keys[i];
	
	memset(line, 0, (COLS-1) * sizeof(char));
	//TODO: scrollwin never uses first&last line if no border's used!!
	helpwin = new scrollWin(4,COLS-2,1,1,NULL,0,CP_DEFAULT,0);
	helpwin->hideScrollbar();
	//content is afhankelijk van program-mode, dus bij verandereing van
	//program mode moet ook de commandset meeveranderen.
	while (k.cmd != CMD_NONE)
	{
		set_keylabel(k.key, keylabel);
		sprintf(desc, "[%3s] %-19s ", keylabel, k.desc);
		if (!(i%3))
			strcpy(line, desc);
		else
			strcat(line, desc);
		if (i%3 == 2)
		{
			helpwin->addItem(line);
			memset(line, 0, (COLS-1) * sizeof(char));
		}
		k = keys[++i];
	}
	if (i%3)
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

/* Return the command corresponding to the given keycode */
command_t
get_command(int kcode, program_mode pm)
{
	int i = 0;
	command_t cmd = CMD_NONE;
	struct keybind_t k = keys[i];
	
	while (k.cmd != CMD_NONE)
	{
		if (k.key == kcode && (pm == k.pm || k.pm == PM_ANY))
		{
			cmd = k.cmd;
			break;
		}
		k = keys[++i];
	}
	return cmd;
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

	for (i = 1; i < 5; i++)
	{
		for (j = 2; j < 55; j += 26)
		{
			move (i, j); 
			chgat(3, A_NORMAL, CP_BUTTON, NULL);
		}
	}
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
	playopts.playing_one_mp3 = 0;
	playopts.pause = 0;
	playopts.playing = 0;
	playopts.one_mp3 = NULL;
	playopts.player = NULL;
}

void
init_globalopts()
{
	globalopts.no_mixer = 0; /* mixer on by default */
	globalopts.fpl = 5; /* 5 frames are played between input handling */
	globalopts.sound_device = NULL; /* default sound device */
	globalopts.downsample = 0; /* no downsampling */
	globalopts.eightbits = 0; /* 8bits audio off by default */
	globalopts.fw_sortingmode = 0; /* case insensitive dirlist */
	globalopts.play_mode = PLAY_GROUPS;
	globalopts.warndelay = 2; /* wait 2 seconds for a warning to disappear */
	globalopts.skipframes=100; /* skip 100 frames during music search */
	globalopts.debug = 0; /* no debugging info per default */
	globalopts.minimixer = 0; /* large mixer by default */
	globalopts.display_mode = 1; /* file display mode (1=filename, 2=id3,
	                              * not finished yet.. */
	globalopts.layout = 1; /* different value might mean diff. ncurses layout */
	globalopts.repeat = 1;
	globalopts.extensions = NULL;
	globalopts.plist_exts = NULL;
#ifdef PTHREADEDMPEG
	globalopts.threads = 100;
#endif
}

void
reset_next_song()
{
	int width = COLS - 33;
	char emptyline[width+1];

	next_song = -1;
	move(8,19);
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
	int mywidth = COLS - 19 - 14;
	char empty[mywidth+1];

	if (!ns)
		return;
	
	memset((void*)empty, ' ', mywidth * sizeof(char));
	empty[mywidth] = '\0';

	move(8,19);
	addstr(empty);
	move(8,19);
	addnstr(ns, COLS-19-14);
	refresh();
}
