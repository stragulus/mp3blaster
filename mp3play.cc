/* (C) 1997 Bram Avontuur (brama@mud.stack.nl)
 * This is an MP3 (Mpeg layer 3 audio) player. Although there are many 
 * mp3-players around for almost any platform, none of the others have the
 * ability of dividing a playlist into groups. Having groups is very useful,
 * since now you can randomly shuffle complete ALBUMS, without mixing numbers
 * of one album with another album. (f.e. if you have a rock-album and a 
 * new-age album on one CD, you probably wouldn't like to mix those!). With
 * this program you can shuffle the entire playlist, play groups(albums) in
 * random order, play in a predefined order, etc. etc.
 * Thanks go out to Jung woo-jae (jwj95@eve.kaist.ac.kr) who made the 
 * mpegsound library used by this program.
 * If you like this program, or if you have any comments, tips for improve-
 * ment, money, etc, you can e-mail me at the following address:
 * brama@mud.stack.nl.
 * TODO:
 * -loading a playlist (snicker)
 * -fix a memory-leak. After each played song, 16kb is lost(!)
 * -Sorting of directory entries while selecting files
 * -Allowing user to change group-order
 * -Keeping selected files while changing dirs in file-selecting mode
 * -Split code in more logical chunks.
 * -Get/hack source that plays mp2's too. (22Khz 56kbit/s & some more modes)
 * -Get rid of the annoying blinking cursor!
 * KNOWN BUGS:
 * -Might coredump when an attempt to play an mp2 is made/has been made.
 *  (can't fix that, the playing-bit is not my code ;-)
 */
#include <curses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

/* define this when the program's not finished (i.e. stay off :-) */
#define UNFINISHED

#define MAX_FILE_LEN 200
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )

/* values for global var 'window' */
#define WINDOW_GROUP   0
#define WINDOW_GWINDOW 1
#define WINDOW_FILES   2
#define WINDOW_LAST    2

/* play songs in current group */
#define PLAY_GROUP 0
/* play all groups */
#define PLAY_GROUPS 1
/* play entire groups in random order */
#define PLAY_GROUPS_RANDOMLY 2
/* play all songs in random order */
#define PLAY_SONGS 3

/* define DEBUG if you want to have more info(spam) while coding */
#undef DEBUG

struct sw_window
{
	WINDOW *sw_win;
	int sw_selection;
	char **items;
	int nitems;
	int nselected;
	int width;
	int height;
	int showpath;
	int shown_range[2];
	int sw_playmode;
	char *sw_emptyline;
	int *sw_selected_items;
	char *sw_title;
	short xoffset;
} *sw;

struct list
{
	int *song_id;
	short update_list;
	int play_index;
	int listsize;
} *playlist;

/* Prototypes */
extern int playmp3(char*, int);
void sw_refresh(struct sw_window*, short);
void sw_change_selection(struct sw_window*, int);
void sw_select_item(struct sw_window*);
void sw_settitle(struct sw_window*, const char*);
void set_header_title(const char*);
void sw_newwin(struct sw_window*, int, int, int, int, char**, int, short,
               short);
void sw_endwin(struct sw_window*);
void sw_additem(struct sw_window*, const char *);
void sw_delitem(struct sw_window*, int);
int sw_is_selected(struct sw_window *, int);
int sw_write_to_file(struct sw_window *, FILE *);
void fw_create(void);
void fw_end(void);
int fw_isdir(int);
void fw_destroy(void);
void fw_changedir(const char*);
void refresh_screen(void);
void Error(const char*);
int is_mp3(const char*);
void cw_set_fkey(short, const char*);
void cw_toggle_group_mode(short);
void cw_toggle_play_mode(short);
void add_group();
void select_group();
void set_group_name(struct sw_window *);
void mw_clear();
void mw_settxt(const char*);
void write_playlist();
void play_list();

/* global vars */

WINDOW
	*command_window = NULL, /* on-screen command window */
	*header_window = NULL, /* on-screen header window */
	*message_window = NULL; /* on-screen message window (bottom window) */
struct sw_window
	**group_stack = NULL, /* array of groups available */
	*group_window = NULL, /* on-screen group-window */
	*file_window = NULL; /* window to select files with */
int
	*group_order = NULL, /* array of group-id's */
	current_group, /* selected group (group_stack[current_group - 1]) */
	ngroups, /* amount of groups currently available */
	window,
	playmode = PLAY_SONGS,
	*is_dir = NULL;

char
	*pwd = NULL; /* path to select files from */

char *playmodes_desc[] = {
	"Play current group",				/* PLAY_GROUP */
	"Play all groups in normal order",	/* PLAY_GROUPS */
	"Play all groups in random order",	/* PLAY_GROUPS_RANDOMLY */
	"Play all songs in random order"	/* PLAY_SONGS */
	};

