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
	void invertSelection();
	void selectItem();
	void deselectItem(int);
	void setTitle(const char*);
	char *getTitle();
	void swRefresh(short);
	void addItem(const char*);
	void delItem(int);
	void delItems(); 
	void setBorder(chtype, chtype, chtype, chtype,
	               chtype, chtype, chtype, chtype);
	void pageDown();
	void pageUp();
	int isSelected(int);
	int writeToFile(FILE *);
	int getNitems();
	char *getSelectedItem();
	char **getSelectedItems(int *itemcount);
	char *getItem(int index);
	char **getItems();

	WINDOW *sw_win;
	int sw_playmode;
	int sw_selection;

protected:
	void init(int, int, int, int, short, short);
	scrollWin() {};
private:
	char **items;
	int nitems;
	int nselected;
	int width;
	int height;
	int showpath;
	int border[8];
	int shown_range[2];
	char *sw_emptyline;
	int *sw_selected_items;
	char *sw_title;
	short xoffset;
	short want_border;
};

#endif /* _SCROLLWIN_CLASS_ */
