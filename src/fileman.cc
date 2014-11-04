/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) Bram Avontuur (bram@avontuur.org)
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "fileman.h"

extern struct _globalopts globalopts;

/* initialize filemanager. If path is NULL, the current working path will be
 * used.
 */
fileManager::fileManager(const char *ourpath, int lines, int ncols, int begin_y,
                         int begin_x, short show_path, short x_offset)
{
	DIR
		*dir = NULL;
	path = NULL;

	if (!ourpath || chdir(ourpath) < 0)
		getCurrentWorkingPath();		
	
	if ( !(dir = opendir(path)) ) /* eeks. */
	{
		delete[] path;
		path = new char[2];
		strcpy(path, "/"); /* if this dir doesn't work, TOO BAD. */
	}
	else
		closedir(dir);

	/* initialize window */
	init(lines, ncols, begin_y,begin_x, show_path, x_offset);
	
	readDir();
}

/* Returns the path used by the file-manager. Don't forget to delete[] it when
 * it's no longer used.
 */
char*
fileManager::getPath()
{
	if (!path)
		return NULL;

	char *tmppwd = new char[strlen(path) + 1];
	return strcpy(tmppwd, path);
}

void
fileManager::getCurrentWorkingPath()
{
	if (path)
	{
		delete[] path;
		path = NULL;
	}

	char *tmppwd = (char *)malloc(65 * sizeof(char));
	int size = 65;
	
	while ( !(getcwd(tmppwd, size - 1)) )
	{
		size += 64;
		tmppwd = (char *)realloc(tmppwd, size * sizeof(char));
	}

	path = new char[strlen(tmppwd) + 2];	
	strcpy(path, tmppwd);
	
	if (strcmp(path, "/")) /* path != "/" */
		strcat(path, "/");

	free(tmppwd);
}

int
fileManager::changeDir(const char *newpath)
{
	if (chdir(newpath) < 0)
		return 0;

	getCurrentWorkingPath(); /* new dir is stored in variable 'path' */
	readDir();
	return 1;
}

int
sortme(const void *a, const void *b)
{
	if (!globalopts.fw_sortingmode)
		return (strcasecmp(*(char**)a, *(char**)b));
	else
		return (strcmp(*(char**)a, *(char**)b));
}

void
fileManager::readDir()
{
	struct dirent
		*entry = NULL;
	char
		**entries = NULL;
	int
		i;
	DIR
		*dir = NULL;

	delItems(); //erase contents first.

	if (!path)
		getCurrentWorkingPath();

	if ( !(dir = opendir(path)) )
	{
		Error("Error opening directory!");
		return;
	}

	int diritems = 0;

	while ( (entry = readdir(dir)) )
	{
		entries = (char **)realloc (entries, (++diritems) * sizeof(char *));
	
		entries[diritems - 1] = (char *)malloc( ((entry->d_reclen) + 1) *
			sizeof(char));
		strcpy(entries[diritems - 1], entry->d_name);
	}
	
	closedir(dir);
	
	/* sort char **entries */
	if (diritems > 1)
		qsort(entries, diritems, sizeof(char*), sortme);

	for (i = 0; i < diritems; i++)
	{
		DIR
			*dir2;

		if ( (dir2 = opendir(entries[i])) ) /* path is a dir */
		{
			closedir(dir2);
			entries[i] = (char *)realloc(entries[i], (strlen(entries[i]) + 2) *
				sizeof(char));
			strcat(entries[i], "/");
			addItem(entries[i]);
			free(entries[i]);
			entries[i] = NULL;
		}
	}

	for (i = 0; i < diritems; i++)
	{
		if (entries[i])
			addItem(entries[i]);
	}

	setTitle(path);
	swRefresh(0);

	//set_header_title(file_window->sw_title);
	//wrefresh(header_window);

	//fw_set_default_fkeys();
	for (i = 0; i < diritems; i++)
		free(entries[i]);
	if (entries)
		free(entries);
}

short
fileManager::isDir(int index)
{
	if (index < 0 || index >= getNitems())
		return 0;
	
	DIR
		*dir2;

	if ( (dir2 = opendir(getItem(index))) ) /* path is a dir */
	{
		closedir(dir2);
		return 1;
	}
	
	return 0;
}
