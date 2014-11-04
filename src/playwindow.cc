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

extern int no_mixer; /* from main.cc */

extern void debug(const char*);

playWindow::playWindow()
{
	short
		begin_x, begin_y;

	status = PS_NORMAL;
	nrlines = LINES;
	nrcols = COLS;
	interface = NULL;
	begin_x = 0;
	begin_y = 0;

	interface = newwin(nrlines, nrcols, begin_y, begin_x);
	leaveok(interface, TRUE);
	keypad(interface, TRUE);
	leaveok(interface, TRUE);
	wbkgd(interface, COLOR_PAIR(4)|A_BOLD);
	wborder(interface, 0, 0, 0, 0, 0, 0, 0, 0);
	char header[100];
	sprintf(header, "MP3Blaster V%s (press 'q' to return to Playlist "\
		"Editor)", VERSION);
	mvwaddch(interface, nrlines - 3, 0, ACS_LTEE|COLOR_PAIR(4)|A_BOLD);
	mvwaddch(interface, nrlines - 3, nrcols -1, ACS_RTEE|COLOR_PAIR(4)|A_BOLD);
	mvwaddstr(interface, nrlines - 2, 2, header);
	for (int i = 1; i < nrcols - 1; i++)
		mvwaddch(interface, nrlines - 3, i, ACS_HLINE|COLOR_PAIR(4)|A_BOLD);

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
	mvwchgat(interface, 2, 2, 50, A_BOLD, 3, NULL);

	int color_pairs[4] = { 7,8,9,10 };
	mixer = NULL;
	if (!no_mixer)
	{
		mixer = new NMixer(interface, 7, nrlines - 7 - 5, color_pairs,
			COLOR_BLUE);
		if (!(mixer->NMixerInit()))
		{
			delete mixer;
			mixer = NULL;
		}
	}
	//remove the ugly 0 1 2 3 4 5 bar nmixer displays
	mvwaddstr(interface, 7, 8, "                                           "\
		"         ");
	touchwin(interface); /* to get the colours right */
	wrefresh(interface);
}

playWindow::~playWindow()
{
	delwin(interface);
	if (mixer)
		delete mixer;
}

void
playWindow::setFileName(const char *flname)
{
	char
		*filename = NULL;
	int borderlen = 0;

	if (!flname || !strlen(flname))
		return;

	if (strlen(flname) > (nrcols-4))
	{
		filename = new char[(nrcols-4)+1];
		strcpy(filename, flname+(strlen(flname)-(nrcols-4)));
		filename[0] = filename[1] = filename[2] = '.';
	}
	else
	{
		filename = new char[strlen(flname)+1];
		strcpy(filename,flname);
	}

	borderlen = ((nrcols-4)-strlen(filename))/2;
	//put path in upper border like "-|...this/is/the/path.mp3|-"
	for (int i=1; i < (1+borderlen); i++)
		mvwaddch(interface, 0, i, ACS_HLINE);
	mvwaddch(interface, 0, 1+borderlen, ACS_RTEE);
	mvwaddstr(interface, 0, 2+borderlen, filename);
	int roffset = nrcols - borderlen - 2;
	if (borderlen+2+strlen(filename) == (roffset-1))
		mvwaddch(interface, 0, roffset - 1, ' ');
	mvwaddch(interface, 0, roffset, ACS_LTEE);
	for (int i = roffset + 1; i < roffset + 1 + borderlen; i++)
		mvwaddch(interface, 0, i, ACS_HLINE);

	delete[] filename;
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
playWindow::setProperties(int mpeg_version, int layer, int freq, int bitrate,
	bool stereo)
{
	mvwprintw(interface, 1, 2, "Mpeg-%d layer %d at %dhz, %d kb/s (%s)",
		mpeg_version, layer, freq, bitrate, (stereo ? "stereo" : "mono"));
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
			mvwaddch(interface, 2, i, '#');
		mvwchgat(interface, 2, offset, amount, A_BOLD, 5, NULL);
		touchline(interface, 2, 1);
	}
	else if (progress[0] > progress[1])
	{
		int offset = progress[1] + 2;
		int amount = progress[0] - progress[1];
		progress[0] = progress[1];
		for (int i = offset; i < offset + amount; i++)
			mvwaddch(interface, 2, i, ' ');
		mvwchgat(interface, 2, offset, amount, A_BOLD, 3, NULL);
		touchline(interface, 2, 1);
	}
	wrefresh(interface);
}

void
playWindow::setSongName(const char *sn)
{
	mvwaddstr(interface, 3, 2, "Songname: ");
	mvwaddnstr(interface, 3, 12, "                              ", 30);
	mvwaddnstr(interface, 3, 12, sn, 30);
	wrefresh(interface);
}

void
playWindow::setArtist(const char *ar)
{
	mvwaddstr(interface, 4, 2, "Artist  : ");
	mvwaddnstr(interface, 4, 12, "                              ", 30);
	mvwaddnstr(interface, 4, 12, ar, 30);
	wrefresh(interface);
}

void
playWindow::setAlbum(const char *tp)
{
	mvwaddstr(interface, 5, 2, "Album   : ");
	mvwaddnstr(interface, 5, 12, "                              ", 30);
	mvwaddnstr(interface, 5, 12, tp, 30);
	wrefresh(interface);
}

void
playWindow::setSongYear(const char *yr)
{
	mvwaddstr(interface, 3, 43, "Year  : ");
	mvwaddnstr(interface, 3, 51, "       ", 4);
	mvwaddnstr(interface, 3, 51, yr, 4);
}

void
playWindow::setSongInfo(const char *inf)
{
	mvwaddstr(interface, 6, 2, "Info    :");
	mvwaddnstr(interface, 6, 12, "                              ", 30);
	mvwaddnstr(interface, 6, 12, inf, 30);
	wrefresh(interface);
}

void
playWindow::setTotalTime(int time)
{
	char temp[20];
	totaltime = time;

	sprintf(temp, "/%02d:%02d", time / 60, time%60);
	mvwaddstr(interface, 2, 58, temp);
	updateTime(0);
}

/* Updates displayed time. 
 * Argument ``time'' is the amount of time that has elapsed while playing.
 * This function displays the elapsed time, calculates the remaining time and
 * displays that too.
 */
void
playWindow::updateTime(int time)
{
	char temp[10];

	sprintf(temp, "%02d:%02d", time/60, time%60);
	mvwaddstr(interface, 2, 53, temp);

	//calculate remaining time
	int remtime = totaltime - time;
	if (remtime < 0)
		remtime = 0;
	sprintf(temp, "(-%02d:%02d)", remtime/60, remtime%60);
	mvwaddstr(interface, 2, 64, temp);
	wrefresh(interface);
}

#ifdef DEBUG
void
playWindow::setFrames(int frames)
{
	mvwaddstr(interface, 4, 43, "Frames:");
	mvwaddnstr(interface, 4, 51, "        ", 7);
	mvwprintw(interface, 4, 51, "%d", frames);
	wrefresh(interface);
}
#endif
