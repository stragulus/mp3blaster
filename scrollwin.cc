/* MP3Blaster V2.0b1 - An Mpeg Audio-file player for Linux
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scrollwin.h"

/* Function Name: scrollWin::scrollWin
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
 *              : x_offset: 1 if window has a (left-)border of 1, 0 otherwise.
 */
scrollWin::scrollWin(int lines, int ncols, int begin_y, int begin_x,
                     char **arr, int narr, short showpath, short x_offset)
{
	int i;

	init(lines, ncols, begin_y, begin_x, showpath, x_offset);

	/* memory for array 'items' is allocated the conventional 'C' way, because
	 * it's a lot easier to increase the array than using vectors or similar..
	 */
	for (i = 0; i < narr; i++)
	{
		this->items = (char**)realloc(this->items, (i + 1) * sizeof(char*));
		this->items[i] = (char*)malloc((strlen(arr[i]) + 1) * sizeof(char));
		strcpy(this->items[i], arr[i]);
	}

	this->nitems = narr;
	shown_range[0] = 0;
	shown_range[1] = MIN(height - 3, narr - 1);

	if (shown_range[1] < 0)
		shown_range[1] = 0;
}	

scrollWin::~scrollWin()
{
	int i;

	for (i = 0; i < nitems; i++)
	{
		free((this->items)[i]);
	}
	free(items);
	free(sw_emptyline);
	if (sw_selected_items)
		free(sw_selected_items);
	if (sw_title)
		free(sw_title);

	delwin(sw_win);
}

/* deletes *all* items in the scroll-window. */
void scrollWin::delItems()
{
	if (nitems)
	{
		for (int i = nitems; i--;)
			free(items[i]);
		free(items);
		items = NULL;
		nitems = 0;
	}

	if (sw_selected_items)
	{
		free(sw_selected_items);
		sw_selected_items = NULL;
	}
	sw_selection = 0;
	nselected = 0;
	shown_range[0] = shown_range[1] = 0;

	if (sw_title)
	{
		free(sw_title);
		sw_title = NULL;
	}
	swRefresh(2);
}

void scrollWin::init(int lines, int ncols, int begin_y, int begin_x, 
                     short showpath, short x_offset)
{
	sw_win = newwin(lines, ncols, begin_y, begin_x);
	leaveok(sw_win, TRUE);
	width = ncols;
	height = lines;
	sw_selection = 0;
	keypad(sw_win, TRUE);
	this->items = NULL;
	sw_title = NULL;
	nitems = 0;
	xoffset = x_offset;
	sw_playmode = 0; /* normal (non-random) play mode */
	shown_range[0] = 0;
	shown_range[1] = 0;
	this->showpath = showpath;

	/* create empty line */
	sw_emptyline = (char *)malloc((width - 1 - x_offset + 1 ) *
		sizeof(char));
	for (int i = 0; i < width - 1 - x_offset; i++)
		sw_emptyline[i] = ' ';
	sw_emptyline[width - 2] = '\0';

	sw_selected_items = NULL;
	nselected = 0;
	for (int i = 0; i < 8; i++)
		border[i] = 0;
	want_border = 0;
}

void
scrollWin::invertSelection()
{
	if (!nitems)
		return;

	int
		i,
		*its = NULL,
		ni2;
		
	its = (int *)malloc(nitems * sizeof(int));

	for (i = 0; i < nitems; i++)
		its[i] = 1;

	ni2 = nitems;

	if ( sw_selected_items )	
	{
		ni2 = nitems - nselected;

		for (i = 0; i < nselected; i++)
			its[sw_selected_items[i]] = 0;

		free(sw_selected_items);
		sw_selected_items = NULL;
	}

	nselected = ni2;
	sw_selected_items = (int *)malloc(ni2 * sizeof(int));

	ni2 = 0;

	for (i = 0; i < nitems; i++)
	{
		if ( its[i] )
			sw_selected_items[ni2++] = i;
	}

	free(its);
	swRefresh(1);	
}

void
scrollWin::setTitle(const char *tmp)
{
	if (sw_title)
		free(sw_title);

	sw_title = (char *)malloc(strlen(tmp) + 1);
	
	strcpy(sw_title, tmp);
}

