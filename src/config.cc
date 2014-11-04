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
 * This file contains code to read/parse mp3blaster's config file.
 * All functions must begin with cf_ for clarity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3blaster.h"
#include "global.h"
#include NCURSES

#define KEYMATCH(x) !strcasecmp(string, (x))
#define MP3BLASTER_RCFILE "~/.mp3blasterrc"

extern short set_warn_delay(unsigned int);
extern short set_fpl(int);
extern short set_audiofile_matching(const char**, int);
extern void set_sound_device(const char*);
extern short set_skip_frames(unsigned int);
#ifdef PTHREADEDMPEG
extern short set_threads(int);
#endif

/* If you want to add a configuration option, you have to add them to the
 * followin three arrays. For each keyword, the index in all three arrays
 * must be the same! The last element in the keywords array must be NULL.
 * In the function cf_add_keyword the keywords will be processed, so be
 * sure to add a case-statement for your new config option here. Constraints
 * on global variables (in globalopts), other then its syntactical type,
 * are put in main.cc and not here!
 */

struct _confopts
{
	char *keyword;
	unsigned short keyword_opts;
} confopts[] = {
{ "Threads", 0 },
{ "DownFrequency", 1 },
{ "FramesPerLoop", 0 },
{ "SoundDevice", 15 },
{ "AudiofileMatching", 31 },
{ "WarnDelay", 0 },
{ "SkipFrames", 0 },
{ NULL, 0 }, /* last entry's keyword MUST be NULL */
};

/* This array is used to check if the value[s] for a keyword are syntactically
 * correct.
 * bit 0-3(1..8): allowed types of a value: 
 *                00=NUMBER, 01=YESNO, 10=KEY, 1111=ANYTHING (see below)
 * bit   4(16): 0: only 1 value allowed. 1: multiple values allowed.
 */
const unsigned short keyword_opts[] = {
	0, /* threads: 1 number */
	1, /* downfrequency: 1 yesno */
	0, /* framesperloop: 1 number */
	15, /* 1 * anything */
	31, /* multiple * anything */
	0, /* warndelay: 1 number */
	};


/* Definitions for all value types:
 * YESNO: yes|no | 1|0 | true|false (case-insensitive)
 * CHAR: [\[\]A-Za-z0-9,.<>/?;:'"{}`~!@#$%^&*()-_=+\\|]
 * KEY: 'space'|'enter'|'kpd_[0-9]'|'ins'|'home'|'del'|'end'|'pgup'|'pgdn'|
 *        'f[1..12]|'scancode_[0-9]+'|CHAR
 * NUMBER: numerical value (int)
 * ANYTHING: any string will do. 
 */
enum _kwdtype { NUMBER, YESNO, KEY, ANYTHING=15};
cf_error error;

char errstring[256];

extern struct _globalopts globalopts; /* from main.cc */

/* returns 1 if string == yes|true|1 (case-insensitive)
 * returns 0 if string == no|false|0 (case-insensitive)
 * returns -1 in all other cases.
 */
short
cf_type_yesno(const char *string)
{
	if (!strcasecmp(string, "yes") || !strcasecmp(string, "true") ||
		atoi(string) == 1)
		return 1;
	if (!strcasecmp(string, "no") || !strcasecmp(string, "false") ||
		atoi(string) == 0)
		return 0;
	return -1;
}

int
cf_type_number(const char *string, int *succeed=0)
{
	int tmp;
	if ((!sscanf(string, "%d", &tmp) || tmp != atoi(string)) && succeed)
		*succeed = 0;
	else if (succeed)
		*succeed = 1;
	return tmp;	
}

/*if string consists of one char (strlen==1), return the char cast to int.
 * else, return ERR
 */
int
cf_type_char(const char *string)
{
	if (strlen(string) == 1)
		return (int)string[0];
	return ERR;
}

/* returns the (hopefully) correct scancode for a valid key descriptor.
 * (See 'KEY' definition in top of file for a list of valid descriptors)
 * If descriptor's invalid, ERR is returned.
 */
