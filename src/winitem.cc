/* Copyright (C) Bram Avontuur (brama@stack.nl)
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
#include "scrollwin.h"
#include "winitem.h"
#include <stdio.h>
#include <string.h>
extern void debug(const char*,...);

winItem::winItem()
{
	int j;
	for (j = 0; j < NAMEDIM; j++)
		names[j] = NULL;
	object = NULL;
	type = TEXT;
	colour = -1; //-1: let global status (from scrollWin) decide.
	prev = next = NULL;
	selectID = 0;
}

winItem::~winItem()
{
	//yes, if there's an object set, it's not deleted.
}

void
winItem::setName(const char *name, short index)
{
	if (index >= NAMEDIM || index < 0)
		return;

	if (names[index])
		delete[] names[index];
	
	names[index] = new char[strlen(name)+1];
	strcpy(names[index], name);
}

const char *
winItem::getName(short index)
{
	if (index >= NAMEDIM || index < 0)
		return (const char *)NULL;

	return names[index];
}

void
winItem::setColour(short pair)
{
	colour = pair;
}

/* Set this item to represent a subwin. Type is automatically set to
 * SUBWIN (a.o.t. TEXT)
 */
void
winItem::setObject(void *ob, itemtype tp)
{
	if (object)
	{
		if (type == SUBWIN)
			delete (scrollWin*)object;
	}
		
	object = ob;
	type = tp;
}

void
winItem::setType(itemtype newtype)
{
	type = newtype;
}
