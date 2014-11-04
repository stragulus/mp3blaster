/* Copyright (C) Bram Avontuur (bram@avontuur.org)
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
#include <stdlib.h>

extern void debug(const char*,...);

winItem::winItem()
{
	int j;
	for (j = 0; j < NAMEDIM; j++)
	{
		names[j].value=NULL;
		names[j].fetch=NULL;
		names[j].arg=NULL;
	}
	object = NULL;
	type = TEXT;
	colour = -1; //-1: let global status (from scrollWin) decide.
	prev = next = NULL;
	selectID = 0;
}

winItem::~winItem()
{
	int i;
	for(i=0;i<NAMEDIM;i++)
	{
		if(names[i].value)
			delete[] names[i].value;
		if(names[i].arg)
			free(names[i].arg);
	}
	//yes, if there's an object set, it's not deleted.
}

void
winItem::setName(const char *name, short index)
{
	if (index >= NAMEDIM || index < 0)
		return;

	if (names[index].value)
		delete[] names[index].value;
	
	names[index].value = new char[strlen(name)+1];
	strcpy(names[index].value, name);
}

void
winItem::setNameFun(char *(*func)(void *), void *arg, short index)
{
	if(index >=NAMEDIM || index < 0)
		return;
	names[index].value=NULL;
	names[index].fetch=func;
	names[index].arg=arg;
}

const char *
winItem::getName(short index)
{
	if (index >= NAMEDIM || index < 0)
		return (const char *)NULL;
	if(!names[index].value && names[index].fetch)
	{
		char *descr=names[index].fetch(names[index].arg);
		if(descr)
		{
			names[index].value = descr;
		}
		else
		{
			names[index].value = new char[8];
			strcpy(names[index].value, "no name");
		}
		names[index].fetch=NULL; /* Prevent from being run a second time */
		if (names[index].arg)
			free(names[index].arg);
		names[index].arg = NULL;
	}
	return names[index].value;
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
