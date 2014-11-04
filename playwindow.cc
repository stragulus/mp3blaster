#include "mp3blaster.h"
#include NCURSES
#include <string.h>
#include "playwindow.h"

playWindow::playWindow()
{
	short
		begin_x, begin_y;

	status = PS_NORMAL;
	nrlines = 12;
	nrcols = 60;
	interface = NULL;
	begin_x = (COLS - nrcols ) / 2;
	begin_y = (LINES - nrlines ) / 2;

	interface = newwin(nrlines, nrcols, begin_y, begin_x);
	keypad(interface, TRUE);
	leaveok(interface, TRUE);
	wbkgd(interface, COLOR_PAIR(4)|A_BOLD);
	wborder(interface, 0, 0, 0, 0, 0, 0, 0, 0);
	mvwaddstr(interface, 1, 2, "File:");
	mvwaddstr(interface, 2, 2, "Path:");

	//draw buttons
	short offset = (nrcols - 20) / 2;
	mvwaddstr(interface, nrlines - 3, offset, "|< << |> >> >| || []");
	mvwaddstr(interface, nrlines - 2, offset, " 1  2  3  4  5  6  7");
	for (int i = offset; i < offset + 20; i += 3)
	{
		mvwchgat(interface, nrlines - 3, i, 2, A_BOLD, 5, NULL);
		mvwchgat(interface, nrlines - 2, i, 2, A_BOLD, 5, NULL);
	}

	touchwin(interface); /* to get the colours right */
	wrefresh(interface);
	wgetch(interface);
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
		mvwaddch(interface, 1, 8, ' ');
		mvwaddch(interface, 2, 8, ' ');
	}
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