int
main(int argc, char *argv[])
{
	int i;
	char
		*grps[] = { "01" },
		*dummy = NULL;

	int key;

	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	if (LINES < 25 || COLS < 80)
	{
		mvaddstr(0, 0, "You'll need at least an 80x25 screen, sorry.\n");
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

	/* initialize command window */
	command_window = newwin(LINES - 2, 24, 0, 0);
	wborder(command_window, 0, ' ', 0, 0, 0, ACS_HLINE, ACS_LTEE, ACS_HLINE);
	cw_set_fkey(1, "Select files");
	cw_set_fkey(2, "Add group");
	cw_set_fkey(3, "Select group");
	cw_set_fkey(4, "Set group's title");
	cw_set_fkey(5, "Write playlist");
	cw_set_fkey(6, "Toggle group mode");
	cw_set_fkey(7, "Toggle play mode");
	cw_set_fkey(8, "Play");

	/* initialize header window */
	header_window = newwin(2, COLS - 27, 0, 24);
	wborder(header_window, 0, 0, 0, ' ', ACS_TTEE, ACS_TTEE, ACS_VLINE,
		ACS_VLINE);
	wrefresh(header_window);

	/* initialize selection window */
	sw = (struct sw_window *)malloc(sizeof(struct sw_window));
	sw_newwin(sw, LINES - 4, COLS - 27, 2, 24, NULL,
		0, 0, 1);
	wborder(sw->sw_win, 0, 0, 0, 0, ACS_LTEE, ACS_RTEE, ACS_BTEE, ACS_BTEE);
	sw_refresh(sw, 0);
	sw_settitle(sw, "Default");
	dummy = (char *)malloc((strlen("01:Default") + 1) * sizeof(char));
	strcpy(dummy, "01:Default");
	set_header_title(dummy);
	free(dummy);

	/* initialize group window */
	group_stack = (struct sw_window**)malloc(1 * sizeof(struct sw_window*));
	group_order = (int *)malloc(1 * sizeof(int));
	group_stack[0] = sw;
	group_order[0] = 0; /* first group */
	current_group = 1; /* == group_stack[current_group - 1] */
	ngroups = 1;
	
	group_window = (struct sw_window*)malloc(sizeof(struct sw_window));
	sw_newwin(group_window, LINES - 2, 3, 0, COLS - 3, grps, 1, 0, 0);
	wborder(group_window->sw_win, ' ', 0, 0, 0, ACS_HLINE, 0, ACS_HLINE,
		ACS_RTEE);
	sw_refresh(group_window, 0);

	/* initialize message window */
	message_window = newwin(2, COLS, LINES - 2, 0);
	wborder(message_window, 0, 0, ' ', 0, ACS_VLINE, ACS_VLINE, 0, 0);
	wrefresh(message_window);

	/* display play-mode in command_window */
	mvwaddstr(command_window, 14, 1, "Current group's mode:");
	cw_toggle_group_mode(1);
	mvwaddstr(command_window, 17, 1, "Current play-mode: ");
	cw_toggle_play_mode(1);

	/* initialize playlist */
	playlist = NULL;
	playlist = (struct list*)malloc(sizeof(struct list));
	playlist->update_list = 1;
	playlist->song_id = NULL;

	window = WINDOW_GROUP; /* window used is group window */
	/* read input from keyboard */
	while ( (key = wgetch(sw->sw_win)) != 'q')
	{
		switch(key)
		{
			case KEY_DOWN:
				if (file_window)
					sw_change_selection(file_window, 1);
				else
					sw_change_selection(group_stack[current_group - 1], 1);
				break;
			case KEY_UP:
				if (file_window)
					sw_change_selection(file_window, -1);
				else
					sw_change_selection(group_stack[current_group - 1], -1);
				break;
			case KEY_DC:
				if (!file_window)
				{
					sw_delitem(group_stack[current_group - 1],
						sw->sw_selection);
				}
				break;
			case ' ':
				if (file_window)
				{
					sw_select_item(file_window);
					sw_change_selection(file_window, 1);
				}
				else
				{
					sw_select_item(group_stack[current_group - 1]);
					sw_change_selection(group_stack[current_group - 1], 1);
				}
				break;
			case 13: /* enter */
				if (!file_window)
				{
					/* in test-phase ! ! */
					if (group_stack[current_group - 1]->nitems > 0)
					{
						const char
							header[] = "Now playing: ";
						char
							*filename = group_stack[current_group - 1]->
								items[group_stack[current_group - 1]->
								sw_selection],
							*message = (char *)malloc(strlen(filename) +
								strlen(header) + 1);

						strcpy(message, header);
						strcat(message, filename);
						mw_settxt(message);
						free(message);
						playmp3(filename, 100);
						mw_clear();
					}
				}
				else if (fw_isdir(file_window->sw_selection))
				{
					fw_changedir(file_window->items[file_window->sw_selection]);
				}
				break;
			case 9: /* tab */
				if (window == WINDOW_GROUP)
					window = WINDOW_GWINDOW;
				else if (window == WINDOW_GWINDOW && !file_window)
					window = WINDOW_GROUP;
				else if (window == WINDOW_GWINDOW && file_window)
					window = WINDOW_FILES;
				break;
			case KEY_F(1): /* F1 - select files */
				if (file_window)
					fw_end();
				else	
					fw_create();
				break;
			case KEY_F(2):/* F2 - add group */
				if (!file_window)
					add_group();
				break;
			case KEY_F(3): /* F3 - select group */
				if (!file_window)
					select_group();
				break;
			case KEY_F(4): /* F4 - set group's name */
				if (!file_window)
					set_group_name(group_stack[current_group - 1]);
				break;
			case KEY_F(5): /* F5 - write playlist */
				if (file_window)
					break;
				write_playlist();
				break;
			case KEY_F(6): /* F6 - toggle group's mode */
				cw_toggle_group_mode(0);
				playlist->update_list = 1;
				break;
			case KEY_F(7): /* F7 - toggle play mode */
				cw_toggle_play_mode(0);
				break;
			case KEY_F(8): /* F8 - play list */
				playlist->update_list = 1;
				play_list();
				break;
			case 'c': /* another test-key */
				sw_additem(group_stack[current_group - 1], "Hoei.mp3");
				sw_refresh(group_stack[current_group - 1], 1);
				break;
			case 'd': /* another test-key */
				for ( i = 0; i < group_stack[current_group - 1]->nitems; i++)
					Error(group_stack[current_group - 1]->items[i]);
				break;
			case 'e': /* even another test-key */
			{
				char a[100];
				sprintf(a, "range = %d..%d", group_stack[current_group-1]->
					shown_range[0], group_stack[current_group-1]->
					shown_range[1]);
				Error(a);
			}
			break;
		}
	}

	endwin();
	return 0;
}

/* Function Name: sw_newwin
 * Description  : Creates a new selection-window, setting width and
 *              : height, items in this window, and whether or not the 
 *              : entire path to each item should be displayed.
 * Arguments    : sw      : selection-window to create
 *              : lines   : amount of lines for this window
 *              : ncols   : amount of columns for this window
 *              : begin_y : y-coordinate of topleft corner from this window
 *              :         : in stdscr.
 *              : begin_y : as begin_y, but now x-coordinate.
 *              : items   : array of strings containing items for this window
 *              : nitems  : amount of items.
 *              : showpath: non-zero if path should be shown.
 *              : x_offset: 1 if window has a left-border, 0 otherwise.
 */
void
sw_newwin(struct sw_window* sw, int lines, int ncols, int begin_y, int
	begin_x, char **items, int nitems, short showpath, short x_offset)
{
	int i;

	sw->sw_win = newwin(lines, ncols, begin_y, begin_x);
	sw->width = ncols;
	sw->height = lines;
	sw->sw_selection = 0;
	keypad(sw->sw_win, TRUE);
	sw->items = NULL;
	sw->sw_title = NULL;
	sw->xoffset = x_offset;
	sw->sw_playmode = 0; /* normal (non-random) play mode */
	
	for (i = 0; i < nitems; i++)
	{
		sw->items = (char**)realloc(sw->items, (i + 1) * sizeof(char*));
		sw->items[i] = (char*)malloc((strlen(items[i]) + 1) * sizeof(char));
		strcpy(sw->items[i], items[i]);
	}
	sw->nitems = nitems;
	sw->showpath = showpath;
	sw->shown_range[0] = 0;
	sw->shown_range[1] = MIN(sw->height - 3, nitems - 1);

	if (sw->shown_range[1] < 0)
		sw->shown_range[1] = 0;

	/* create empty line */
	sw->sw_emptyline = (char *)malloc((sw->width - 1 - x_offset + 1 ) *
		sizeof(char));
	for (i = 0; i < sw->width - 1 - x_offset; i++)
		sw->sw_emptyline[i] = ' ';
	sw->sw_emptyline[sw->width - 2] = '\0';

	sw->sw_selected_items = NULL;
	sw->nselected = 0;
}

/* sw_endwin frees all memory allocated by sw, but does not deallocate the
 * memory for the struct itself! (i.e. you need to free(sw) yourself)
 */
void
sw_endwin(struct sw_window *sw)
{
	int i;
	
	delwin(sw->sw_win);
	
	for (i = 0; i < sw->nitems; i++)
	{
		free(sw->items[i]);
	}
	free(sw->items);
	free(sw->sw_emptyline);
	if (sw->sw_selected_items)
		free(sw->sw_selected_items);
	if (sw->sw_title)
		free(sw->sw_title);
}

/* sw_additem adds an item to the selection-list of window sw.
 * It is advised to call sw_refresh(sw) after [a series of] sw_additem[s].
 */
void
sw_additem(struct sw_window *sw, const char *item)
{
	int maxy, maxx;
	
	if (!item || !strlen(item))
		return;
	
	sw->items = (char **)realloc(sw->items, (sw->nitems + 1) * sizeof(char*));
	sw->items[(sw->nitems)++] = (char *)malloc(strlen(item) + 1);
	strcpy(sw->items[sw->nitems - 1], item);

	getmaxyx(sw->sw_win, maxy, maxx);
	
	if (sw->nitems > 1 && sw->shown_range[1] < maxy - 3)
		++(sw->shown_range[1]);
}

/* deselect an item in sw. Usually the item to
 * deselect is sw->items[sw->selection].
 */
void
sw_deselect_item(struct sw_window *sw, int which)
{
	int
		index = -1,
		i,
		need_redraw = 0;
	
	for (i = 0;  i < sw->nselected; i++)
	{
		if (sw->sw_selected_items[i] == which)
		{
			index = i;
			break;
		}
	}
	if (index < 0) /* sw->items[which] not selected */
		return;
	
	if (which != sw->sw_selection)
		++need_redraw;

	/* shrink sw_selected_items array */
	for (i = index; i < sw->nselected - 1; i++)
		sw->sw_selected_items[i] = sw->sw_selected_items[i + 1];
	
	if ( !(--(sw->nselected)))
	{
		free(sw->sw_selected_items);
		sw->sw_selected_items = NULL;
	}
	else
		sw->sw_selected_items = (int *)realloc(sw->sw_selected_items,
			(sw->nselected) * sizeof(int));
	
	sw_refresh(sw, need_redraw);
}

/* sw_delitem removes sw->sw_items[item_index] and redraws selection window
 */
void
sw_delitem(struct sw_window *sw, int item_index)
{
	int
		i;
		
	if ( !sw->items || (item_index + 1) > sw->nitems || item_index < 0)
		return;

	if ( sw_is_selected(sw, item_index) )
		sw_deselect_item(sw, item_index);
	
	for (i = item_index; i < (sw->nitems - 1); i++)
	{
		free(sw->items[i]);
		sw->items[i] = (char *)malloc(strlen(sw->items[i + 1]) + 1);
		strcpy(sw->items[i], sw->items[i + 1]);
	}
	
	free(sw->items[sw->nitems - 1]);

	if ( !(--(sw->nitems)) )
	{
		free(sw->items);
		sw->items = NULL;
		sw->sw_selection = 0;
		sw->shown_range[0] = 0;
		sw->shown_range[1] = 0;
		mvwaddstr(sw->sw_win, 1, 1, sw->sw_emptyline);
		sw_refresh(sw, 1);
		return;
	}
	else
	{
		sw->items = (char **)realloc(sw->items, sw->nitems * sizeof(char *));
	}

	/* check old situation */
	/* this case never happens when someone removes an on-screen mp3file
	 * And therefore I removed it (Bram).
	if (item_index < sw->shown_range[0]) 
	{
		--(sw->shown_range[0]);
		--(sw->sw_selection);
	}
	 */

	/*
	if (sw->shown_range[1] >= sw->nitems)
	{
		sw->shown_range[1] = sw->nitems - 1;
	}
	*/
	if (sw->shown_range[0] > 0 && sw->shown_range[1] > sw->nitems -1)
	{
		--(sw->shown_range[0]);
		--(sw->shown_range[1]);
	}
	else if (sw->shown_range[1] > sw->nitems - 1)
	{
		/* shown_range[1] always was drawn on the window. Erase that line */
		mvwaddstr(sw->sw_win, 1 + sw->shown_range[1], 1, 
			sw->sw_emptyline);
		--(sw->shown_range[1]);	
	}		

	if (sw->sw_selection >= sw->nitems)
		--(sw->sw_selection);
	if (sw->sw_selection < sw->shown_range[0])
		sw->sw_selection = sw->shown_range[0];

	sw_refresh(sw, 1);
}

void
sw_settitle(struct sw_window *sw, const char *tmp)
{
	sw->sw_title = (char *)malloc(strlen(tmp) + 1);

	strcpy(sw->sw_title, tmp);
}

/* Function Name: sw_refresh
 * Description  : Redraws selection-window sw. Usually called after 
 *              : creating and initializing the window or when the highlighted
 *              : selection-bar is to be replaced.
 * Arguments    : sw    : the selection-window to redraw
 *              : scroll: Whether or not the contents of the window scroll 
 *              :       : up/down (non-zero if they do), or if the window 
 *              :       : needs to be redrawn.
 */
void
sw_refresh(struct sw_window *sw, short scroll)
{
	int i;

	for (i = 1; i < MIN((sw->height) - 1, sw->nitems -
		sw->shown_range[0] + 1); i++)
	{
		int
			item_index = (i + sw->shown_range[0]) - 1;
		char *item_name;

		/* erase current line if needed */
		if ((item_index == sw->sw_selection) || scroll)
			mvwaddstr(sw->sw_win, i, sw->xoffset, sw->sw_emptyline);

		if (sw->showpath)
			item_name = sw->items[item_index];
		else
		{
			char *last_slash = strrchr(sw->items[item_index], '/');
			int begin;

			if (!last_slash) /* no '/' in sw->items[item_index] */
				begin = 0;
			else
				begin = strlen(sw->items[item_index]) - strlen(last_slash) + 1;

			item_name = sw->items[item_index] + begin;
		}
	
		mvwaddnstr(sw->sw_win, i, sw->xoffset, item_name, sw->width - 1 -
			sw->xoffset);

		if ( sw_is_selected(sw, item_index) )
		{
			mvwchgat(sw->sw_win, i, sw->xoffset, strlen(item_name),
				A_BOLD, 1, NULL);
		}

		if (item_index == sw->sw_selection)
		{
			int colour = 1;
#ifdef DEBUG
			mvprintw(16, 1, "item_index = %d", item_index);
			refresh();
#endif
			/* draw highlighted selection bar */
			if ( sw_is_selected(sw, item_index) )
				++colour;
			mvwchgat(sw->sw_win, i, sw->xoffset, strlen(item_name),
				A_REVERSE, colour, NULL);
		}
	}
	if (scroll)
		touchwin(sw->sw_win);
	wrefresh(sw->sw_win);
}

/* Function Name: sw_change_selection
 * Description  : the currently selected item in sw is increased (or 
 *              : decreased if change < 0) with change. The window is 
 *              : redrawn and shows the highlighted bar over the new
 *              : selection.
 * Argument     : sw    : window to change selection in.
 *              : change: selection-modifier.
 */
void
sw_change_selection(struct sw_window *sw, int change)
{
	int
		new_selection = sw->sw_selection + change;
	short
		need_to_scroll = 0;

	if (!sw->nitems) /* empty itemlist */
		return;

	/* remove the highlighted bar from the old selection */
	mvwchgat(sw->sw_win, (sw->sw_selection - sw->shown_range[0]) + 1,
		sw->xoffset, sw->width - 1 - sw->xoffset, A_NORMAL, 1, NULL);

	if (new_selection >= sw->nitems)
		new_selection = 0;
	if (new_selection < 0)
		new_selection = sw->nitems - 1;

	if (new_selection < sw->shown_range[0])
	{
		sw->shown_range[0] = new_selection;
		sw->shown_range[1] = MIN(new_selection + (sw->height - 3),
			sw->nitems - 1);
		++need_to_scroll;
	}
	else if (new_selection > sw->shown_range[1])
	{
		sw->shown_range[1] = new_selection;
		sw->shown_range[0] = new_selection - (sw->height - 3);
		if (sw->shown_range[0] < 0) 
		{
			/* this happens when an item has just been added while there were
			 * less than sw->height - 3 items before.
			 */
			sw->shown_range[0] = 0;
		}
		++need_to_scroll;
	}

	sw->sw_selection = new_selection;
	
#ifdef DEBUG
	mvprintw(12, 1, "shown_range[0] = %d", sw->shown_range[0]);
	mvprintw(13, 1, "shown_range[1] = %d", sw->shown_range[1]);
	mvprintw(14, 1, "sw->selection = %d", sw->sw_selection);
	refresh();
#endif

	sw_refresh(sw, need_to_scroll);
}

/* is 'which' amongst sw->sw_selected_items[] ?
 * (i.e. is sw->items[which] selected ?)
 */
int
sw_is_selected(struct sw_window *sw, int which)
{
	int i;

	for (i = 0; i < sw->nselected; i++)
	{
		if (sw->sw_selected_items[i] == which)
			return 1;
	}
	
	return 0;
}

/* [de]select sw->sw_selection
 * sw->sw_window will be updated.
 */
void
sw_select_item(struct sw_window *sw)
{
	if (!sw->nitems) /* nothing to select */
		return;

	if ( sw_is_selected(sw, sw->sw_selection) ) /* already selected */
	{
		sw_deselect_item(sw, sw->sw_selection);
		return;
	}

	sw->sw_selected_items = (int *)realloc(sw->sw_selected_items, 
		(++(sw->nselected)) * sizeof(int));
	sw->sw_selected_items[sw->nselected - 1] = sw->sw_selection;

	sw_refresh(sw, 0);
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
	middle = (x - strlen(title)) / 2;
	if (middle < 1)
		middle = 1;

	for (i = 1; i < x - 1; i++)
		mvwaddch(header_window, 1, i, ' ');
	mvwaddnstr(header_window, 1, middle, title, x - 2);
	wrefresh(header_window);
}

/* fw_create() reads a directory's contents and creates a file-
 * selection window with all *.mp3 filenames.
 * TODO: if an error while reading/opening a dir occurs, the program will
 * exit. In the future this needs to be replaced by a box with an error
 * message without exiting.
 * PRE: int *is_dir should not be alloc'd and be NULL.
 */
void
fw_create()
{
	struct dirent
		*entry = NULL;
	char
		**entries = NULL;
	int
		nitems = 0,
		i;
	DIR
		*dir = NULL;
	
	if (!pwd)
	{	
		char *tmppwd = (char *)malloc(65 * sizeof(char));
		int size = 65;

		while ( !(getcwd(tmppwd, size - 1)) )
		{
			size += 64;
			tmppwd = (char *)realloc(tmppwd, size * sizeof(char));
		}

		pwd = (char *)malloc(strlen(tmppwd) + 2);
		strcpy(pwd, tmppwd);
		
		if (strcmp(pwd, "/")) /* pwd != "/" */
			strcat(pwd, "/");
		free(tmppwd);
	}
	
	if (!pwd)
	{
		fprintf(stderr, "Error reading directory!\n");
		exit(1);
	}

	if ( !(dir = opendir(pwd)) )
	{
		fprintf(stderr, "Error opening directory!\n");
		exit(1);
	}

	if (is_dir)
	{
		free(is_dir);
		is_dir = NULL;
	}

	while ( (entry = readdir(dir)) )
	{
		char *path = (char *)malloc(strlen(pwd) + entry->d_reclen + 1);
		int extra_len = 1;
		DIR *dir2 = NULL;
		
		entries = (char **)realloc (entries, (++nitems) * sizeof(char *));
		is_dir = (int *)realloc(is_dir, nitems * sizeof(int));
		
		strcpy(path, pwd);
		strcat(path, entry->d_name);
		if ( (dir2 = opendir(path)) ) /* path is a dir! */
		{
			++extra_len;
			closedir(dir2);
			is_dir[nitems - 1] = 1;
		}
		else
			is_dir[nitems - 1] = 0;

		entries[nitems - 1] = (char *)malloc( ((entry->d_reclen) + extra_len) *
			sizeof(char));
		strcpy(entries[nitems - 1], entry->d_name);
		if (extra_len == 2) /* this entry is a directory */
			strcat(entries[nitems - 1], "/");
		free(path);
	}
	
	closedir(dir);
	
	if (file_window)
	{
		sw_endwin(file_window);
		free(file_window);
		file_window = NULL;
	}

	file_window = (struct sw_window *)malloc(sizeof(struct sw_window));
	sw_newwin(file_window, LINES - 4, COLS - 27, 2, 24, entries,
		nitems, 1, 1);
	wborder(file_window->sw_win, 0, 0, 0, 0, ACS_LTEE, ACS_RTEE, ACS_BTEE,
		ACS_BTEE);
	sw_refresh(file_window, 0);

	sw_settitle(file_window, pwd);

	set_header_title(file_window->sw_title);
	wrefresh(header_window);

	cw_set_fkey(1, "Add files to group");
	for (i = 0; i < nitems; i++)
		free(entries[i]);
	if (entries)
		free(entries);
}

/* fw_end closes the file_window if it's on-screen, and moves all selected
 * files into the currently used group. Finally, the selection-window of the
 * current group is put back on the screen and the title in the header-window
 * is set to match that of the current group.
 */
void
fw_end()
{
	int i;
	char *dummy = NULL;

	if (!file_window)
		return;

	for ( i = 0; i < file_window->nselected; i++)
	{
		char
			*itemname = NULL;
		int
			item_index = file_window->sw_selected_items[i];

	/* Depricated.
	for ( i = 0; i < file_window->nitems; i++)
	{
		char *itemname = NULL;
		
		if ( !file_window->sw_selected_items[i])
			continue;

		if (is_dir[i])
			continue;
	*/
		if (is_dir[item_index])
			continue;

		if (!is_mp3(file_window->items[item_index]))
			continue;

		itemname = (char *)malloc(strlen(file_window->items[item_index]) +
			strlen(pwd) + 1);
		sprintf(itemname, "%s%s", pwd, file_window->items[item_index]);
		
		sw_additem(group_stack[current_group - 1], itemname);
		free(itemname);
	}
	fw_destroy();
	sw_refresh(group_stack[current_group -1], 1);
	dummy = (char *)malloc((strlen(group_stack[current_group - 1]->sw_title) +
		10) * sizeof(char));
	sprintf(dummy, "%02d:%s", current_group, group_stack[current_group - 1]->
		sw_title);
	set_header_title(dummy);
	free(dummy);

	cw_set_fkey(1, "Select files");
}

void
fw_destroy()
{
	if (is_dir)
	{
		free(is_dir);
		is_dir = NULL;
	}
	sw_endwin(file_window);
	free(file_window);
	file_window = NULL;
}

/* fw_isdir returns non-zero when file_window->items[selection] is a dir. 
 */
int
fw_isdir(int selection)
{
	if (!file_window)
		return 0;

	if (selection < 0 || selection > (file_window->nitems))
		return 0;
	if (is_dir[selection])
		return 1;
	return 0;
}

void
fw_changedir(const char *path)
{
	char *newpath = (char *)malloc((strlen(path) + 1) * sizeof(char));

	if (!file_window)
		return;

	strcpy(newpath, path);	
	fw_destroy();
	if (pwd)
	{
		free(pwd);
		pwd = NULL;
	}
	/* don't panic if chdir fails. We'll remain in the same dir */
	if (chdir(newpath) < 0)
	{
		char error[80];
		sprintf(error, "ERRNO: %d", errno);
		Error(error);
	}
	fw_create(); /* automagically gets new cwd if pwd == NULL */
}

void
refresh_screen()
{
	touchwin(command_window);
	wnoutrefresh(command_window);
	touchwin(header_window);
	wnoutrefresh(header_window);
	touchwin(group_window->sw_win);
	wnoutrefresh(group_window->sw_win);
	if (file_window)
	{
		touchwin(file_window->sw_win);
		wnoutrefresh(file_window->sw_win);
	}
	else
	{
		touchwin(group_stack[current_group - 1]->sw_win);
		wnoutrefresh(group_stack[current_group - 1]->sw_win);
	}
	doupdate();
}

/* Now some Nifty(tm) message-windows */
void
Error(const char *txt)
{
	int
		offset = (COLS - 2 - strlen(txt)) / 2,
		i;

	for (i = 1; i < COLS - 1; i++)
	{
		chtype tmp = '<';
		
		if (i < offset)
			tmp = '>';
		mvwaddch(message_window, 0, i, tmp);
	}
	mvwaddnstr(message_window, 0, offset, txt, COLS - offset - 1);
	mvwchgat(message_window, 0, 1, COLS - 2, A_BOLD, 3, NULL);
	wrefresh(message_window);
	wgetch(message_window);
	for (i = 1; i < COLS - 1; i++)
		mvwaddch(message_window, 0, i, ' ');
	
	wrefresh(message_window);
}

int
is_mp3(const char *filename)
{
	int len;
	
	if (!filename || (len = strlen(filename)) < 5)
		return 0;

	if (strcasecmp(filename + (len - 4), ".mp3"))
		return 0;
	
	return 1;
}

void
cw_set_fkey(short fkey, const char *desc)
{
	int
		maxy, maxx;
	char *fkeydesc = NULL;
		
	if (fkey < 1 || fkey > 12)
		return;
	
	/* y-pos of fkey-desc = fkey */
	
	getmaxyx(command_window, maxy, maxx);
	
	fkeydesc = (char *)malloc((5 + strlen(desc) + 1) * sizeof(char));
	sprintf(fkeydesc, "F%2d: %s", fkey, desc);
	mvwaddnstr(command_window, fkey, 1, "                              ",
		maxx - 1);
	if (strlen(desc))
		mvwaddnstr(command_window, fkey, 1, fkeydesc, maxx - 1);
	touchwin(command_window);
	wrefresh(command_window);
}

void
add_group()
{
	char gnr[3], *gtitle = NULL;

	ngroups++;
	
	group_stack = (struct sw_window**)realloc(group_stack, ngroups *
		sizeof(struct sw_window*));
	group_stack[ngroups - 1] =
		(struct sw_window*)malloc(sizeof(struct sw_window));
	sprintf(gnr, "%02d", ngroups);
	sw_additem(group_window, gnr);
	group_window->sw_selection = ngroups - 1;
	sw_refresh(group_window, 1);
	
	sw_newwin(group_stack[ngroups - 1], LINES - 4, COLS - 27, 2, 24, NULL,
		0, 0, 1);
	wborder(group_stack[ngroups - 1]->sw_win, 0, 0, 0, 0, ACS_LTEE, ACS_RTEE,
		ACS_BTEE, ACS_BTEE);
	sw_refresh(group_stack[ngroups - 1], 0);
	
	gtitle = (char *)malloc(16 * sizeof(char));
	sprintf(gtitle, "%02d:Default", ngroups);
	sw_settitle(group_stack[ngroups - 1], "Default");
	set_header_title(gtitle);
	free(gtitle);
	current_group = ngroups;
	cw_toggle_group_mode(1); /* display this group's mode */
}

void
select_group()
{
	char *dummy = NULL;
	
	if (++current_group > ngroups)
		current_group = 1;
	sw_refresh(group_stack[current_group - 1], 1);
	touchwin(group_window->sw_win);
	sw_change_selection(group_window, 1);
	dummy = (char*)malloc( (strlen(group_stack[current_group - 1]->sw_title) +
		10) * sizeof(char));
	sprintf(dummy, "%02d:%s", current_group, group_stack[current_group - 1]->
		sw_title);
	set_header_title(dummy);
	free(dummy);
	cw_toggle_group_mode(1); /* display this group's playmode */
}

void
set_group_name(struct sw_window *sw)
{
	WINDOW
		*gname = newwin(5, 64, (LINES - 5) / 2, (COLS - 64) / 2);
	char
		name[49], tmpname[55];

	keypad(gname, TRUE);
	wbkgd(gname, COLOR_PAIR(4)|A_BOLD);
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
	sw_settitle(sw, name);
	if (sw == group_stack[current_group - 1])
		set_header_title(tmpname);
	delwin(gname);
	refresh_screen();
}

void
mw_clear()
{
	int i;

	for (i = 1; i < COLS - 1; i++)
		mvwaddch(message_window, 0, i, ' ');
	wrefresh(message_window);
}

void
mw_settxt(const char *txt)
{
	mw_clear();

	mvwaddnstr(message_window, 0, 1, txt, COLS - 2);
	wrefresh(message_window);
}

/* write the contents of sw to a file (f, which must be opened for writing).
 * This is how the contents are written to the file:
 * GROUPNAME: <name>
 * <file1>
 * ...
 * <fileN>
 * <empty line>
 * Note: f is not closed. When an error while writing occurs 0 will be 
 * returned (non-zero otherwise)
 */
int
sw_write_to_file(struct sw_window *sw, FILE *f)
{
	char header[] = "GROUPNAME: ";
	char *group_header = (char *)malloc((strlen(header) + strlen(sw->sw_title) +
		2) * sizeof(char));
	int i;
	
	strcpy(group_header, header);
	strcat(group_header, sw->sw_title);
	strcat(group_header, "\n");
	
	if (!fwrite(group_header, sizeof(char), strlen(group_header), f))
		return 0;
	
	for (i = 0; i < sw->nitems; i++)
	{
		if (!fwrite(sw->items[i], sizeof(char), strlen(sw->items[i]), f))
			return 0;
		if (!fwrite("\n", sizeof(char), 1, f))
			return 0;
	}

	return 1; /* everything succesfully written. */
}

void
write_playlist()
{
	int i;
	FILE *f;
	
	if ( !(f = fopen("test.lst", "w")) )
	{
		Error("Error opening playlist for writing!");
		return;
	}
	
	for (i = 0; i < ngroups; i++)
	{
		if ( !(sw_write_to_file(group_stack[i], f)) ||
			!(fwrite("\n", sizeof(char), 1, f)))
		{
			Error("Error writing playlist!");
			fclose(f);
			return;
		}
	}
	
	fclose(f);
}

void
cw_toggle_group_mode(short notoggle)
{
	char
		*modes[] = {
			"Play in normal order",
			"Play in random order"
			};
	sw_window
		*sw = group_stack[current_group - 1];

	int
		i, maxy, maxx;

	if (!notoggle)
		sw->sw_playmode = 1 - sw->sw_playmode; /* toggle between 0 and 1 */

	getmaxyx(command_window, maxy, maxx);
	
	for (i = 1; i < maxx - 1; i++)
		mvwaddch(command_window, 15, i, ' ');

	mvwaddstr(command_window, 15, 1, modes[sw->sw_playmode]);
	wrefresh(command_window);
}

void
cw_toggle_play_mode(short notoggle)
{
	int
		i, maxy, maxx;
	char
		*desc;

	if (!notoggle)
	{
		if (++playmode > PLAY_SONGS)
			playmode = PLAY_GROUP;
		playlist->update_list = 1;
	}

	getmaxyx(command_window, maxy, maxx);
	
	for (i = 1; i < maxx - 1; i++)
	{	
		mvwaddch(command_window, 18, i, ' ');
		mvwaddch(command_window, 19, i, ' ');
	}

	desc = playmodes_desc[playmode];

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
		mvwaddnstr(command_window, 18, 1, desc, index - 1);
		mvwaddstr(command_window, 19, 1, desc + index);
		free(desc2);
	}
	else
		mvwaddstr(command_window, 18, 1, playmodes_desc[playmode]);
	wrefresh(command_window);
}

