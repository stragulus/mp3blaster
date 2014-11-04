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
 */
#include "mp3blaster.h"
#include NCURSES
#include <string.h>
#include "playwindow.h"

playWindow::playWindow()
{
	short
		begin_x, begin_y;

	status = PS_NORMAL;
	nrlines = 15;
	nrcols = 60;
	interface = NULL;
	begin_x = (COLS - nrcols ) / 2;
	begin_y = (LINES - nrlines ) / 2;

	interface = newwin(nrlines, nrcols, begin_y, begin_x);
	leaveok(interface, TRUE);
	keypad(interface, TRUE);
	leaveok(interface, TRUE);
	wbkgd(interface, COLOR_PAIR(4)|A_BOLD);
	wborder(interface, 0, 0, 0, 0, 0, 0, 0, 0);
	char header[100];
	sprintf(header, "MP3Blaster V%s (C)1998 Bram Avontuur (brama@stack.nl)",
		VERSION);
	mvwaddch(interface, nrlines - 3, 0, ACS_LTEE|COLOR_PAIR(4)|A_BOLD);
	mvwaddch(interface, nrlines - 3, nrcols -1, ACS_RTEE|COLOR_PAIR(4)|A_BOLD);
	mvwaddstr(interface, nrlines - 2, 2, header);
	for (int i = 1; i < nrcols - 1; i++)
		mvwaddch(interface, nrlines - 3, i, ACS_HLINE|COLOR_PAIR(4)|A_BOLD);
	mvwaddstr(interface, 1, 2, "File:");
	mvwaddstr(interface, 2, 2, "Path:");

	//draw buttons
	short offset = (nrcols - 20) / 2;
	mvwaddstr(interface, nrlines - 5, offset, "|< << |> >> >| || []");
	mvwaddstr(interface, nrlines - 4, offset, " 1  2  3  4  5  6  7");
	for (int i = offset; i < offset + 20; i += 3)
	{
		mvwchgat(interface, nrlines - 5, i, 2, A_BOLD, 5, NULL);
		mvwchgat(interface, nrlines - 4, i, 2, A_BOLD, 5, NULL);
	}

	//draw empty progress-bar
	progress[0] = progress[1] = 0;
	mvwchgat(interface, 5, 2, 50, A_BOLD, 3, NULL);

	touchwin(interface); /* to get the colours right */
	wrefresh(interface);
}

playWindow::~playWindow()
{
	delwin(interface);
}

void
playWindow::setFileName(const char *flname)
{
	char
		*filepath = NULL;
	const char
		*filename = NULL;

	if (!flname || !strlen(flname))
		return;


	if ( !(filename = strrchr(flname, '/')) )
	{
		filename = flname;
		filepath = new char[3];
		strcpy(filepath, "./");
	}
	else
	{
		filename = filename + 1;
		int fplen = strlen(flname) - strlen(filename);
		filepath = new char[fplen + 1];
		strncpy(filepath, flname, fplen);
		filepath[fplen] = '\0';
	}

	// erase possibly old filenames from interface
	for (int i = 8; i < nrcols - 1; i++)
	{
		mvwaddch(interface, 1, i, ' ');
		mvwaddch(interface, 2, i, ' ');
	}
	//touchline(interface, 1, 2);
	mvwaddnstr(interface, 1, 8, filename, nrcols - 10);
	mvwchgat(interface, 1, 8, MIN(strlen(filename), nrcols - 10), A_BOLD,
		1, NULL);
	mvwaddstr(interface, 2, 2, "Path:");
	mvwaddnstr(interface, 2, 8, filepath, nrcols - 10);
	mvwchgat(interface, 2, 8, MIN(strlen(filepath), nrcols - 10), A_BOLD,
		1, NULL);
	
	delete[] filepath;
	wrefresh(interface);
}

void
playWindow::setStatus(playstatus ps)
{
	status = ps;
}

chtype
playWindow::getInput()
{
	chtype
		ch;

	if (status == PS_PLAYING || status == PS_PAUSED)
	{
		nodelay(interface, TRUE);
		ch = wgetch(interface);
		nodelay(interface, FALSE);
	}
	else
	{
		nodelay(interface, FALSE);
		ch = wgetch(interface);
	}

	return ch;
}

void
playWindow::setProperties(int layer, int freq, int bitrate, bool stereo)
{
	mvwprintw(interface, 4, 2, "Mpeg-2 layer %d at %dhz, %d kb/s (%s)",
		layer, freq, bitrate, (stereo ? "stereo" : "mono"));
	wrefresh(interface);
}

void
playWindow::redraw()
{
	wclear(stdscr);
	wrefresh(interface);
}

void
playWindow::setProgressBar(int percentage)
{
	progress[1] = percentage/2; /* 50 chars represent 100%.. */
	if (progress[1] > 50)
		progress[1] = 50;
		
	if (progress[1] == progress[0]) /* no change... continue! */
		return;
	if (progress[1] > progress[0])
	{
		int offset = 2 + progress[0];
		int amount = progress[1] - progress[0];
		progress[0] = progress[1];
		for (int i = offset; i < offset + amount; i++)
			mvwaddch(interface, 5, i, '#');
		mvwchgat(interface, 5, offset, amount, A_BOLD, 5, NULL);
		touchline(interface, 5, 1);
	}
	else if (progress[0] > progress[1])
	{
		int offset = progress[1] + 2;
		int amount = progress[0] - progress[1];
		progress[0] = progress[1];
		for (int i = offset; i < offset + amount; i++)
			mvwaddch(interface, 5, i, ' ');
		mvwchgat(interface, 5, offset, amount, A_BOLD, 3, NULL);
		touchline(interface, 5, 1);
	}
	wrefresh(interface);
}

void
playWindow::setSongName(const char *sn)
{
	mvwaddstr(interface, 6, 2, "Songname: ");
	mvwaddnstr(interface, 6, 12, "                              ", 30);
	mvwaddnstr(interface, 6, 12, sn, 30);
	wrefresh(interface);
}

void
playWindow::setArtist(const char *ar)
{
	mvwaddstr(interface, 7, 2, "Artist  : ");
	mvwaddnstr(interface, 7, 12, "                              ", 30);
	mvwaddnstr(interface, 7, 12, ar, 30);
	wrefresh(interface);
}

void
playWindow::setAlbum(const char *tp)
{
	mvwaddstr(interface, 8, 2, "Album   : ");
	mvwaddnstr(interface, 8, 12, "                              ", 30);
	mvwaddnstr(interface, 8, 12, tp, 30);
	wrefresh(interface);
}

void
playWindow::setSongYear(const char *yr)
{
	mvwaddstr(interface, 6, 43, "Year  : ");
	mvwaddnstr(interface, 6, 51, "       ", 4);
	mvwaddnstr(interface, 6, 51, yr, 4);
}

void
playWindow::setSongInfo(const char *inf)
{
	mvwaddstr(interface, 9, 2, "Info    :");
	mvwaddnstr(interface, 9, 12, "                              ", 30);
	mvwaddnstr(interface, 9, 12, inf, 30);
	wrefresh(interface);
}

#ifdef DEBUG
void
playWindow::setFrames(int frames)
{
	mvwaddstr(interface, 7, 43, "Frames:");
	mvwaddnstr(interface, 7, 51, "        ", 7);
	mvwprintw(interface, 7, 51, "%d", frames);
	wrefresh(interface);
}
#endif
