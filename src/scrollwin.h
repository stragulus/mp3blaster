/* Changes:
 * 18 Apr 1999: Added changeItem() function.
 * 13 Feb 2000: Items are represented by winItems i/o charpointers.
 * 
 */
#ifndef _SCROLLWIN_CLASS_
#define _SCROLLWIN_CLASS_

#include "mp3blaster.h"
#include NCURSES_HEADER
#include "winitem.h"
#include "exceptions.h"
#include <stdio.h>

enum sw_searchflags { SW_SEARCH_NONE, SW_SEARCH_CASE_INSENSITIVE };

class scrollWin
{
public:
	/* Function Name: scrollWin::scrollWin
	 * Description  : Creates a new selection-window, setting width and
	 *              : height, items in this window, and whether or not the 
	 *              : entire path to each item should be displayed.
	 * Arguments    : sw      : selection-window to create
	 *              : lines   : amount of lines for this window
	 *              : ncols   : amount of columns for this window
	 *              : begin_y : y-coordinate of topleft corner from this window
	 *              :         : in stdscr.
	 *              : begin_x : as begin_y, but now x-coordinate.
	 *              : arr     : array of strings containing items for this
	 *              :         : window. May be NULL if narr is 0.
	 *              : narr    : amount of items.
	 *              : color   : Default colourpair to use
	 *              : x_offset: 1 if window has a (left-)border of 1, 0 otherwise.
	 * Throws       : IllegalArgumentsException when size requirements have not
	 *              : been met
	 */
	scrollWin(int lines, int ncols, int begin_y, int begin_x, char **arr,
		int narr, short color, short x_offset);

	virtual ~scrollWin();

	/* Function   : resize
	 * Description: Resizes the window
	 * Parameters : width
	 *            :  new width
	 *            : height
	 *            :  new height
	 * Returns    : Nothing.
	 * SideEffects: None.
	 * Throws     : IllegalArgumentsException when size requirements have not
	 *            : been met
	 */
	virtual void resize(int width, int height);

	void changeSelection(int);
	void invertSelection();
	void selectItem(int which = -1);
	void deselectItem(int);
	void deselectItems(); //deselect *all* items
	void selectItems(const char *pattern, const char *type = "regex",
		sw_searchflags flags = SW_SEARCH_CASE_INSENSITIVE,
		short display_index = 0);
	void setTitle(const char*);
	const char *getTitle();
	void swRefresh(short);
	virtual short addItem(const char*, short status=0, short colour=0, int index=-1,
	             short before=0, void *object=NULL, itemtype type=TEXT);
	virtual short addItem(const char **, short *, short colour=0, int index=-1,
	             short before=0, void *object=NULL, itemtype type=TEXT);
	int findItem(const char *, short nameindex = 0);
	void changeItem(int, const char *, short nameindex=0);
	void changeItem(int, char *(*)(void *), void *arg=NULL, short nameindex=0);
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
	short getDisplayMode() { return dispindex; }
	void setDefaultColor(short color);
	short moveItem(int index_from, int index_to, short before=0);
	short moveSelectedItems(int index_to, short before=0);
	void clearwin();
	void drawBorder();
	void drawTitleInBorder(int);
	int itemWidth() { return width - (xoffset ? 2 : 0); }
	void hideScrollbar() { hide_bar = 1; }
	/* setWrap: if enabled, list will wrap at boundaries when moving the 
	 * scrollbar with changeSelection */
	void setWrap(bool want_wrap) { wrap = (want_wrap ? 1 : 0); }
	void enableScreenUpdates() { enable_updates = 1; }
	void disableScreenUpdates() { enable_updates = 0; }
	int sw_selection;

/* Added by Douglas Richard <fmluder@imsa.edu> - March 11, 2002 */
	void resetPan();
	void pan(short);
	void jumpTop();
	void jumpBottom();
/* End Add */

protected:
	void init(int, int, int, int, short, short);
	short addItem(winItem *newitem, int index=-1, short before=0);
	winItem *getWinItem(int);
	winItem **getSelectedWinItems(int*);
	scrollWin() {};
	winItem *first, *last;

private:
	void checkSize(const int lines, const int ncols, const int x_offset);

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
	short wrap;
	short want_border;
	short max_pan_offset; /* maximum pansize */
	short dispindex; //which of the NAMEDIM names of each item to show
	short colour; //which (global) colourpair to use for drawing items
	int selectID; //increases after each select, used to sort selected items
	              //in the order they were selected.
	int draw_title; //1 if title should be drawn in border

	/* enable_updates determines whether functions in this class will call
	 * call swRefresh after altering this window's content. If zero, one
	 * should always call swRefresh(1) after calling a function that messes
	 * with content/layout. Default is to allow updates from within the class.
	 * You might want to disable it temporarily when performing lots of 
	 * addItems() in a loop, etc.
	 */
	short enable_updates; 

	int panoffset;
};

#endif /* _SCROLLWIN_CLASS_ */