void
play_entire_list()
{
	int i;

	for (i = 0; i < playlist->listsize; i++)
	{
		const char
				header[] = "Now playing: ";
		char
			*filename,
			*message;
		int
			sid, gid, j, a, keypress;

		a = playlist->song_id[i];
		
		for (j = 0; j < ngroups; j++)
		{
			a -= (group_stack[j]->nitems);
			if (a < 0)
			{
				gid = j;
				sid = a + group_stack[j]->nitems;
				break;
			}
		}
	
		message = NULL;

		filename = group_stack[gid]->items[sid];
		message = (char *)malloc(strlen(filename) +
							strlen(header) + 1);
		
		strcpy(message, header);
		strcat(message, filename);
		mw_settxt(message);
		free(message);
		keypress = playmp3(filename, 100);
		switch(keypress)
		{
			case KEY_F(8):
				return;
		}
		mw_clear();
#ifdef UNFINISHED
		sleep(1);
#endif
	}
}

/* PRE: arr is malloced in at least range [offset, offset + group_stack[group]->
 * nitems-1] && srandom() is called with a seed like time()
 * POST: arr[offset..offset+group_stack[group]->nitems-1] is filled with song-
 * id's for the songs in group_stack[group]. The random/normal playmode for
 * that group is also considered.
 */