/* FunctionName : swRefresh
 * Description  : Redraws selection-window. Usually called after 
 *              : creating and initializing the window or when the highlighted
 *              : selection-bar is to be replaced.
 * Arguments    : scroll: Whether or not the contents of the window scroll 
 *              :       : up/down (non-zero if they do), or if the window 
 *              :       : needs to be redrawn. If scroll == 2 the window will
 *              :       : be erased completely before anything's added to it.
 */
void scrollWin::swRefresh(short scroll)
{
	int i;

	if (scroll == 2)
	{
		werase(this->sw_win);
		if (want_border)
			wborder(this->sw_win, border[0], border[1], border[2], border[3],
				border[4], border[5], border[6], border[7]);
	}

	for (i = 1; i < MIN((this->height) - 1, this->nitems -
		this->shown_range[0] + 1); i++)
	{
		int
			item_index = (i + this->shown_range[0]) - 1;
		char *item_name;

		/* erase current line if needed */
		if (scroll != 2 && ((item_index == this->sw_selection) || scroll))
			mvwaddstr(this->sw_win, i, this->xoffset, this->sw_emptyline);

		if (this->showpath)
			item_name = this->items[item_index];
		else
		{
			char
				*last_slash = strrchr(this->items[item_index], '/');
			int
				begin;

			if (!last_slash) /* no '/' in this->items[item_index] */
				begin = 0;
			else
				begin = strlen(this->items[item_index]) - strlen(last_slash) +
					1;

			item_name = this->items[item_index] + begin;
		}
	
		mvwaddnstr(this->sw_win, i, this->xoffset, item_name, this->width -
			1 - this->xoffset);

		if ( item_index != this->sw_selection && isSelected(item_index) )
		{
			unsigned int
				maxx, maxy,
				drawlen;
			
			getmaxyx(this->sw_win, maxy, maxx);

			drawlen = MIN(strlen(item_name), (maxx - 1 - this->xoffset));

			mvwchgat(this->sw_win, i, this->xoffset, drawlen, A_BOLD, 1, NULL);
		}

		if (item_index == this->sw_selection)
		{
			int
				maxx, maxy,
				drawlen,
				colour = 1;
			unsigned int
				tmp;
			
			getmaxyx(this->sw_win, maxy, maxx);
			tmp = maxx - 1 - this->xoffset;

			drawlen = MIN(strlen(item_name), tmp);

			/* draw highlighted selection bar */
			if ( isSelected(item_index) )
				++colour;
			mvwchgat(this->sw_win, i, this->xoffset, drawlen,
				A_REVERSE, colour, NULL);
		}
	}
	if (scroll)
		touchwin(this->sw_win);

	wrefresh(this->sw_win);
}

/* is 'which' amongst sw->sw_selected_items[] ?
 * (i.e. is this->items[which] selected ?)
 */
int scrollWin::isSelected(int which)
{
	int i;

	for (i = 0; i < this->nselected; i++)
	{
		if (this->sw_selected_items[i] == which)
			return 1;
	}
	
	return 0;
}

/* Function Name: scrollWin::changeSelection
 * Description  : the currently selected item in sw is increased (or 
 *              : decreased if change < 0) with change. The window is 
 *              : redrawn and shows the highlighted bar over the new
 *              : selection.
 * Argument     : change: selection-modifier.
 */
void scrollWin::changeSelection(int change)
{
	int
		new_selection = this->sw_selection + change;
	short
		need_to_scroll = 0;

	if (!this->nitems) /* empty itemlist */
		return;

	/* remove the highlighted bar from the old selection */
	mvwchgat(this->sw_win, (this->sw_selection - this->shown_range[0]) + 1,
		this->xoffset, this->width - 1 - this->xoffset, A_NORMAL, 1, NULL);

	if (new_selection >= this->nitems)
		new_selection = 0;
	if (new_selection < 0)
		new_selection = this->nitems - 1;

	if (new_selection < this->shown_range[0])
	{
		this->shown_range[0] = new_selection;
		this->shown_range[1] = MIN(new_selection + (this->height - 3),
			this->nitems - 1);
		++need_to_scroll;
	}
	else if (new_selection > this->shown_range[1])
	{
		this->shown_range[1] = new_selection;
		this->shown_range[0] = new_selection - (this->height - 3);
		if (this->shown_range[0] < 0) 
		{
			/* this happens when an item has just been added while there were
			 * less than this->height - 3 items before.
			 */
			this->shown_range[0] = 0;
		}
		++need_to_scroll;
	}

	this->sw_selection = new_selection;
	swRefresh(need_to_scroll);
}

