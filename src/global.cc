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
 * 
 * This file contains global functions that are globally useful.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>


extern FILE *debug_info;
void debug(const char*);

/* get homedir of user 'username' (username == NULL => user running this prog)
 * it returns a malloc'd string which you should free(), or NULL if user
 * does not exist
 */
char *
get_homedir(const char *username)
{
	uid_t userid = 0;
	int match_uid = 0;
	struct passwd *pwdentry;
	short foundluser = 0;
	char *homedir = NULL;

	if (!username) // this user's homedir
	{
		userid = getuid();
		match_uid = 1;
	}

	setpwent(); //rewind passwd file

	while (!foundluser && (pwdentry = getpwent()))
	{
		if (match_uid)
		{
			if (userid == pwdentry->pw_uid)
			{
				foundluser = 1;
				homedir = strdup(pwdentry->pw_dir);
			}
		}
		else 
		{
			if (!strcmp(username, pwdentry->pw_name))
			{
				foundluser = 1;
				homedir = strdup(pwdentry->pw_dir);
			}
		}
	}
	endpwent();

	if (!foundluser)
		return NULL;
	
	else return homedir;
}

/* Replace 'org_path' with a new path, in which ~ is expanded.
 * (if there's already a standard (ANSI) C function that does this,
 *  send me a wall so I can bang my head against it. ;)
 * the return value = NULL if ~ can't be expanded, or else a malloc()'d
 * string containing the new path (which equals org_path if it didn't contain
 * a ~ as first char)
 */
char *
expand_path(const char *org_path)
{
	char
		*new_path = NULL;
	int
		mode = 0;

	if (!org_path)
		return NULL;

	if (!strncmp(org_path, "~/", 2) || (!strcmp(org_path, "~")) )
		mode = 1;
	else if (org_path[0] == '~')
		mode = 2;
	else /* no ~ to expand */
	{
		new_path = (char*)malloc((strlen(org_path)+1) * sizeof(char));
		strcpy(new_path, org_path);
		return new_path;
	}

	if (mode == 1)
	{
		char *hd = get_homedir(NULL);	
		if (!hd)
			return NULL;

		new_path = (char*)malloc((strlen(hd) + strlen(org_path) + 1) *
			sizeof(char));
		strcpy(new_path, hd);
		strcat(new_path, "/");
		strcat(new_path, org_path+1);
		free(hd);
		return new_path;
	}
	else if (mode == 2) /* ~username/path  or ~file */
	{
		char
			user[strlen(org_path)+1],
			rest_path[strlen(org_path)+1],
			*hd = NULL;

		int scanval = sscanf(org_path, "~%[^/]/%s", user, rest_path);

		if (!scanval)
			return NULL;

		if (!(hd = get_homedir(user))) //perhaps a filename was meant..
		{
			new_path = (char*)malloc((strlen(org_path)+1)*sizeof(char));
			strcpy(new_path, org_path);
			return new_path;
		}
		if (scanval == 2)
		{
			new_path = (char*)malloc((strlen(hd)+1+strlen(rest_path)+1) *
				sizeof(char));

			strcpy(new_path, hd);
			strcat(new_path, "/");
			strcat(new_path, rest_path);
		}
		else
		{
			new_path = (char*)malloc((strlen(hd)+2) * sizeof(char));
			strcpy(new_path, hd);
			strcat(new_path, "/");
		}
		free(hd);
		return new_path;
	}
	//should never get here;
	return NULL;
}

void 
debug(const char *txt)
{
	if (debug_info)
	{
		fwrite(txt, sizeof(char), strlen(txt), debug_info);
		fflush(debug_info);
	}
}

/* PRE: f is open 
 * Function returns NULL when EOF(f) else the next line read from f, terminated
 * by \n. The line is malloc()'d so don't forget to free it ..
 */
char *
readline(FILE *f)
{
	short
		not_found_endl = 1;
	char *
		line = NULL;

	while (not_found_endl)
	{
		char tmp[256];
		char *index;
		
		if ( !(fgets(tmp, 255, f)) )
			break;

		index = (char*)rindex(tmp, '\n');
		
		if (index && strlen(index) == 1) /* terminating \n found */
			not_found_endl = 0;

		if (!line)	
		{
			line = (char*)malloc(sizeof(char));
			line[0] = '\0';
		}
			
		line = (char*)realloc(line, (strlen(line) + strlen(tmp) + 1) *
			sizeof(char));
		strcat(line, tmp);
	}

	if (line && line[strlen(line) - 1] != '\n') /* no termin. \n! */
	{
		line = (char*)realloc(line, (strlen(line) + 2) * sizeof(char));
		strcat(line, "\n");
	}

	return line;
}

/* returns a malloc()'d string */
char *
crop_whitespace(const char *string)
{
	char *orgstring, *tmpstring, *newstring;
	int index = 0;

	if (!string)
		return NULL;

	tmpstring = strdup(string);
	orgstring = tmpstring;

	while (tmpstring[0] != '\0' && isspace((int)tmpstring[0]))
		tmpstring++;

	while ( (index = strlen(tmpstring)-1) > -1 &&
		isspace((int)tmpstring[index]))
		tmpstring[index]='\0';
	
	newstring = (char*)malloc((strlen(tmpstring)+1)*sizeof(char));	
	strcpy(newstring, tmpstring);
	free(orgstring);
	return newstring;
}

/* checks if a path is a dir we can access. (hence diff. from fstat, which
 * doesn't check access)
 */
short
is_dir(const char *path)
{
	DIR 
		*dir2 = opendir(path);
	
	if (dir2)
	{
		closedir(dir2);
		return 1;
	}

	return 0;
}