void
compose_group(int group, int *arr, int offset)
{
	sw_window 
		*sw = group_stack[group];
	int
		i,
		precount = 0;

	for (i = 0; i < group; i++)
		precount += group_stack[i]->nitems;
		
	for (i = 0; i < sw->nitems; i++)
		arr[offset + i] = precount + i;
	
	if (sw->sw_playmode == 1) /* randomly play songs in group */
	{
		for (i = 0; i < sw->nitems; i++)
		{
			int
				rn = random()%sw->nitems,
				tmp = arr[offset + i];

			arr[offset + i] = arr[offset + rn];
			arr[offset + rn] = tmp;
		}
	}
}

void
play_list()
{
	int 
		i,
		nsongs = 0;
	time_t
		t;

	cw_set_fkey(8, "Stop playing");
	cw_set_fkey(9, "Pause/Continue");

	if (!playlist->update_list)
	{
		play_entire_list();
		cw_set_fkey(8, "Play");
		cw_set_fkey(9, "");
		return;
	}

	if (playlist->song_id)
	{
		free(playlist->song_id);
		playlist->song_id = NULL;
	}
	playlist->update_list = 0;
	playlist->play_index = 0;
	playlist->listsize = 0;

	srandom(time(&t));
	
	if (playmode == PLAY_SONGS) /* play all songs in random order */
	{
		for (i = 0; i < ngroups; i++)
			nsongs += group_stack[i]->nitems;
		
		playlist->listsize = nsongs;
		playlist->song_id = (int *)malloc(nsongs * sizeof(int));
		
		for (i = 0; i < nsongs; i++)
			playlist->song_id[i] = i;

		/* the kewl 'card-shuffle' algorithm */
		for (i = 0; i < nsongs; i++)
		{
			int
				rn = random()%nsongs,
				tmp;

			tmp = playlist->song_id[i];
			playlist->song_id[i] = playlist->song_id[rn];
			playlist->song_id[rn] = tmp;
		}

		/* nsongs[0..nsongs-1], each element being [0..nsongs-1] */
	}
	else if (playmode == PLAY_GROUP)
	{
		sw_window *sw = group_stack[current_group - 1];
		
		playlist->song_id = (int *)malloc((sw->nitems) * sizeof(int));
		playlist->listsize = sw->nitems;

		compose_group(current_group - 1, playlist->song_id, 0);
	}
	else if (playmode == PLAY_GROUPS)
	{		
		int
			precount = 0;
		for (i = 0; i < ngroups; i++)
			nsongs += group_stack[i]->nitems;
		
		playlist->listsize = nsongs;
		playlist->song_id = (int *)malloc(nsongs * sizeof(int));

		for (i = 0; i < ngroups; i++)
		{
			compose_group(i, playlist->song_id, precount);
			precount += group_stack[i]->nitems;
		}
	}
	else if (playmode == PLAY_GROUPS_RANDOMLY)
	{
		int
			*grouplist = (int *)malloc(ngroups * sizeof(int)),
			precount = 0;
		
		for (i = 0; i < ngroups; i++)
			nsongs += group_stack[i]->nitems;

		playlist->listsize = nsongs;
		playlist->song_id = (int *)malloc(nsongs * sizeof(int));
		
		for (i = 0; i < ngroups; i++)
			grouplist[i] = i;
		for (i = 0; i < ngroups; i++)
		{
			int
				rn = random()%ngroups,
				tmp = grouplist[i];

			grouplist[i] = grouplist[rn];
			grouplist[rn] = tmp;
		}

		/* grouplist is array with group-id's in random order */
		for (i = 0; i < ngroups; i++)
		{
			compose_group(grouplist[i], playlist->song_id, precount);
			precount += group_stack[grouplist[i]]->nitems;
		}

		if (grouplist)
			free(grouplist);
	}
	
	play_entire_list();
	cw_set_fkey(8, "Play");
	cw_set_fkey(9, "");
	return;
}
