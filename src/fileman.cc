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

#ifdef LIBPTH
#  include <pth.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include "fileman.h"
#include "global.h"

extern struct _globalopts globalopts;

sortmodes_t tmpsort;

/* initialize filemanager. If path is NULL, the current working path will be
 * used.
 */
fileManager::fileManager(const char *ourpath, int lines, int ncols, int begin_y,
                         int begin_x, short color, short x_offset)
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
	init(lines, ncols, begin_y,begin_x, color, x_offset);
	
	readDir();
}

/* Returns the path used by the file-manager.
 * TODO: return const char* undeep copy
 */
const char*
fileManager::getPath()
{
	if (!path)
		return NULL;

	return path;
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
	int invalid_path = 0;	

	while ( !(getcwd(tmppwd, size - 1)) )
	{
		if (errno != ERANGE)
		{
			invalid_path = 1;
			break;
		}
		size += 64;
		tmppwd = (char *)realloc(tmppwd, size * sizeof(char));
	}

	if (invalid_path)
	{
		/* beuh? Someone must have deleted it.. */
		if (tmppwd)
			free(tmppwd);
		tmppwd = strdup("/");
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
	int retval;

	if (chdir(newpath) < 0)
		return 0;

	getCurrentWorkingPath(); /* new dir is stored in variable 'path' */

	if (!readDir())
		retval = 0;
	else
		retval = 1;
	drawBorder(); //updates path in title
	return retval;
}

int
sortme(const void *a, const void *b)
{
	sortmodes_t smode = tmpsort;
	const char
		*fa = *(char**)a, 
		*fb = *(char**)b;

	if (smode == FW_SORT_ALPHA) //case-insenstive alphabetical sort
		return (strcasecmp(fa, fb));
	if (smode == FW_SORT_ALPHA_CASE) //case-sensitive "" ""
		return (strcmp(fa, fb));
	if (smode == FW_SORT_NONE) //don't sort
		return 0;

	struct stat sa, sb;

	if (stat(fa, &sa) < 0 || stat(fb, &sb) < 0) //can't stat one o/t files
		return 0;
		
	if (smode == FW_SORT_MODIFY_NEW || smode == FW_SORT_MODIFY_OLD)
	{
		time_t
			time_a = sa.st_mtime,
			time_b = sb.st_mtime;

		if (time_a < time_b)
			return (smode == FW_SORT_MODIFY_OLD ? -1 : 1);
		else if (time_a > time_b)
			return (smode == FW_SORT_MODIFY_OLD ? 1 : -1);
		return 0;
	}

	if (smode == FW_SORT_SIZE_SMALL || smode == FW_SORT_SIZE_BIG)
	{
		off_t
			size_a = sa.st_size,
			size_b = sb.st_size;

		if (size_a < size_b)
			return (smode == FW_SORT_SIZE_SMALL ? -1 : 1);
		else if (size_a > size_b)
			return (smode == FW_SORT_SIZE_SMALL ? 1 : -1);
		return 0;
	}

	return 0;
}

int
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
		return 0;
	}

	int diritems = 0;


	while ( (entry = readdir(dir)) )
	{
		entries = (char **)realloc (entries, (++diritems) * sizeof(char *));
	
		entries[diritems - 1] = (char *)malloc( (strlen(entry->d_name) + 1) *
			sizeof(char));
		strcpy(entries[diritems - 1], entry->d_name);
	}
	
	closedir(dir);

	/* sort char **entries */
	if (diritems > 1)
	{
		tmpsort = globalopts.fw_sortingmode;
		qsort(entries, diritems, sizeof(char*), sortme);
	}

	PTH_YIELD;

	char *dirs[diritems];
	unsigned int dircount = 0;
	for (i = 0; i < diritems; i++)
		dirs[i] = NULL;

	for (i = 0; i < diritems; i++)
	{
		DIR
			*dir2;

		PTH_YIELD;

		if ( (dir2 = opendir(entries[i])) ) /* path is a dir */
		{
			closedir(dir2);
			dirs[dircount] = (char *)malloc((strlen(entries[i]) + 2) *
				sizeof(char));
			strcpy(dirs[dircount], entries[i]);
			strcat(dirs[dircount], "/");
			free(entries[i]);
			entries[i] = NULL;
			dircount++;
		}
	}

	tmpsort = FW_SORT_ALPHA;
	qsort(dirs, dircount, sizeof(char*), sortme);

	PTH_YIELD;

	disableScreenUpdates();

	//add subdirs first.
	unsigned int j;

	for (j = 0; j < dircount; j++)
	{
		addItem(dirs[j], 0, CP_FILE_DIR);
		free(dirs[j]);
		dirs[j] = NULL;
	}


	const char
		*bla[4],
		*fsizedescs[] = { "B", "K", "M", "G" },
		*fsizedesc = NULL;
	char
		*fdesc = NULL,
		*id3name = NULL;
	short
		foo[3] = { 0, 1, 2 },
		fsize,
		indx,
		clr = 0;
	struct stat
		buffy;
	

	for (i = 0; i < diritems; i++)
	{
		PTH_YIELD;

		if (entries[i])
		{
			if (stat(entries[i], &buffy) == -1 ||
				(globalopts.fw_hideothers && !is_audiofile(entries[i]) &&
				!is_playlist(entries[i])) )
				continue;

			fsizedesc = NULL;
			fdesc = NULL;

			if (buffy.st_size < (10000)) //10000 B
			{
				fsize = buffy.st_size;
				fsizedesc = fsizedescs[0]; //bytes
			}
			else if (buffy.st_size < (10240000)) //10000 KB
			{
				fsize = buffy.st_size / 1024;
				fsizedesc = fsizedescs[1]; //kbytes
			}
			else if (buffy.st_size < (2147483647)) //2 Gb
			{
				fsize = buffy.st_size / (1024 * 1024);
				fsizedesc = fsizedescs[2]; //mbytes
			}
			else //this is past the 2Gb barrier.. YMMV.
			{
				fsize = buffy.st_size / (1024 * 1024 * 1024);
				fsizedesc = fsizedescs[3];
			}

			fdesc = new char[strlen(entries[i]) + 10];
			sprintf(fdesc, "[%4d%s] %s", fsize, fsizedesc, entries[i]);

			id3name = NULL;
			bla[0] = entries[i];
			bla[1] = fdesc;
			bla[2] = NULL;
			bla[3] = NULL;

			if (is_audiofile(entries[i]))
				clr = CP_FILE_MP3;
			else if (is_playlist(entries[i]))
				clr = CP_FILE_LST;
			else
				clr = CP_DEFAULT;
			indx=addItem(bla, foo, clr);
			if(indx!=-1)
			{
				changeItem(indx,&id3_filename,strdup(bla[0]),2);
			}
			delete[] fdesc;
			if (id3name)
				free(id3name);
		}
	}

	setTitle(path);
	enableScreenUpdates();
	resetPan();
	swRefresh(1);

	for (i = 0; i < diritems; i++)
		free(entries[i]);
	if (entries)
		free(entries);

	return 1;
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
