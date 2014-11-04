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
 *
 * ?? ??? 1996: Initial creation.
 * 24 Jan 2000: getSelectedItem() returns a pointer i/o of a deep copy.
 * 13 Feb 2000: struct winobj is used for each item now, instead of a char
 *            : pointer.
 * 26 Feb 2000: InsertItem added, writeToFile removed (to other class)
 * 27 Feb 2000: Rewrote selection system such that the items keep track
 *            : of their selection status i/o having this class do that.
 * 27 Feb 2000: Added moveItem and moveSelectedItems
 * 11 May 2000: Fixed a bug in ::delItem() that forgot to delete the first
 *            : char when no border was used. Geez, that must have been in
 *            : here since 1996(!)
 * 11 May 2000: Now this class doesn't use a separate ncurses window object
 *            : anymore. Instead, it draws on stdscr in the same area the
 *            : windowed version would have done. It should be more compatible
 *            : with scary versions of ncurses too.
 * 16 May 2000: Added itemWidth() function.
 * TODO: Remove the debug() calls once the new scrollwin code seems stable.
 */
#include "mp3blaster.h"
#include NCURSES
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
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
 *              : begin_x : as begin_y, but now x-coordinate.
 *              : items   : array of strings containing items for this window
 *              : nitems  : amount of items.
 *              : color   : Default colourpair to use
 *              : x_offset: 1 if window has a (left-)border of 1, 0 otherwise.
 */
scrollWin::scrollWin(int lines, int ncols, int begin_y, int begin_x,
                     char **arr, int narr, short color, short x_offset)
{
	int i;

	init(lines, ncols, begin_y, begin_x, color, x_offset);

	/* memory for array 'items' is allocated the conventional 'C' way, because
	 * it's a lot easier to increase the array than using vectors or similar..
	 */
	for (i = 0; i < narr; i++)
		addItem(arr[i]);

	nitems = narr;
	shown_range[0] = 0;
	shown_range[1] = MIN(height - 3, narr - 1);
	if (shown_range[1] < 0)
		shown_range[1] = 0;
}	

scrollWin::~scrollWin()
{
	winItem *tmp = first;

	while (tmp)
	{
		winItem *newtmp = tmp->next;
		delete tmp;
		tmp = newtmp;
	}
	delete[] sw_emptyline;
	if (sw_title)
		free(sw_title);
}

/* deletes *all* items in the scroll-window. */
void scrollWin::delItems()
{
	winItem *tmp = first;
	while (tmp)
	{
		winItem *newtmp = tmp->next;
		delete tmp;
		tmp = newtmp;
	}
	nitems = 0;
	first = last = NULL;

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
                     short color, short x_offset)
{
	by = begin_y;
	bx = begin_x;
	width = ncols;
	height = lines;
	sw_selection = nitems = 0;
	hide_bar = 0;
	first = last = NULL;
	sw_title = NULL;
	xoffset = x_offset;
	shown_range[0] = shown_range[1] = 0;
	colour = color;
	//wbkgd(sw_win, COLOR_PAIR(colour)|A_BOLD);

	/* create empty line */
	sw_emptyline = new char[(width - (xoffset ? 2 : 0)) + 1];
	memset((void*)sw_emptyline, ' ', (width - (xoffset ? 2 : 0)) * sizeof(char));
	sw_emptyline[width - (xoffset?2:0)] = '\0';

	nselected = 0;
	for (int i = 0; i < 8; i++)
		border[i] = 0;
	want_border = 0;
	dispindex = 0;
	selectID = 1;
}