/* deselect an item in this object. Usually the item to
 * deselect is this->items[this->selection].
 */
void scrollWin::deselectItem(int which)
{
	int
		index = -1,
		need_redraw = 0,
		i;
	
	for (i = 0;  i < this->nselected; i++)
	{
		if (this->sw_selected_items[i] == which)
		{
			index = i;
			break;
		}
	}
	if (index < 0) /* this->items[which] not selected */
		return;
	
	if (which != this->sw_selection)
		++need_redraw;

	/* shrink sw_selected_items array */
	for (i = index; i < this->nselected - 1; i++)
		this->sw_selected_items[i] = this->sw_selected_items[i + 1];
	
	if ( !(--(this->nselected)))
	{
		free(this->sw_selected_items);
		this->sw_selected_items = NULL;
	}
	else
		this->sw_selected_items = (int *)realloc(this->sw_selected_items,
			(this->nselected) * sizeof(int));
	
	swRefresh(need_redraw);
}

/* [de]select sw->sw_selection
 * sw->sw_window will be updated.
 */
void scrollWin::selectItem()
{
	if (!this->nitems) /* nothing to select */
		return;

	if ( isSelected(this->sw_selection) ) /* already selected */
	{
		deselectItem(this->sw_selection);
		return;
	}

	this->sw_selected_items = (int *)realloc(this->sw_selected_items, 
		(++(this->nselected)) * sizeof(int));
	this->sw_selected_items[this->nselected - 1] = this->sw_selection;

	swRefresh(0);
}

/* addItem adds an item to the selection-list of this object.
 * It is advised to call swRefresh after [a series of] addItem[s].
 */
void scrollWin::addItem(const char *item)
{
	int maxy, maxx;
	
	if (!item || !strlen(item))
		return;
	
	this->items = (char **)realloc(this->items, (this->nitems + 1) *
		sizeof(char*));
	this->items[(this->nitems)++] = (char *)malloc(strlen(item) + 1);
	strcpy(this->items[this->nitems - 1], item);

	getmaxyx(this->sw_win, maxy, maxx);
	
	if (this->nitems > 1 && this->shown_range[1] < maxy - 3)
		++(this->shown_range[1]);
}

/* delItem removes this->sw_items[item_index] and redraws selection window
 * 1997.09.07: Now also updates sw_selected_items.
 */
void scrollWin::delItem(int item_index)
{
	int
		i;
		
	if ( !this->items || (item_index + 1) > this->nitems || item_index < 0)
		return;

	if ( isSelected(item_index) )
		deselectItem(item_index);

	/* update sw_selected_items */
	for (i = 0; i < this->nselected; i++)
	{
		if (this->sw_selected_items[i] > item_index)
			--(this->sw_selected_items[i]);
	}
	
	for (i = item_index; i < (this->nitems - 1); i++)
	{
		free(this->items[i]);
		this->items[i] = (char *)malloc(strlen(this->items[i + 1]) + 1);
		strcpy(this->items[i], this->items[i + 1]);
	}
	
	free(this->items[this->nitems - 1]);

	if ( !(--(this->nitems)) )
	{
		free(this->items);
		this->items = NULL;
		this->sw_selection = 0;
		this->shown_range[0] = 0;
		this->shown_range[1] = 0;
		mvwaddstr(this->sw_win, 1, 1, this->sw_emptyline);
		swRefresh(1);
		return;
	}
	else
	{
		this->items = (char **)realloc(this->items, this->nitems *
			sizeof(char *));
	}

	/* check old situation */
	/* this case never happens when someone removes an on-screen [mp3]file
	 * And therefore I (Bram) removed it.
	if (item_index < this->shown_range[0]) 
	{
		--(this->shown_range[0]);
		--(this->sw_selection);
	}
	 */

	/*
	if (this->shown_range[1] >= this->nitems)
	{
		this->shown_range[1] = this->nitems - 1;
	}
	*/
	if (this->shown_range[0] > 0 && this->shown_range[1] > this->nitems -1)
	{
		--(this->shown_range[0]);
		--(this->shown_range[1]);
	}
	else if (this->shown_range[1] > this->nitems - 1)
	{
		/* shown_range[1] always was drawn on the window. Erase that line */
		mvwaddstr(this->sw_win, 1 + this->shown_range[1], 1, 
			this->sw_emptyline);
		--(this->shown_range[1]);	
	}		

	if (this->sw_selection >= this->nitems)
		--(this->sw_selection);
	if (this->sw_selection < this->shown_range[0])
		this->sw_selection = this->shown_range[0];

	swRefresh(1);
}

