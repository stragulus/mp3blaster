#ifndef _SCROLLWIN_CLASS_
#define _SCROLLWIN_CLASS_

#include "mp3blaster.h"
#include NCURSES
#include <stdio.h>

class scrollWin
{
public:
	scrollWin(int, int, int, int, char **, int, short, short);
	~scrollWin();

	void changeSelection(int);
	void selectItem();
	void deselectItem(int);
	void setTitle(const char*);
	void swRefresh(short);
	void addItem(const char*);
	void delItem(int);
	void pageDown();
	void pageUp();
	int isSelected(int);
	int writeToFile(FILE *);
	int getNitems();
	char *getSelectedItem();
	char *getItem(int index);
	char **getItems();

	WINDOW *sw_win;
	int sw_playmode;

private:
	int sw_selection;
	char **items;
	int nitems;
	int nselected;
	int width;
	int height;
	int showpath;
	int shown_range[2];
	char *sw_emptyline;
	int *sw_selected_items;
	char *sw_title;
	short xoffset;
};

#endif /* _SCROLLWIN_CLASS_ */
