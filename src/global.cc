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
 * 
 * This file contains global functions that are globally useful for the
 * mp3blaster binary
 */
/* Change history
 * 29 oct 2000: Added is_wav()
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/types.h>
#include "mp3blaster.h"
#include "id3parse.h"
#include <stdarg.h>

#include NCURSES_HEADER
extern _globalopts globalopts;
void debug(const char *fmt, ...);

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
	if (!strlen(org_path))
			return strdup(org_path);

	if (!strncmp(org_path, "~/", 2) || (!strcmp(org_path, "~")) )
		mode = 1;
	else if (org_path[0] == '~')
		mode = 2;
	else /* no ~ to expand */
		return strdup(org_path);

	if (mode == 1)
	{
		char *hd = get_homedir(NULL);	
		if (!hd)
				hd = strdup("");

		new_path = (char*)malloc((strlen(hd) + strlen(org_path) + 2) *
			sizeof(char));
		strcpy(new_path, hd);
		// org_path has at least chars (since it must have a string length) */
		if (org_path[1] != '/')
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
			return strdup(org_path);
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
debug(const char *fmt, ... ) {
	va_list ap;
	char buf[1024];
	static int debug_init = 1;
	static FILE *debug_info = NULL;

	if (!globalopts.debug)
		return;

	if (debug_init) {
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
	va_start(ap, fmt);
	vsnprintf(buf,1024,fmt,ap);
	va_end(ap);
	if (debug_info) {
		(void)fwrite(buf, sizeof(char), strlen(buf), debug_info);
		fflush(debug_info);
	}
}

/* PRE: f is open 
 * Function returns NULL when EOF(f) else the next line read from f, terminated
 * by \n. The line is malloc()'d so don't forget to free it ..
 */
char *
readline(FILE *f) {
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
crop_whitespace(const char *string) {
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
is_dir(const char *path) {
	DIR 
		*dir2 = opendir(path);
	
	if (dir2) {
		closedir(dir2);
		return 1;
	}

	return 0;
}

const char *
chop_path(const char *a) {
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

/* delete[] string after use */
char *
chop_file(const char *a)
{
	const char
		*last_slash = strrchr(a, '/');
	char 
		*new_path = NULL;

	if (!last_slash) /* no '/' in this->items[item_index] */
	{
		new_path = new char[1];
		new_path[0] = '\0';
		return new_path;
	}

	new_path = new char[strlen(a) + 1];
	strcpy(new_path, a);
	new_path[(strlen(new_path) - strlen(last_slash)) + 1] = '\0';
	return new_path;
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
is_wav(const char *filename)
{
	int len;
	
	if (!filename || (len = strlen(filename)) < 5)
		return 0;

	if (fnmatch(".[wW][aA][vV]", (filename + (len - 4)), 0))
		return 0;
	//if (strcasecmp(filename + (len - 4), ".mp3"))
	//	return 0;
	
	return 1;
}

int
is_ogg(const char *filename)
{
	int len;
	
	if (!filename || (len = strlen(filename)) < 5)
		return 0;

	if (fnmatch(".[oO][gG][gG]", (filename + (len - 4)), 0))
		return 0;
	
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
		int regflags = REG_EXTENDED | REG_ICASE;
		while (globalopts.extensions[i] != NULL)
		{
			int doesmatch = 0;
			regex_t dum;
			regcomp(&dum, globalopts.extensions[i], regflags);
			doesmatch = regexec(&dum, filename, 0, 0, 0);
			regfree(&dum);
			if (!doesmatch) //regexec returns 0 for a match.
				return 1;
			i++;
		}
		return 0;
	}
	return (is_mp3(filename) || is_sid(filename) || is_httpstream(filename) ||
		is_ogg(filename) || is_wav(filename));
}

int
is_playlist(const char *filename)
{
	int i = 0, loop = 1;

	while (loop)
	{
		const char *ext;
		int doesmatch;

		if (!globalopts.plist_exts || (!i && !globalopts.plist_exts[i]))
		{
			if (!i)
			{
				ext = "\\.lst$";
				i++;
			}
			else
			{
				ext = "\\.m3u$";
				loop = 0;
			}
		}
		else
		{
			ext = globalopts.plist_exts[i];
			loop = (globalopts.plist_exts[++i] != NULL);
		}

		regex_t dum;
		regcomp(&dum, ext, REG_EXTENDED | REG_ICASE);
		doesmatch = regexec(&dum, filename, 0, 0, 0);
		regfree(&dum);
		if (!doesmatch) //regexec returns 0 for a match.
			return 1;
	}
	return 0;
}

/* crunch_string takes a string 'flname' and shortens it (if necessary) to
 * length 'length', by chopping of the begin of the string, and replacing
 * the first 3 characters of the new strings with dots.
 * E.g. crunch_string("foobar", 5); will return "...ar".
 *
 * If the input string is shorter than 4 characters, a copy of the (unmodified)
 * string will be returned.
 */
char *
crunch_string(const char *flname, unsigned int length)
{
	char
		*filename = NULL;

	if (!flname) 
		return NULL;

	if (!strlen(flname) || length < 4)
	{
		//too short, return the thing itself.
		filename = new char[strlen(flname) + 1];
		strcpy(filename, flname);
		return filename;
	}

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

short
read_file(const char *filename, char ***lines, int *linecount)
{
	char *line = NULL;
	FILE *f = fopen(filename, "r");

	if (!f)
		return 0;

	*lines = NULL;
	*linecount = 0;

	while ( (line = readline(f)) )
	{
		*lines = (char**)realloc(*lines, (*linecount + 1) * sizeof(char**));
		if (!lines)
			return 0;

		(*lines)[(*linecount)++] = line;
	}

	return 1;
}

//charset stuff
void
recode_string(char *string)
{
	int c;

	if (string != NULL)
	{
		for( ; *string != '\000'; string++ )
		{
			c=((int)*string)&0xFF;
			*string = globalopts.recode_table[c];
		}
	}
}

//returns a filename built from id3tag info. Returned string is malloc()'d!
char *
id3_filename(const char *filename)
{
	if (!filename)
		return NULL;

	char *id3name = NULL;
	id3Parse *id3 = new id3Parse(filename);
	id3header *hdr;

	if (!id3)
		return NULL;
	if (!(hdr = id3->parseID3()))
	{
		delete id3;
		return NULL;
	}

	
	if (!strlen(hdr->artist) && !strlen(hdr->songname)) //not enough info
	{
		delete id3;
		return NULL;
	}

	char *artist = NULL, *song = NULL;

	if (!strlen(hdr->artist))
		artist = strdup("Unknown Artist");
	else
		artist = strdup(hdr->artist);

	if (!strlen(hdr->songname))
		song = strdup("Unknown Title");
	else
		song = strdup(hdr->songname);

	artist = crop_whitespace(artist);
	song = crop_whitespace(song);

	if (globalopts.recode_table)
	{
		recode_string(artist);
		recode_string(song);
	}

	id3name = (char*)malloc( (strlen(artist) + strlen(song) + 20) *
		sizeof(char));
	
	sprintf(id3name, "[ID3] %s - %s", artist, song);

	delete id3;

	return id3name;
}

/* same as above, only this function always returns a non-NULL char* pointer,
 * which must be delete[]'d instead of free()'d. Also, the argument is
 * a void* pointer.
 * The reason for this seamingly weird behaviour is because it is fed to
 * changeItem() in the scrollWin class.
 */
char *
id3_filename(void *filename)
{
	char *result =  id3_filename((char*)filename);

	if (!result)
	{
		result = new char[strlen((char*)filename) + 1];
		strcpy(result, (char*)filename);
	}
	else
	{
		//it must be allocated using new.
		char * result2 = new char[strlen(result) + 1];
		strcpy(result2, result);
		free(result);
		result = result2;
	}
	return result;
}


/* Function   : lowercase
 * Description: Transforms uppercase characters in a string to lowercase.
 * Parameters : str   : (char*)string to be lowercased
 * Returns    : Nothing.
 * SideEffects: If str is not 0-terminated, probably many.
 */
void
lowercase(char *str)
{
	unsigned int i = 0;

	if (!str)
		return;

	while (str[i])
	{
		if (isupper(str[i]))
			str[i] = tolower(str[i]);
		i++;
	}
}

/* Function   : get_input
 * Description: Gets input from a user. Predefined input value may be given,
 *            : making it easy for the user to alter that.
 * Parameters : win   : window to get input from
 *            : defval: default input value (optional)
 *            : maxlen: Maximum length of input.
 *            : y     : initial cursor y position in window
 *            : x     :    ""     ""   x   ""     ""   ""
 * Returns    : Input given by user. Free with delete[] after use.
 * SideEffects: Sets several curses input and output options. You will want
 *            : to reset them back to the values they were before calling
 *            : this function, as the current settings cannot be read by
 *            : this function.
 */
#if 0
char *
get_input(WINDOW *win, const char*defval, int maxlen, int y, int x)
{
	char *input = new char[maxlen + 1];
	int
		curlen = 0,
		cursor_pos = 0, //offset from beginning cursor pos. Also index in input.
		i = 0, c = 0;
	short
		insert_mode = 1; //0 -> replace i/o insert

	memset(input, 0, maxlen + 1);
	if (defval)
	{
		strncpy(input, defval, maxlen);
		curlen = strlen(defval);
		mvwaddstr(win, y, x, input);
		if (strlen(defval) > (unsigned int)maxlen)
			curlen = maxlen;
	}

	noecho();
	cbreak();
	leaveok(win, FALSE);
	nodelay(win, FALSE);
	wmove(win, y,x);
	wrefresh(win);

	while ((c = getch()) != 13)
	{
		switch(c)
		{
		case KEY_LEFT:
			if (cursor_pos > 0)
			{
				cursor_pos--;
			}
			break;
		case KEY_RIGHT:
			if (cursor_pos < curlen)
				cursor_pos++;
			break;
		case KEY_BACKSPACE:
			if (cursor_pos > 0)
			{
				for (i = cursor_pos - 1; i < curlen - 1; i++)
				{
					input[i] = input[i + 1];
					mvwaddch(win, y, x + i, input[i]);
				}
				input[--curlen] = '\0';
				mvwaddch(win, y, x + curlen, ' ');
				cursor_pos--;
			}
			break;
		default:
			if (cursor_pos < maxlen && isprint(c))
			{
				mvwaddch(win, y, x + cursor_pos, c);
				input[cursor_pos] = c;
				if (cursor_pos == curlen)
					curlen++;
				if (cursor_pos < maxlen - 1)
					cursor_pos++;
			}
		}
		//debug("Cursor_pos: %d\n", cursor_pos);
		wmove(win, y, x + cursor_pos);
		wrefresh(win);
	}
	return input;
};
#endif

int
sort_files_fun(const void *a, const void *b)
{
	sortmodes_t smode = globalopts.fw_sortingmode;
	const char
		*fa = chop_path(*(char**)a), 
		*fb = chop_path(*(char**)b);

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

/* Function   : sort_files
 * Description: Sorts files. Sort order is determined by globalopts.fw_sortmode
 * Parameters : files: Array to sort. Its elements can be shuffled.
 *            :        Paths in filenames are not taken into account when
 *            :        sorting!
 *            : nrfiles: Number of files in files array.
 * Returns    : Nothing.
 * SideEffects: Mangles 'files'.
 */
void
sort_files(char **files, int nrfiles)
{
	qsort(files, nrfiles, sizeof(char*), sort_files_fun);
}