void
scrollWin::invertSelection()
{
	winItem *tmp = first;
	selectID = 1; //since all items to be selected will be selected anew,
	              //it's safe to reset selectID to 1.
	while (tmp)
	{
		if (tmp->selected())
			tmp->deselect();
		else
			tmp->select(selectID++);
		tmp = tmp->next;
	}
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
	winItem *current = NULL;

	if (scroll == 2)
	{
		clearwin();
		if (want_border)
			drawBorder();
	}
	for (i = (xoffset ? 1 : 0); i < MIN((height) - (xoffset ? 1 : 0), nitems -
		shown_range[0] + 1); i++)
	{
		int
			item_index = (i + shown_range[0]) - (xoffset ? 1 : 0),
			tmp_dispindex = dispindex;
		const char *item_name;

		current = (current ? current->next : getWinItem(item_index));
		if (!current)
		{
			char bla[50];sprintf(bla,"swRefresh(%d): no next item!!!\n",
				item_index);debug(bla);
			continue;
		}
		
		/* erase current line if needed */
		if (scroll == 1 || item_index == sw_selection)
		{
			move(by + i,bx + xoffset);
			addstr(sw_emptyline);
		}

		//determine which itemname to choose.
		//If an item does not have a name for dispindex, find the name
		//with the highest possible index below dispindex
		while (tmp_dispindex > -1 && !current->getName(tmp_dispindex))
			tmp_dispindex--;
		if (tmp_dispindex < 0) //No itemname, shouldn't happen. Ever.
		{
			item_name = sw_emptyline;
			debug("swRefresh: nameless item\n");
		}
		else
			item_name = current->getName(tmp_dispindex);

		int
			maxdrawlen = width - (xoffset ? 2 : 0),
			drawlen = MIN(strlen(item_name), maxdrawlen);
		short kleur = current->getColour();	
		attr_t drawmode = (item_index == sw_selection  && !hide_bar ?
		                  A_REVERSE : A_NORMAL);

		if (kleur == -1)
			kleur = colour;
		if (current->selected())
			drawmode |= A_BOLD;
		
		//mvwaddnstr(sw_win, i, xoffset, item_name, drawlen);
		attrset(drawmode|COLOR_PAIR(kleur));
		move(by + i, bx + xoffset); addnstr(item_name, drawlen);
		attrset(COLOR_PAIR(CP_DEFAULT)|A_NORMAL);
		//mvwchgat(sw_win, i, xoffset, drawlen, drawmode, kleur, NULL);
	}
	if (scroll)
		touchwin(stdscr);

	refresh();
}
 
/* Function   : isSelected
 * Description: Checks if item at index `which' is selected.
 * Parameters : which: index of item to check
 * Returns    : non-zero if item's selected, zero otherwise.
 * SideEffects: None.
 */
