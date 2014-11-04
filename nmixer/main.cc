/* A soundmixer for linux with a nice text-interface for those non-X-ers.
 * (C)1999 Bram Avontuur (brama@stack.nl)
 */
#include "nmixer.h"

/* Prototypes */

int
main(int argc, char *argv[])
{
	NMixer *bla;

	/* initialize curses screen management */
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);

	border(0, 0, 0, 0, 0, 0, 0, 0);
	if (COLS < 80 || LINES < 8)
	{
		mvprintw(1,1, "You'll need at least an 80x8 terminal, sorry.");
		getch();
		endwin();
		exit(1);
	}

	mvprintw(0, (COLS - strlen(MYVERSION)) / 2, MYVERSION);

	bla = new NMixer(stdscr, 2, LINES - 4);
	if (!(bla->NMixerInit()))
	{
		endwin();
		fprintf(stderr, "Couldn't initialize mixer.\n");
		exit(1);
	}

	/* get input */
	while (bla->ProcessKey(getch()));

	endwin();
	return 0;
}
