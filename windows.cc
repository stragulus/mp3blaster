/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) 1997 Bram Avontuur (brama@stack.nl)
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
 */
#include "mp3blaster.h"
#include NCURSES

/* Function Name: PopUpWindow
 * Description  : Pops up a window on a given part of the screen
 *              : (default is the middle of the screen) and places
 *              : 'txt' in it (should be one line of tekst only!.
 *              : Waits for a key to be pressed then returns.
 * Arguments    : (char *)txt: The text to be printed in the box
 *              : (int)colour_pair: Which colour-pair to use. 
 *              : (default: 1)
 *              : (int)ypos: The y-position of the topleft corner of this
 *              : window (default: middle of the screen, = -1))
 *              : (int)xpos: The x-position of the topleft corner of this
 *              : window (default: middle of the screen, = -1))
 */
void popupWindow(const char *txt, int colour_pair = 1, int ypos = -1,
	int xpos = -1)
{
	WINDOW *a;
	unsigned int columns = MAX(strlen(txt), 27)  + 4;
	unsigned short lines = 7; 
	
	if (ypos > -1 && xpos > -1)
		a = newwin(lines, columns, ypos, xpos);
	else if (ypos > -1)
		a = newwin(lines, columns, ypos, (80 - columns) / 2);
	else if (xpos > -1)
		a = newwin(lines, columns, (LINES - lines) / 2, xpos);
	else
		a = newwin(lines, columns, (LINES - lines) / 2, (COLS - columns) /
			2);
	leaveok(a, TRUE);
	wbkgd(a, COLOR_PAIR(colour_pair));
	mvwprintw(a, 2, (columns - strlen(txt)) / 2, txt); //write text in window
	mvwprintw(a, 4, (columns - 27) / 2, "Press any key to continue..");
	box(a, 0, 0);
	wrefresh(a);
	wgetch(a);
	delwin(a);
	refresh(); // update screen
}
	
// Warning - Display a warning (txt) and wait for a key to be pressed
void
warning(const char *txt)
{
	popupWindow(txt, 1, 13);
}

// MessageBox - Display 'txt' in a blue window in the middle of the screen.
void
messageBox(const char *txt)
{
	popupWindow(txt, 2, 13);
}

#if 0
void
refresh_screen()
{
	wclear(stdscr);
	touchwin(stdscr);
	doupdate();
}
#endif