int scrollWin::isSelected(int which)
{
	winItem *tmp = getWinItem(which);
	if (tmp && tmp->selected())
		return 1;
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
	short kleur;
	winItem *tmp;

	if (!this->nitems || !change) /* empty itemlist or no change */
		return;

	/* remove the highlighted bar from the old selection */
	kleur = ((tmp = getWinItem(sw_selection)) ?
		tmp->getColour() : -1);
	if (kleur == -1)
		kleur = colour;
	//mvwchgat(this->sw_win, (this->sw_selection - this->shown_range[0]) + 1,
	//	this->xoffset, this->width - 1 - this->xoffset, A_NORMAL, kleur, NULL);

	if (new_selection >= this->nitems)
		new_selection = 0;
	if (new_selection < 0)
		new_selection = this->nitems - 1;

	if (new_selection < this->shown_range[0])
	{
		this->shown_range[0] = new_selection;
		this->shown_range[1] = MIN(new_selection + (this->height - (xoffset?3:1)),
			this->nitems - 1);
		++need_to_scroll;
	}
	else if (new_selection > this->shown_range[1])
	{
		this->shown_range[1] = new_selection;
		this->shown_range[0] = new_selection - (this->height - (xoffset?3:1));
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
	winItem *tmp = getWinItem(which);
	
	if (tmp && tmp->selected())
		tmp->deselect();

	//TODO: Add a parameter to swRefresh that will make it only redraw 1 item.
	//And a function that determines if an item is onscreen..
	swRefresh(1);
}

/* [de]select sw_selection
 * sw->sw_window will be updated.
 */
void scrollWin::selectItem()
{
	winItem *tmp = getWinItem(sw_selection);

	if (!nitems || !tmp) /* nothing to select */
		return;

	//TODO: Similar to sw_selection, store the currently hilit winItem object
	//in a global variable too. This prevents unnecessary calls to getWinItem.
	if ( tmp->selected() ) /* already selected */
		tmp->deselect();
	else
		tmp->select(selectID++);

	swRefresh(0);
}

short
scrollWin::addItem(const char *item, short status, short colour, int index,
                   short before, void *object, itemtype type)
{
	const char *bla[2] = { item, NULL };
	return addItem(bla, &status, colour, index, before, object, type);
}

/* addItem adds an item to the selection-list of this object.
 * item is a list of descriptions for the item, status an array for
 * the indexes used for the descriptions. item must contain one more
 * element than status, the last element being NULL.
 * It is advised to call swRefresh after [a series of] addItem[s].
 */
short
scrollWin::addItem(const char **item, short *status, short colour, int index,
                   short before, void *object, itemtype type)
{
	int i = 0;

	winItem *tmp;
	if (!item || !status)
		return 0;

	tmp = new winItem;
	while (item[i])	
	{
		tmp->setName(item[i], status[i]);
		i++;
	}
	tmp->setColour(colour);
	if (object)
		tmp->setObject(object, type);
	return addItem(tmp, index, before);
}

short
scrollWin::addItem(winItem *newitem, int index, short before)
{
	if (!first) //special case
	{
		first = newitem;
		first->prev = NULL;
		first->next = NULL;
		last = first;
	}
	else
	{
		winItem *tmp = (index == -1 ? last : getWinItem(index));

		if (!tmp)
		{
			debug("Tried to add an item before/after non-existant item.\n");
			return 0;
		}
		if (before)
		{
			newitem->prev = tmp->prev;
			if (tmp->prev)
				(tmp->prev)->next = newitem;
			else //replacement of first
				first = newitem;
			tmp->prev = newitem;
			newitem->next = tmp;
		}
		else
		{
			if (tmp->next)
				(tmp->next)->prev = newitem;
			else
				last = newitem;
			newitem->next = tmp->next;
			newitem->prev = tmp;
			tmp->next = newitem;
		}
		if (last->next)
		{
			debug("Item following last is impossible\n");
			//exit(1);
		}
	}
	nitems++;

	if (this->nitems > 1 && this->shown_range[1] < height - (xoffset?3:1))
		++(this->shown_range[1]);
	return 1;
}

/* changeItem changes an item. It does not destroy the winItem object.
 * It is advised to call swRefresh after [a series of] changeItem[s].
 */
void scrollWin::changeItem(int item_index, const char *item, short nameindex)
{
	winItem *tmp;
	if ( !first || (item_index + 1) > nitems || item_index < 0 ||
		!item || !strlen(item))
		return;

	tmp = getWinItem(item_index);
	if (tmp)
		tmp->setName(item, nameindex);
	else
		debug("changeItem on non-existant item. Duh..?\n");
}

void
scrollWin::replaceItem(int item_index, winItem *newitem)
{
	winItem *tmp;
	if ( !first || (item_index + 1) > nitems || item_index < 0 ||
		!newitem)
		return;

	tmp = getWinItem(item_index);
	if (tmp)
	{
		delete tmp;
		tmp = newitem;
	}
	else
		debug("changeItem on non-existant item. Duh..?\n");
}
 
/* Function   : delItem
 * Description: Remove item at `item_index' from linked list. If del != 0,
 *            : delete the winItem. If del = 0, don't delete it; just set
 *            : its next/prev to NULL and deselect it if it was selected.
 * Parameters : item_index, del: See Description.
 * Returns    : Nothing.
 * SideEffects: See Description.
 */
void scrollWin::delItem(int item_index, int del)
{
	int i;
	winItem *tmp;
		
	if ( !first || (item_index + 1) > nitems || item_index < 0)
		return;

	//debug
	if (!first)
	{
		debug("No first while deleting element impossible!\n");
		exit(1);
	}
	//perform actual deletion;
	i = 0; tmp = first;
	while (i != item_index && tmp->next)
	{
		tmp = tmp->next;
		i++;
	}
	if (i != item_index)
	{
		debug("delItem: Could not find item to delete.\n");
		return;
	}
	if (tmp->prev)
		(tmp->prev)->next = tmp->next;
	else //this was `first'!
	{
		if (tmp->next)
			first = tmp->next;
		else
			first = NULL;
	}
	if (tmp->next)
		(tmp->next)->prev = tmp->prev;
	else //this was `last'
	{
		if (tmp->prev)
			last = tmp->prev;
		else
			last = NULL;
	}
	if (del)
		delete tmp;
	else
	{
		tmp->next = tmp->prev = NULL;
		if (tmp->selected())
			tmp->deselect();
	}
	if ((nitems - 1) != 0 && (!first || !last))
	{
		debug("delItem: No first or last after deletion!\n");
	};

	//no more items left?
	if ( !(--nitems) )
	{
		first = last = NULL;
		this->sw_selection = 0;
		this->shown_range[0] = 0;
		this->shown_range[1] = 0;
		//mvwaddstr(this->sw_win, 1, xoffset, sw_emptyline);
		move(by + (xoffset?1:0), bx + xoffset);
		addstr(sw_emptyline);
		swRefresh(1);
		return;
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

	/* perform scary screen management */
	if (this->shown_range[0] > 0 && this->shown_range[1] > this->nitems -1)
	{
		--(this->shown_range[0]);
		--(this->shown_range[1]);
	}
	else if (this->shown_range[1] > this->nitems - 1)
	{
		/* shown_range[1] always was drawn on the window. Erase that line */
		//mvwaddstr(this->sw_win, 1 + this->shown_range[1], xoffset, sw_emptyline);
		move(by + this->shown_range[1] + (xoffset?1:0), bx + xoffset);
		addstr(sw_emptyline);
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

	sw_relative_sel = this->sw_selection - this->shown_range[0];
	old_selection = this->sw_selection;
	if (sw_relative_sel < 0) /* can't really happen.. */
		sw_relative_sel = 0;

	//sprintf(dummy,"[0]=%d,[1]=%d",this->shown_range[0],this->shown_range[1]);
	//Error(dummy);

	this->shown_range[0] += (height - (xoffset?3:1));
	this->shown_range[1] = this->shown_range[0] + (height - (xoffset?3:1));
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

	sw_relative_sel = this->sw_selection - this->shown_range[0];
	old_selection = this->sw_selection;
	if (sw_relative_sel < 0) /* can't really happen.. */
		sw_relative_sel = 0;

	this->shown_range[0] -= (height - (xoffset?3:1));
	this->shown_range[1] = this->shown_range[0] + (height - (xoffset?3:1));
	new_selection = this->shown_range[0] + sw_relative_sel;

	if (this->shown_range[0] < 0)
	{
		this->shown_range[0] = 0;
		this->shown_range[1] = height - (xoffset?3:1);
		new_selection = sw_relative_sel;
	}
	if (this->shown_range[1] > (this->nitems - 1))
		this->shown_range[1] = this->nitems - 1;
	if (this->shown_range[1] < 0)
		this->shown_range[1] = 0;
	if (new_selection > this->shown_range[1] || new_selection <
		this->shown_range[0])
		new_selection = this->shown_range[1];

	changeSelection(new_selection - old_selection);

	swRefresh(2);
}

/* returns the item (string) that is currently highlighted by this window's
 * scrollbar into the char* item. 
 */
const char* scrollWin::getSelectedItem(short nameindex)
{
	if (!nitems || sw_selection < 0 || sw_selection >= nitems)
		return (const char*)NULL;

	return getItem(sw_selection, nameindex);
}

/* returns all items (array of strings) contained by the scroll-window (ALL
 * items, i.e. including those that are not on-screen when this function's
 * called). PLEASE note that a COPY of the array is returned, and not the
 * pointer as used by this class. this COPY is alloc'ed using C++'s new.
 * So if you want to stop using the returned array, please delete[] all strings
 * in it, and then delete[] the array-pointer as well.
 */
char **scrollWin::getItems(short nameindex)
{
	char **arr = NULL;
	winItem *tmp = first;
	int i = 0;
	if (!nitems || !tmp)
		return arr;

	arr = new char*[nitems];

	while (i < nitems && tmp)
	{
		const char *bla = tmp->getName(nameindex);
		arr[i] = new char[strlen(bla)+1];
		strcpy(arr[i], bla);
		i++; tmp = tmp->next;
	}
	if (i != nitems)
		debug("getItems: nitems != aantal items?\n");

	return arr;
}

/* returns the total number of items contained by the scroll-window (it's the
 * number of elements in the string-array as returned by scrollWin::getItems)
 */
int scrollWin::getNitems()
{
	return nitems;
}

const char *
scrollWin::getItem(int index, short name_index)
{
	winItem *tmp;
	if ( (tmp = getWinItem(index)))
		return tmp->getName(name_index);
	else
		return NULL;
}

winItem * 
scrollWin::getWinItem(int index)
{
	winItem *tmp = first;
	int i = 0;

	if (index < 0 || index >= nitems || !first)
		return NULL;

	while (i != index && tmp->next)
	{
		tmp = tmp->next;
		i++;
	}
	if (i == index)
		return tmp;
	return NULL;
}

//Sorts a winItem* array based on the value of the winItem::selected() member.
int
sortwinitems(const void *a, const void *b)
{
	int id1, id2;

	id1 = (*(winItem**)a)->selected();
	id2 = (*(winItem**)b)->selected();
	if (id1 < id2)
		return -1;
	else if (id1 > id2)
		return 1;
	else
		return 0;
}

/* Returns an array of strings, representing all selected items in this window.
 * Each string and the string-array are allocated using C++'s new. Oh yeah,
 * the order of the selected items is the order in which they have been selected
 * by the user or program operating on this class. Ain't that nice :) And ..
 * The amount of strings is stored in itemcount. Great huh. Delete the
 * array and its members with delete[].
 */
char **scrollWin::getSelectedItems(int *itemcount, short nameindex)
{
	int i;
	char **selitems = NULL;
	winItem **sortarr;

	//fetch presorted winItems array
	sortarr = getSelectedWinItems(itemcount);
	if (!*itemcount)
		return selitems;
	
	selitems = new char*[*itemcount];
	for (i = 0; i < *itemcount; i++)
	{
		int tmpindex = nameindex;
		while (tmpindex > -1 && !sortarr[i]->getName(tmpindex))
			tmpindex--;
		if (tmpindex == -1) //should not happen
			selitems[i] = NULL;
		else
		{
			const char *bla = sortarr[i]->getName(tmpindex);
			selitems[i] = new char[strlen(bla)+1];
			strcpy(selitems[i], bla);
		}
	}
	delete[] sortarr; //yes, don't delete the members. That's correct :)
	return selitems; /* don't forget to delete[] the members + array! */
}

/* Returns an array of winItem objects, the last element begin NULL.
 * The elements are sorted by their selection order. Delete[] the array after
 * use, but not the members!
 */
winItem**
scrollWin::getSelectedWinItems(int *itemcount)
{
	int selcount = 0, cnt;
	winItem *tmp = first;
	winItem **sortarr;

	//determine amount of selected items.
	while (tmp)
	{
		if (tmp->selected())
			selcount++;
		tmp = tmp->next;
	}
	tmp = first;
	if (!selcount)
	{
		*itemcount = 0;
		return NULL;
	}

	sortarr = new winItem*[selcount];
	cnt = 0;
	while (tmp)
	{
		if (tmp->selected())
			sortarr[cnt++] = tmp;
		tmp = tmp->next;
	}
	//sort sortarr in selection order.
	qsort(sortarr, selcount, sizeof(winItem*), sortwinitems);
	*itemcount = selcount;
	return sortarr;
}

void
scrollWin::setBorder(chtype ls, chtype rs, chtype ts, chtype bs, 
                     chtype tl, chtype tr, chtype bl, chtype br)
{
	border[0] = ls; border[1] = rs; border[2] = ts; border[3] = bs;
	border[4] = tl; border[5] = tr; border[6] = bl; border[7] = br;

	drawBorder();
	want_border = 1;
	refresh();
}

void
scrollWin::drawBorder()
{
	move(by,bx); addch(border[4]);
	move(by,bx+width-1); addch(border[5]);
	move(by+height-1,bx); addch(border[6]);
	move(by+height-1,bx+width-1); addch(border[7]);
	move(by+height-1,bx+1); hline(border[3],width-2);
	move(by+1,bx); vline(border[0], height-2);
	move(by+1,bx+width-1); vline(border[1], height-2);
	if (!draw_title || !sw_title)
	{
		move(by,bx+1); hline(border[2],width-2);
	}
	else //draw title in border
	{
		int lpad, rpad, i, len;
		char *nt = crunch_string(sw_title, width - 4);
		len = (width - 4 - strlen(nt));
		lpad = rpad = len / 2;
		rpad += len%2;
		move(by,bx+1);
		for (i = 0; i < lpad; i++)
			addch(border[2]);
		addch(ACS_RTEE);
		addstr(nt);
		addch(ACS_LTEE);
		for (i = 0; i < rpad; i++)
			addch(border[2]);
		delete[] nt;
	}
}

const char *
scrollWin::getTitle()
{
	if (!sw_title)
		return NULL;

	return (const char *)sw_title;
}

/* setItem selects this->sw_items[item_index] and redraws selection window
 */
void
scrollWin::setItem(int item_index)
{
	int diff = item_index - sw_selection;
	changeSelection(diff);
}

void
scrollWin::dump_contents()
{
	char overview[100];
	sprintf(overview, "Total items: %d\n", nitems);
	debug(overview);
	winItem *tmp = first;
	int i = 0;
	while (tmp)
	{
		char buff[500];
		sprintf(buff, "%03d: %s\n", i++, tmp->getName(0));
		debug(buff);
		tmp = tmp->next;
	}
}

void
scrollWin::dump_info()
{
	char fiets[100];
	sprintf(fiets, "sw_selection: %3d sr[0]: %3d sr[1]: %3d nitems: %3d\n",
		sw_selection, shown_range[0], shown_range[1], nitems);
	debug(fiets);
}

void
scrollWin::setDisplayMode(short mode)
{
	if (mode < 0 || mode >= NAMEDIM)
		return;
	dispindex = mode;
}

void
scrollWin::setDefaultColor(short color)
{
	if (color < 0 || color > COLOR_PAIRS)
		return;
	colour = color;
}
 
/* Function   : moveItem
 * Description: Moves an item at position `index_from' before or after the
 *            : item at position `index_to' (index_to is the index BEFORE the
 *            : move!)
 * Parameters : index_from, index_to: See Description
 *            : before: 0 if item's to be moved *after* index_to, otherwise
 *            : it's moved *before* index_to.
 * Returns    : Nothing.
 * SideEffects: None. (so call swRefresh(1) yourself)
 */
short
scrollWin::moveItem(int index_from, int index_to, short before)
{
	int new_indexto;
	winItem *tmp;

	if (index_from == index_to || index_from < 0 || index_from >=nitems ||
		index_to < 0 || index_to > nitems)
		return 0;

	//determine itemindex to add to *after* removal of index_from
	if (index_from < index_to)
		new_indexto = index_to - 1;
	else
		new_indexto = index_to;

	if (!(tmp = getWinItem(index_from)))
		return 0;
	
	delItem(index_from, 0); //remove from list without deleting object.
	if (!addItem(tmp, new_indexto, before))	
	{
		//This case should *NOT* happen. 
		debug("moveItem(): Could not add item after removal.\n");
		delete tmp; //no dangling pointers please.	
		return 0;
	}
	return 1;
}
 
/* Function   : moveSelectedItems
 * Description: Moves items that are selected before or after the item at
 *            : `index_to'. The item at `index_to' can not be selected.
 *            : The screen is not refreshed; take care of this yourself!
 * Parameters : index_to: Index of item to move to. (usually sw_selection)
 *            : before: if 0, items are moved *behind* the item at `index_to',
 *            : otherwise *before*.
 * Returns    : Nothing.
 * SideEffects: None.
 */
short
scrollWin::moveSelectedItems(int index_to, short before)
{
	int retval = 1;
	winItem *tmp;
	//find selected item that was selected first.
	//break if there are no more selected items;
	//move the item.
	//determine new index_to

	//index_to must exist and the item there must not be selected itself!
	if (!(tmp = getWinItem(index_to)) || tmp->selected())
		return 0;

	while (1)
	{
		int index = 0;
		int found_at = -1;
		int select_id = 0;

		//find item that was selected first when items are to be moved BEFORE
		//index_to. Otherwise, find the last item.
		tmp = first;
		while (tmp)
		{
			int selid = tmp->selected();
			//find item selected first if before.
			if (selid && before && (selid < select_id || !select_id))
			{
				select_id = selid;
				found_at = index;
			}
			//find item selected last if !before
			else if (selid && !before && (selid > select_id || !select_id))
			{
				select_id = selid;
				found_at = index;
			}
			tmp = tmp->next;
			index++;
		}
		if (found_at == -1)
			break;
		//Found first selected item at index `found_at'
		if (found_at == index_to || !moveItem(found_at, index_to, before))
		{
			retval = 0; break;
		}
		//Item moved. Calculate new index_to.
		if (before && found_at > index_to)
			index_to++;
		else if (!before && found_at < index_to)
			index_to--;
	}
	return retval;
}

void
scrollWin::clearwin()
{
	int i;

	for (i = xoffset; i < height; i++)
	{
		move(by + i, bx + xoffset);
		addstr(sw_emptyline);
	}
}

void
scrollWin::drawTitleInBorder(int should_i)
{
	if (should_i)
		draw_title = 1;
	else
		draw_title = 0;
}
