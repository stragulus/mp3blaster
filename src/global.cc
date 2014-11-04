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
 * This file contains global functions that are globally useful for the
 * mp3blaster binary
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/types.h>
#include "mp3blaster.h"

extern _globalopts globalopts;
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
	static int debug_init = 1;
	static FILE *debug_info = NULL;

	if (!globalopts.debug)
		return;

	if (debug_init)
	{
		debug_init = 0;
		char *homedir = get_homedir(NULL);
		if (!homedir) return;
		const char flnam[] = ".mp3blaster";
		char to_open[strlen(homedir) + strlen(flnam) + 2];
		sprintf(to_open, "%s/%s", homedir, flnam);
		if (!(debug_info = fopen(to_open, "a")))
			return;
		//debug("Debugging messages enabled. Hang on to yer helmet!\n");
	}
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

const char *
chop_path(const char *a)
{
	const char
		*last_slash = strrchr(a, '/');
	int
		begin;

	if (!last_slash) /* no '/' in this->items[item_index] */
		begin = 0;
	else
		begin = strlen(a) - strlen(last_slash) + 1;

	return a + begin;
}

int
is_mp3(const char *filename)
{
	int len;
	
	if (!filename || (len = strlen(filename)) < 5)
		return 0;

	if (fnmatch(".[mM][pP][23]", (filename + (len - 4)), 0))
		return 0;
	//if (strcasecmp(filename + (len - 4), ".mp3"))
	//	return 0;
	
	return 1;
}

int
is_sid(const char *filename)
{
#ifdef HAVE_SIDPLAYER
	char *ext = strrchr(filename, '.');
	if (ext) {
		if (!strcasecmp(ext, ".psid")) return 1;
		if (!strcasecmp(ext, ".sid")) return 1;
		if (!strcasecmp(ext, ".dat")) return 1;
		if (!strcasecmp(ext, ".inf")) return 1;
		if (!strcasecmp(ext, ".info")) return 1;
	}
#else
	if(filename); //prevent warning
#endif
	return 0;
}

int
is_httpstream(const char *filename)
{
	if (!strncasecmp(filename, "http://", 7))
		return 1;
	return 0;
}

int
is_audiofile(const char *filename)
{
	if (globalopts.extensions)
	{
		int i = 0;
		while (globalopts.extensions[i] != NULL)
		{
			int doesmatch = 0;
			regex_t dum;
			regcomp(&dum, globalopts.extensions[i], 0);
			doesmatch = regexec(&dum, filename, 0, 0, 0);
			regfree(&dum);
			if (!doesmatch) //regexec returns 0 for a match.
				return 1;
			i++;
		}
		return 0;
	}
	return (is_mp3(filename) || is_sid(filename) || is_httpstream(filename));
}

int
is_playlist(const char *filename)
{
	int i = 0, loop = 1;

	while (loop)
	{
		const char *ext;
		int doesmatch;

		if (!globalopts.plist_exts || !globalopts.plist_exts[i])
		{
			ext = "\\.lst$";
			loop = 0;
		}
		else
			ext = globalopts.plist_exts[i];

		regex_t dum;
		regcomp(&dum, ext, 0);
		doesmatch = regexec(&dum, filename, 0, 0, 0);
		regfree(&dum);
		if (!doesmatch) //regexec returns 0 for a match.
			return 1;
		if (loop)
			loop = (globalopts.plist_exts[++i] != NULL);
	}
	return 0;
}

/* crunch_string takes a string 'flname' and shortens it (if necessary) to
 * length 'length', by chopping of the begin of the string, and replacing
 * the first 3 characters of the new strings with dots.
 * E.g. crunch_string("foobar", 5); will return "...ar".
 */
char *
crunch_string(const char *flname, unsigned int length)
{
	char
		*filename = NULL;

	if (!flname || !strlen(flname) || length < 4)
		return NULL;

	if (strlen(flname) > length)
	{
		filename = new char[length+1];
		strcpy(filename, flname+(strlen(flname)-length));
		filename[0] = filename[1] = filename[2] = '.';
	}
	else
	{
		filename = new char[strlen(flname)+1];
		strcpy(filename,flname);
	}
	return filename;
}