void scrollWin::pageDown()
{
	int
		maxx, maxy,
		sw_relative_sel,
		old_selection, new_selection;

	if (!(this->nitems))
		return;

	/* 1 special case */
	if (this->shown_range[1] == this->nitems - 1)
	{
		int
			change = ( this->nitems - 1 ) - this->sw_selection;

		if (change < 0)
			change = 0;
		changeSelection(change);
		return;
	}

	getmaxyx(this->sw_win, maxy, maxx);

	sw_relative_sel = this->sw_selection - this->shown_range[0];
	old_selection = this->sw_selection;
	if (sw_relative_sel < 0) /* can't really happen.. */
		sw_relative_sel = 0;

	//sprintf(dummy,"[0]=%d,[1]=%d",this->shown_range[0],this->shown_range[1]);
	//Error(dummy);

	this->shown_range[0] += (maxy - 3);
	this->shown_range[1] = this->shown_range[0] + (maxy - 3);
	new_selection = this->shown_range[0] + sw_relative_sel;

	/* doing it the "easy" way now.. */
	if (this->shown_range[0] < 0)
		this->shown_range[0] = 0;
	if (this->shown_range[1] > (this->nitems - 1))
		this->shown_range[1] = this->nitems - 1;
	if (this->shown_range[1] < 0)
		this->shown_range[1] = 0;
	if (new_selection > this->shown_range[1] || new_selection <
		this->shown_range[0])
		new_selection = this->shown_range[1];
	//sprintf(dummy,"[0]=%d,[1]=%d",this->shown_range[0],this->shown_range[1]);
	//Error(dummy);

	changeSelection(new_selection - old_selection);

	swRefresh(2);
}