int
cf_type_key(const char *string)
{
	int dum;

	//check literal matches first.
	if (KEYMATCH("space"))
		return (int)' ';
	else if (KEYMATCH("enter") || KEYMATCH("return"))
		return KEY_ENTER;
	else if (KEYMATCH("ins"))
		return KEY_IL;
	else if (KEYMATCH("del"))
		return KEY_DC;
	else if (KEYMATCH("home"))
		return KEY_HOME;
	else if (KEYMATCH("end"))
		return KEY_END;
	else if (KEYMATCH("pgup"))
		return KEY_PPAGE;
	else if (KEYMATCH("pgdn"))
		return KEY_NPAGE;

	//check for function keys
	if (sscanf(string, "f%d", &dum) && dum >= 0 && dum < 64)
		return KEY_F(dum);
	else if (sscanf(string, "kpd_%d", &dum) && dum >=0 && dum < 10)
	{
		switch(dum)
		{
		case '1': return KEY_C1;
		case '2': return KEY_DOWN;
		case '3': return KEY_C3;
		case '4': return KEY_LEFT;
		case '5': return KEY_B2;
		case '6': return KEY_RIGHT;
		case '7': return KEY_A1;
		case '8': return KEY_UP;
		case '9': return KEY_A3;
		}
	}
	else if (sscanf(string, "scancode_%d", &dum) && dum >=0 && dum < 256)
		return dum;
	else if ( (dum=cf_type_char(string)) != ERR )
		return dum;

	//bad key descriptor.
	return ERR;
}

//returns 1 if type of value is correct, else 0
short
cf_checktype(const char *value, _kwdtype kwdtype)
{
	if (kwdtype == ANYTHING)
		return 1;
	else if (kwdtype == NUMBER)
	{
		int tmp, nr;
		nr = cf_type_number(value, &tmp);
		if (!tmp)
			return 0;
	}
	else if (kwdtype == YESNO)
	{
		short ynv = cf_type_yesno(value);
		if (ynv == -1)
			return 0;
	}
	else if (kwdtype == KEY)
	{
		int dum = cf_type_key(value);
		if (dum == ERR)
			return 0;
	}
	else //unkown type
		return 0;

	return 1;
}

/* This function handles all the possible keywords and their values. Checks
 * have already been run on the syntactical correctness of the keyword and
 * its values. For most keywords this function sets some global option for
 * mp3blaster.
 */
