#include "mp3win.h"
#include "mp3item.h"
#include "mp3blaster.h"
#include <stdio.h>
#include <string.h>

extern void debug(const char*,...);

mp3Win::mp3Win(int lines, int ncols, int begin_y, int begin_x,
               char **arr, int narr, short color, short x_offset) :
			   scrollWin(lines, ncols, begin_y, begin_x, arr, narr,
			   color, x_offset)
{
	playmode = 0;
	played = 0;
	playing = 0;
}

mp3Win::~mp3Win()
{
	debug("~mp3Win()\n");
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
int mp3Win::writeToFile(FILE *f)
{
	char
		header[] = "GROUPNAME: ",
		*group_header = new char[strlen(header) +
			strlen(getTitle()) + 2];
	winItem *tmp;
	
	strcpy(group_header, header);
	strcat(group_header, getTitle());
	strcat(group_header, "\n");
	
	if (!fwrite(group_header, sizeof(char), strlen(group_header), f))
	{
		delete[] group_header;
		return 0;
	}
	delete[] group_header;

	char temp[20];
	sprintf(temp, "PLAYMODE: %1d\n", playmode);
	if (!fwrite(temp, sizeof(char), strlen(temp), f))
		return 0;

	tmp = first;
	while (tmp)
	{
		const char
			*iname = tmp->getName(0); //TODO: nameindex not hardcoded!
		if (!iname)
			iname = "/non/existant/mp3/debug.mp3";
		if (!fwrite(iname, sizeof(char), strlen(iname), f))
			return 0;
		if (!fwrite("\n", sizeof(char), 1, f))
			return 0;
		tmp = tmp->next;
	}

	return 1; /* everything succesfully written. */
}

short
mp3Win::addItem(const char *item, short status, short colour, int index,
                   short before, void *object, itemtype type)
{
	return scrollWin::addItem(item, status, colour, index, before, object,
		type);
}

short
mp3Win::addItem(const char **item, short *status, short colour, int index,
                   short before, void *object, itemtype type)
{
	int i = 0;

	winItem *tmp;
	if (!item || !status)
		return 0;

	tmp = new mp3Item;
	while (item[i])	
	{
		tmp->setName(item[i], status[i]);
		i++;
	}
	tmp->setColour(colour);
	if (object)
		tmp->setObject(object, type);
	return scrollWin::addItem(tmp, index, before);
}

/* Adds a group to the list. Refresh the screen yourself, TYVM */
void
mp3Win::addGroup(mp3Win *newgroup, const char *groupname)
{
	winItem *tmp = first;

	int index = 0;
	short before = 0;

	while (tmp && tmp->getType() == SUBWIN)
	{
		tmp = tmp->next;
		index++;
	}
	if (index)
		index -= 1;
	else if ((tmp = getWinItem(index)) && tmp->getType() != SUBWIN)	
		before = 1;
	addItem((groupname ? groupname : "[Default Group]"), 0, CP_FILE_WIN,
		index, before, newgroup, SUBWIN);	
	newgroup->addItem("[..]", 0, CP_FILE_WIN, 0, 1, this, SUBWIN);
}

/* Make sure subgroups get deleted properly */
void
mp3Win::delItem(int item_index, int del)
{
	winItem *tmp;

	if (!del || !(tmp = getWinItem(item_index)) ||
		(!item_index && !strcmp(tmp->getName(), "[..]"))) //don't del [..]
		return scrollWin::delItem(item_index, del);

	if (tmp->getType() == SUBWIN)
	{
		debug("Deleting subgroup ");
		debug(tmp->getName());
		debug("\n");
		mp3Win *subwin = (mp3Win*)tmp->getObject();
		if (subwin)
			delete subwin;
	}

	return scrollWin::delItem(item_index, del);
}

short
mp3Win::isGroup(int index)
{
	winItem *tmp;

	if ((tmp = getWinItem(index)) && tmp->getType() == SUBWIN)
		return 1;

	return 0;
}

/* Function   : isPreviousGroup
 * Description: Is an entry in the list a link to a group one position 
 *            : higher in the hierarchy?
 * Parameters : None.
 * Returns    : 1 if it is, otherwise 0.
 * SideEffects: None.
 */
short
mp3Win::isPreviousGroup(int index)
{
	return (isGroup(index) && !strcmp(getItem(index), "[..]"));
}

mp3Win*
mp3Win::getGroup(int index)
{
	winItem *tmp = getWinItem(index);

	if (tmp && tmp->getType() == SUBWIN)
	{
		mp3Win *subgroup = (mp3Win*)tmp->getObject();
		return subgroup;
	}
	return NULL;
}

int
mp3Win::getUnplayedSongs(short recursive)
{
	return getUnplayedItems(0, recursive);
}

/* Count the number of subgroups in this group that have not been played
 * and have unplayed songs in them. If this group contains unplayed songs
 * and it's not played, count it as well.
 */
int 
mp3Win::getUnplayedGroups(short recursive)
{
	return getUnplayedItems(1, recursive);
}

/* type=0: unplayed songs
 * type=1: unplayed groups
 */
int
mp3Win::getUnplayedItems(short type, short recursive)
{
	mp3Item *tmp = (mp3Item*)getWinItem(0); //first item.
	int count = 0;

	while (tmp)
	{
		if (tmp->getType() == SUBWIN)
		{
			if (strcmp(tmp->getName(), "[..]")) //beware of recursion.
			{
				mp3Win *win = (mp3Win*)(tmp->getObject());
				if (recursive) //songs
					count += win->getUnplayedItems(type, recursive);
				else if (type == 1 && !win->isPlayed() &&
					win->getUnplayedSongs(0)) //group, non-recursive
					count ++;
			}
		}
		else if (!type && !tmp->isPlayed())
		{
			count++;
		}
		tmp = (mp3Item*)tmp->next;
	}
	//count this group if it's not played and contains unplayed songs.
	if (type == 1 && !isPlayed() && getUnplayedSongs(0))
		count++;
	return count;
}

mp3Win *
mp3Win::getUnplayedGroup(int unplayed_index, short set_played, short recursive)
{
	mp3Item *tmp = (mp3Item*)getWinItem(0);
	int count = 0;

	while (tmp)
	{
		int subgroup_count = 0;
		if (tmp->getType() == SUBWIN)
		{
			mp3Win *tmp_win = (mp3Win*)(tmp->getObject());
			if (strcmp(tmp->getName(), "[..]"))	
			{
				if (recursive)
				{
					subgroup_count = tmp_win->getUnplayedGroups(recursive);
					if (count + subgroup_count > unplayed_index)
					{
						return tmp_win->getUnplayedGroup(unplayed_index - count,
							set_played, recursive);
					}
					else 
						count += subgroup_count;
				}
			}
			/* HUH? Group [..] should never be checked..why did I do this?
			else if (!tmp_win->isPlayed() && tmp_win->getUnplayedSongs())
			{
				if (count == unplayed_index)
				{
					if (set_played)
						tmp_win->setPlayed();
					return tmp_win;
				}
				else
					count++;
			}
			*/
		}
		tmp = (mp3Item*)tmp->next;
	}
	if (count == unplayed_index && !isPlayed() && getUnplayedSongs())
	{
		if (set_played)
			setPlayed();
		return this;
	}
	return NULL;
}

const char *
mp3Win::getUnplayedSong(int unplayed_index, short set_played, short recursive)
{
	mp3Item *tmp = (mp3Item*)getWinItem(0);
	int count = 0;

	while (tmp)
	{
		int subgroup_count = 0;
		if (tmp->getType() == SUBWIN)
		{
			if (recursive && strcmp(tmp->getName(), "[..]"))	
			{
				mp3Win *tmp_win = (mp3Win*)(tmp->getObject());
				subgroup_count = tmp_win->getUnplayedSongs();
				if (count + subgroup_count > unplayed_index)
				{
					return tmp_win->getUnplayedSong(unplayed_index - count, set_played,
						recursive);
				}
				else 
					count += subgroup_count;
			}
		}
		else if (count == unplayed_index && !tmp->isPlayed())
		{
			if (set_played)
				tmp->setPlayed();
			return (const char *)tmp->getName();
		}
		else if (!tmp->isPlayed())
			count++;

		tmp = (mp3Item*)tmp->next;
	}
	return NULL;
}

/* set all songs in this group (and subgroups if recursive) to unplayed */
void
mp3Win::resetSongs(int recursive, short setplayed)
{
	mp3Item *tmp = (mp3Item*)getWinItem(0);

	while (tmp)
	{
		if (tmp->getType() == SUBWIN)
		{
			if (recursive && strcmp(tmp->getName(), "[..]"))
				((mp3Win*)(tmp->getObject()))->resetSongs(recursive, setplayed);
		}
		else if (!setplayed && tmp->isPlayed())
			tmp->setNotPlayed();
		else if (setplayed && !tmp->isPlayed())
			tmp->setPlayed();

		tmp = (mp3Item*)tmp->next;
	}
}

void
mp3Win::resetGroups(int recursive)
{
	mp3Item *tmp = (mp3Item*)getWinItem(0);

	while (tmp)
	{
		if (tmp->getType() == SUBWIN)
		{
			mp3Win *win = (mp3Win*)(tmp->getObject());
			if (strcmp(tmp->getName(), "[..]"))
				win->resetGroups(recursive);
		}
		tmp = (mp3Item*)tmp->next;
	}
	if (isPlayed())
		setNotPlayed();
}

/* is this group (or any of its subgroups) playing? */
short
mp3Win::isPlaying()
{
	mp3Item *tmp = (mp3Item*)getWinItem(0);

	if (playing)
		return 1;

	while (tmp)
	{
		if (tmp->getType() == SUBWIN)
		{
			mp3Win *win = (mp3Win*)(tmp->getObject());
			if (strcmp(tmp->getName(), "[..]") && win->isPlaying())
				return 1;
		}
		tmp = (mp3Item*)tmp->next;
	}
	return 0;
}

/* Function   : setAllSongsPlayed
 * Description: Will set each song's status to played.
 * Parameters : recursive: do the same to each subgroup in this group (except
 *            : for [..])
 * Returns    : Nothing.
 * SideEffects: None.
 */
void
mp3Win::setAllSongsPlayed(short recursive)
{
	resetSongs(recursive, 1);
}

void
mp3Win::resize(int width, int height, int recursive)
{
	scrollWin::resize(width, height);

	if (!recursive)
	{
		return;
	}

	//call resize in all subgroups as well
	mp3Item *tmp = (mp3Item*)getWinItem(0);

	while (tmp)
	{
		if (tmp->getType() == SUBWIN)
		{
			mp3Win *win = (mp3Win*)(tmp->getObject());
			if (strcmp(tmp->getName(), "[..]"))
			{
				win->resize(width, height, 1);
			}
		}
		tmp = (mp3Item*)tmp->next;
	}

}