/* Once to be merged with scrollWin::pageDown...(find the 10 diffs! ;) */
void scrollWin::pageUp()
{
	int
		maxx, maxy,
		sw_relative_sel,
		old_selection, new_selection;

	if (!(this->nitems))
		return;

	/* 1 special case */
	if (this->shown_range[0] == 0)
	{
		int
			change = 0 - this->sw_selection;
		changeSelection(change);
		return;
	}

	getmaxyx(this->sw_win, maxy, maxx);

	sw_relative_sel = this->sw_selection - this->shown_range[0];
	old_selection = this->sw_selection;
	if (sw_relative_sel < 0) /* can't really happen.. */
		sw_relative_sel = 0;

	//sprintf(dummy,"[0]=%d,[1]=%d",this->shown_range[0],this->shown_range[1]);
	//Error(dummy);

	this->shown_range[0] -= (maxy - 3);
	this->shown_range[1] = this->shown_range[0] + (maxy - 3);
	new_selection = this->shown_range[0] + sw_relative_sel;

	if (this->shown_range[0] < 0)
	{
		this->shown_range[0] = 0;
		this->shown_range[1] = maxy - 3;
		new_selection = sw_relative_sel;
	}
	if (this->shown_range[1] > (this->nitems - 1))
		this->shown_range[1] = this->nitems - 1;
	if (this->shown_range[1] < 0)
		this->shown_range[1] = 0;
	if (new_selection > this->shown_range[1] || new_selection <
		this->shown_range[0])
		new_selection = this->shown_range[1];
	//sprintf(dummy,"[0]=%d,[1]=%d",this->shown_range[0],this->shown_range[1]);
	//Error(dummy);

	changeSelection(new_selection - old_selection);

	swRefresh(2);
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
/* This should be part of a special mp3-class that inherits the standard
 * scrollWin class!! (some day, that is ... :-)
 */
int scrollWin::writeToFile(FILE *f)
{
	char
		header[] = "GROUPNAME: ",
		*group_header = (char *)malloc((strlen(header) +
			strlen(this->sw_title) + 2) * sizeof(char));
	int i;
	
	strcpy(group_header, header);
	strcat(group_header, this->sw_title);
	strcat(group_header, "\n");
	
	if (!fwrite(group_header, sizeof(char), strlen(group_header), f))
		return 0;
	
	for (i = 0; i < this->nitems; i++)
	{
		if (!fwrite(this->items[i], sizeof(char), strlen(this->items[i]), f))
			return 0;
		if (!fwrite("\n", sizeof(char), 1, f))
			return 0;
	}

	return 1; /* everything succesfully written. */
}

/* returns the item (string) that is currently highlighted by this window's
 * scrollbar into the char* item. 
 * Please remember to delete[] (not free()!) the returned string when you stop
 * using it, since it will be alloced by this function!
 */
char* scrollWin::getSelectedItem()
{
	char *item = NULL;

	if (!nitems || sw_selection < 0 || sw_selection >= nitems)
		return (char*)NULL;

	item = new char[strlen(items[sw_selection]) + 1];
	return strcpy(item, items[sw_selection]);
}

/* returns all items (array of strings) contained by the scroll-window (ALL
 * items, i.e. including those that are not on-screen when this function's
 * called). PLEASE note that a COPY of the array is returned, and not the
 * pointer as used by this class. this COPY is alloc'ed using C++'s new.
 * So if you want to stop using the returned array, please delete[] all strings
 * in it, and then delete[] the array-pointer as well.
 */
char **scrollWin::getItems()
{
	char **arr = NULL;

	if (!nitems)
		return arr;

	arr = new char*[nitems];

	for (int i = 0; i < nitems; i++)
	{
		arr[i] = new char[strlen(items[i]) + 1];
		strcpy(arr[i], items[i]);
	}
	return arr;
}

/* returns the total number of items contained by the scroll-window (it's the
 * number of elements in the string-array as returned by scrollWin::getItems)
 */
int scrollWin::getNitems()
{
	return nitems;
}

/* the returned string is malloc'ed by this function using C++'s new. Don't
 * forget to delete[] (not free()!) it after use.
 */
char *scrollWin::getItem(int index)
{
	char *item = NULL;

	if (index < 0 || index >= nitems)
		return NULL;

	item = new char[strlen(items[index]) + 1];
	return strcpy(item, items[index]);
}

/* Returns an array of strings, representing all selected items in this window.
 * Each string and the string-array are allocated using C++'s new. Oh yeah,
 * the order of the selected items is the order in which they have been selected
 * by the user or program operating on this class. Ain't that nice :) And ..
 * The amount of strings is stored in itemcount. Great huh.
 */
char **scrollWin::getSelectedItems(int *itemcount)
{
	int i;

	char **selitems = NULL;

	if (!nselected)
	{
		*itemcount = 0;
		return NULL;
	}

	selitems = new char*[nselected];
	
	for (i = 0; i < nselected; i++)
	{
		char
			*pruts = items[sw_selected_items[i]];

		selitems[i] = new char[strlen(pruts) + 1];
		strcpy(selitems[i], pruts);
	}

	*itemcount = nselected;
	return selitems; /* don't forget to delete[] the string + array! */
}

void
scrollWin::setBorder(chtype ls, chtype rs, chtype ts, chtype bs, 
                     chtype tl, chtype tr, chtype bl, chtype br)
{
	border[0] = ls; border[1] = rs; border[2] = ts; border[3] = bs;
	border[4] = tl; border[5] = tr; border[6] = bl; border[7] = br;

	wborder(sw_win, ls, rs, ts, bs, tl, tr, bl, br);
	want_border = 1;
	wrefresh(sw_win);
}

char *
scrollWin::getTitle()
{
	if (!sw_title)
		return NULL;

	char *tit = new char[strlen(sw_title) + 1];
	return strcpy(tit, sw_title);
}
