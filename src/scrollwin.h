/* Changes:
 * 18 Apr 1999: Added changeItem() function.
 * 13 Feb 2000: Items are represented by winItems i/o charpointers.
 * 
 */
#ifndef _SCROLLWIN_CLASS_
#define _SCROLLWIN_CLASS_

#include "mp3blaster.h"
#include NCURSES
#include "winitem.h"
#include <stdio.h>

class scrollWin
{
public:
	scrollWin(int, int, int, int, char **, int, short, short);
	virtual ~scrollWin();

	void changeSelection(int);
	void invertSelection();
	void selectItem();
	void deselectItem(int);
	void setTitle(const char*);
	const char *getTitle();
	void swRefresh(short);
	virtual short addItem(const char*, short status=0, short colour=0, int index=-1,
	             short before=0, void *object=NULL, itemtype type=TEXT);
	virtual short addItem(const char **, short *, short colour=0, int index=-1,
	             short before=0, void *object=NULL, itemtype type=TEXT);
	void changeItem(int, const char *, short nameindex=0);
	void replaceItem(int, winItem *newitem);
	void setItem(int);
	virtual void delItem(int, int del=1);
	void delItems();
	void setBorder(chtype, chtype, chtype, chtype,
	               chtype, chtype, chtype, chtype);
	void pageDown();
	void pageUp();
	int isSelected(int);
	int getNitems();
	const char *getSelectedItem(short nameindex=0);
	char **getSelectedItems(int *itemcount, short nameindex=0);
	const char *getItem(int index, short name_index=0);
	char **getItems(short nameindex=0);
	void dump_contents();
	void dump_info();
	void setDisplayMode(short mode);
	void setDefaultColor(short color);
	short moveItem(int index_from, int index_to, short before=0);
	short moveSelectedItems(int index_to, short before=0);
	void clearwin();
	void drawBorder();
	void drawTitleInBorder(int);
	int itemWidth() { return width - (xoffset ? 2 : 0); }
	void hideScrollbar() { hide_bar = 1; }
	int sw_selection;

protected:
	void init(int, int, int, int, short, short);
	short addItem(winItem *newitem, int index=-1, short before=0);
	winItem *getWinItem(int);
	winItem **getSelectedWinItems(int*);
	scrollWin() {};
	winItem *first, *last;

private:
	int nitems;
	int hide_bar;
	char *sw_title;
	int nselected;
	int width;
	int height;
	int by, bx;
	int showpath;
	int border[8];
	int shown_range[2];
	char *sw_emptyline;
	short xoffset;
	short want_border;
	short dispindex; //which of the NAMEDIM names of each item to show
	short colour; //which (global) colourpair to use for drawing items
	int selectID; //increases after each select, used to sort selected items
	              //in the order they were selected.
	int draw_title; //1 if title should be drawn in border
};

#endif /* _SCROLLWIN_CLASS_ */