short
cf_add_keyword(int keyword, const char **values, int nrvals)
{
	switch(keyword)
	{
	case 0: /* threads */
#ifdef PTHREADEDMPEG
		if (!set_threads(cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
#endif
		break;
	case 1: /* DownFrequency */
		globalopts.downsample = cf_type_yesno(values[0]);
		break;
	case 2: /* FramesPerLoop */
		if (!set_fpl(cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 3: /* SoundDevice */
		set_sound_device(values[0]);
		break;
	case 4: /* AudiofileMatching */
		if (!set_audiofile_matching(values, nrvals))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 5: //WarnDelay
		if (!set_warn_delay((unsigned int)cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 6: //SkipFrames
		if (!set_skip_frames((unsigned int)cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	}
	return 1;
}

/* returns !0 if keyword was processed successfully.
 * 0 if scanning went wrong, and an error is stored.
 */
short
cf_process_keyword(const char *keyword, const char **values, int nrvals)
{
	int i = 0;
	short found = 0;

	while (!found && confopts[i].keyword != NULL)
	{
		//Check if keyword & its values are valid. If true, process it.
		if (!strcmp(keyword, confopts[i].keyword))
		{
			short
				multkwd = (confopts[i].keyword_opts>>4)&1;
			_kwdtype
				kwdtype = (_kwdtype)(confopts[i].keyword_opts&15);

			found = 1;
			//check for appropriate #values
			if (!multkwd && nrvals != 1)
			{
				error = TOOMANYVALS;
				return 0;
			}

			//check if values are valid.
			for (int j = 0; j < nrvals; j++)
			{
				if (!cf_checktype(values[j], kwdtype))
				{
					error = BADVALTYPE;
					return 0;
				}
			}
			//everything's syntactically correct. Now actually do something
			//with the keyword.
			if (!cf_add_keyword(i, values, nrvals))
				return 0;
		}
		i++;
	}
	if (!found)
	{
		error = BADKEYWORD;
		return 0;
	}
	return 1; //grin.
}

short
cf_parse_config_line(const char *line)
{
	char keyword[strlen(line)+1],
		tmpvals[strlen(line)+1],
		**values = NULL,
		*tmpval2 = NULL,
		prev = '\0';
	int
		scanval;
	unsigned int 
		val_index, i, curval_pos;
	short
		retval;

	if (!strlen(line) || line[0] == '#' || !strlen((tmpval2 =
		crop_whitespace(line))))
	{
		if (tmpval2)
			free(tmpval2);
		return 1;
	}
	if (tmpval2)
	{
		free(tmpval2);
		tmpval2 = NULL;
	}

	scanval = sscanf(line, "%[^ =\n] = %[^\n]\n", keyword, tmpvals);

	if (scanval != 2)
		return 0;

	//tmpvals = keyword[s];
	i = 0; curval_pos = 0; val_index = 0;
	values = (char**)malloc(1 * sizeof(char*));
	values[val_index] = (char*)calloc(strlen(tmpvals)+1, sizeof(char));
	while (i < strlen(tmpvals))
	{
		char curr = tmpvals[i];
		if (prev == '\\') 
		{
			//previous read char was a backslash. That implies that this 
			//character is treated a 'special' character. Currently only
			//comma and backslash itself.
			switch(curr)
			{
			case '\\':
			case ',':
				values[val_index][curval_pos++] = curr;
				break;
			}
			prev = '\0'; //prevent special char interpr. in next loop ;)
		}
		else if (curr == '\\') 
		{
			prev = '\\';
		}
		else if (curr == ',') //keyword separator
		{
			tmpval2 = crop_whitespace(values[val_index]);
			free(values[val_index]);
			values[val_index] = tmpval2;
			curval_pos = 0;
			values = (char**)realloc(values, ((++val_index)+1) * sizeof(char*));
			values[val_index] = (char*)calloc(strlen(tmpvals)+1, sizeof(char));
			prev = ',';
		}
		else //normal character
		{
			values[val_index][curval_pos++] = curr;
			prev = curr;
		}
		i++;
	}

	tmpval2 = crop_whitespace(values[val_index]);
	free(values[val_index]);
	values[val_index] = tmpval2;

	//values holds the values found for this keyword
	//val_index = (#values -1)

	retval = cf_process_keyword((const char*)keyword, (const char **)values,
		val_index + 1);

	for (i = 0; i <= val_index; i++)
		free(values[i]);
	free(values);
	
	return retval;
}

void
cf_set_error(unsigned int lineno=0)
{
	char dummy[100];
	switch(error)
	{
	case TOOMANYVALS:
		strcpy(dummy, "Too many values");
		break;
	case BADVALTYPE:
		strcpy(dummy, "Bad value[s]");
		break;
	case BADKEYWORD:
		strcpy(dummy, "Unknown keyword");
		break;
	case NOSUCHFILE:
		strcpy(dummy, "File not found");
	default:
		strcpy(dummy, "Unkown error");
	}
	if (lineno)
		sprintf(errstring, "In config file, line %04d: %s", lineno, dummy);
	else
		sprintf(errstring, "%s", dummy);
}

cf_error 
cf_get_error()
{
	return error;
}

const char*
cf_get_error_string()
{
	return (const char*)errstring;
}

short
cf_parse_config_file(const char *flnam = NULL)
{
	char
		*filename = NULL,
		*line = NULL;
	FILE
		*f = NULL;
	short
		no_error = 1;
	unsigned int
		lineno = 0;

	error = CF_NONE;

	if (!flnam)
	{
		if ( !(filename = expand_path(MP3BLASTER_RCFILE)) )
			return 0;
	}
	else
	{
		if ( !(filename = expand_path(flnam)) )
			return 0;
	}

	if (!(f = fopen(filename, "r")))
	{
		error = NOSUCHFILE;
		cf_set_error();
		return 0;
	}
	while ((line = readline(f)) && no_error)
	{
		lineno++;
		no_error = cf_parse_config_line((const char*)line);
	}
	
	if (!no_error)
		cf_set_error(lineno);

	fclose(f);
	free(filename);
	return no_error;
}


