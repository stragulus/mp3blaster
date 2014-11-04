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
 * This file contains code to read/parse mp3blaster's config file.
 * All functions must begin with cf_ for clarity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3blaster.h"
#include "global.h"
#include NCURSES_HEADER

#define WORDMATCH(x) !strcasecmp(string, (x))
#define MP3BLASTER_RCFILE "~/.mp3blasterrc"

extern short set_warn_delay(unsigned int);
extern short set_audiofile_matching(const char**, int);
extern void set_sound_device(const char*);
extern void set_mixer_device(const char*);
extern short set_skip_frames(unsigned int);
extern short set_mini_mixer(short);
extern short set_playlist_matching(const char**, int);
extern short set_playlist_dir(const char*);
extern short set_sort_mode(const char*);
extern void bindkey(command_t,int);
extern short set_charset_table(const char *);
extern short set_pan_size(int);
extern short set_audio_driver(const char *);
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

//TODO: remove color-options bartoken/progressbar
struct _confopts
{
	const char * const keyword;
	unsigned short keyword_opts;
} confopts[] = {
{ "Threads", 0 },               //0
{ "DownFrequency", 1 },
{ "SoundDevice", 15 },
{ "AudiofileMatching", 31 },
{ "WarnDelay", 0 },             
{ "SkipLength", 0 },            //5
{ "WrapAround", 1 },
{ "PlaylistMatching", 31 },
{ "Color.Default.fg", 3 },
{ "Color.Default.bg", 3 },
{ "Color.Popup.fg", 3 },        //10
{ "Color.Popup.bg", 3 },
{ "Color.PopupInput.fg", 3 },
{ "Color.PopupInput.bg", 3 },
{ "Color.Error.fg", 3 },
{ "Color.Error.bg", 3 },        //15
{ "Color.Button.fg", 3 },
{ "Color.Button.bg", 3 },
{ "Color.ProgressBar.bg", 3 },
{ "Color.BarToken.fg", 3 },
{ "Color.BarToken.bg", 3 },     //20
{ "Color.ShortCut.fg", 3 },
{ "Color.ShortCut.bg", 3 },
{ "Color.Label.fg", 3 },
{ "Color.Label.bg", 3 },
{ "Color.Number.fg", 3 },       //25
{ "Color.Number.bg", 3 },
{ "Color.FileMp3.fg", 3 },
{ "Color.FileDir.fg", 3 },
{ "Color.FileLst.fg", 3 },
{ "Color.FileWin.fg", 3 },      //30
{ "Key.SelectFiles", 2 },
{ "Key.AddGroup", 2 },
{ "Key.LoadPlaylist", 2 },
{ "Key.WritePlaylist", 2 },
{ "Key.SetGroupTitle", 2 },     //35
{ "Key.ToggleRepeat", 2 },
{ "Key.ToggleShuffle", 2 },
{ "Key.TogglePlaymode", 2 },
{ "Key.StartPlaylist", 2 },			//no longer needed!
{ "Key.ChangeThread", 2 },      //40
{ "Key.ToggleMixer", 2 },
{ "Key.MixerVolDown", 2 },
{ "Key.MixerVolUp", 2 },
{ "Key.MoveAfter", 2 },
{ "Key.MoveBefore", 2 },        //45
{ "Key.QuitProgram", 2 },
{ "Key.Help", 2 },
{ "Key.Del", 2 },
{ "Key.Select", 2 },
{ "Key.Enter", 2 },             //50
{ "Key.Refresh", 2 },
{ "Key.PrevPage", 2 },
{ "Key.NextPage", 2 },
{ "Key.Up", 2 },
{ "Key.Down", 2 },              //55
{ "Key.File.Down", 2 }, //TODO: Deprecated
{ "Key.File.Up", 2 }, //TODO: Deprecated
{ "Key.File.PrevPage", 2 }, //TODO: Deprecated
{ "Key.File.NextPage", 2 }, //TODO: Deprecated
{ "Key.File.Enter", 2 },        //60
{ "Key.File.Select", 2 },
{ "Key.File.AddFiles", 2 },
{ "Key.File.InvSelection", 2 },
{ "Key.File.RecursiveSelect", 2 },
{ "Key.File.SetPath", 2 },      //65
{ "Key.File.DirsAsGroups", 2 },
{ "Key.File.Mp3ToWav", 2 },
{ "Key.File.AddURL", 2 },
{ "Key.StartSearch", 2 },
{ "Key.File.UpDir", 2 },        //70
{ "Key.Play.Previous", 2 },
{ "Key.Play.Play", 2 },
{ "Key.Play.Next", 2 },
{ "Key.Play.Rewind", 2 },
{ "Key.Play.Stop", 2 },         //75
{ "Key.Play.Forward", 2 },
{ "Key.HelpPrev", 2 },
{ "Key.HelpNext", 2 },
{ "Key.File.MarkBad", 2 },
{ "PlaylistDir", 15 },          //80
{ "Key.ClearPlaylist", 2 },
{ "Key.DeleteMark", 2 },
{ "HideOtherFiles", 1 },
{ "File.SortMode", 15 },
{ "File.ID3Names", 1 },         //85
{ "Key.File.Delete", 2 },
{ "Key.Play.SkipEnd", 2 },
{ "Key.Play.NextGroup", 2 },
{ "Key.Play.PrevGroup", 2 },
{ "SelectItems.UnselectFirst", 1 }, //90
{ "SelectItems.SearchRegex", 1 },
{ "SelectItems.SearchCaseInsensitive", 1 },
{ "ScanMP3", 1 },
{ "Key.File.Rename", 2 },
{ "MixerDevice", 15 },              //95
{ "CharsetTable", 15 }, 
{ "Key.ToggleSort", 2 },
{ "Key.ToggleDisplay", 2 },
{ "Key.Left", 2 },
{ "Key.Right", 2 },                 //100
{ "Key.Home", 2 },
{ "Key.End", 2 },
{ "PanSize", 0 },
{ "AudioDriver", 15 },
{ NULL, 0 }, /* last entry's keyword MUST be NULL */
};

/* This array is used to check if the value[s] for a keyword are syntactically
 * correct.
 * bit 0-3(1..15): allowed types of a value: 
 *                00=NUMBER, 01=YESNO, 10=KEY, 11=COLOUR, 1111=ANYTHING
 *                (see below)
 * bit   4(16): 0: only 1 value allowed. 1: multiple values allowed.
 */


/* Definitions for all value types:
 * YESNO: yes|no | 1|0 | true|false (case-insensitive)
 * CHAR: [\[\]A-Za-z0-9,.<>/?;:'"{}`~!@#$%^&*()-_=+\\|]
 * KEY: 'spc'|'ent'|'kp[0-9]'|'ins'|'hom'|'del'|'end'|'pup'|'pdn'|
 *        'f[1..12]|'s[0-f]{2}'|up|dwn|lft|rig|bsp|CHAR
 *      The s[0-f]{2} means scancode 0-255 (in hex!)
 * NUMBER: numerical value (int)
 * ANYTHING: any string will do. 
 */
enum _kwdtype { NUMBER, YESNO, KEY, COLOUR, ANYTHING=15};
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

short
cf_type_colour(const char *string)
{
	if (WORDMATCH("black"))
		return COLOR_BLACK;
	else if (WORDMATCH("red"))
		return COLOR_RED;
	else if (WORDMATCH("green"))
		return COLOR_GREEN;
	else if (WORDMATCH("yellow"))
		return COLOR_YELLOW;
	else if (WORDMATCH("blue"))
		return COLOR_BLUE;
	else if (WORDMATCH("magenta"))
		return COLOR_MAGENTA;
	else if (WORDMATCH("cyan"))
		return COLOR_CYAN;
	else if (WORDMATCH("white"))
		return COLOR_WHITE;
	else
		return -1;	
}

/* returns the (hopefully) correct scancode for a valid key descriptor.
 * (See 'KEY' definition in top of file for a list of valid descriptors)
 * If descriptor's invalid, ERR is returned.
 */
int
cf_type_key(const char *string)
{
	int dum;

	if ( strlen(string) == 1 && (dum=cf_type_char(string)) != ERR )
		return dum;
	//check literal matches first.
	else if (WORDMATCH("spc"))
		return (int)' ';
	else if (WORDMATCH("ent") || WORDMATCH("ret"))
		return 13;
	else if (WORDMATCH("ins"))
		return KEY_IL;
	else if (WORDMATCH("del"))
		return KEY_DC;
	else if (WORDMATCH("hom"))
		return KEY_HOME;
	else if (WORDMATCH("end"))
		return KEY_END;
	else if (WORDMATCH("pup"))
		return KEY_PPAGE;
	else if (WORDMATCH("pdn"))
		return KEY_NPAGE;
	else if (WORDMATCH("up"))
		return KEY_UP;
	else if (WORDMATCH("dwn"))
		return KEY_DOWN;
	else if (WORDMATCH("lft"))
		return KEY_LEFT;
	else if (WORDMATCH("rig"))
		return KEY_RIGHT;
	else if (WORDMATCH("bsp"))
		return KEY_BACKSPACE;

	//check for other types
	if (sscanf(string, "f%d", &dum) && dum >= 0 && dum < 64)
		return KEY_F(dum);
	else if (sscanf(string, "kp%d", &dum) && dum >=0 && dum < 10)
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
	else if (sscanf(string, "s%x", &dum) && dum >=0 && dum < 256)
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
	else if (kwdtype == COLOUR)
	{
		short clr = cf_type_colour(value);
		if (clr < 0)
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
 * TODO: instead of constant numbers, use enum to determine what value
 *       to set..
 */
short
cf_add_keyword(int keyword, const char **values, int nrvals)
{
	const char *v = values[0];

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
	case 2: /* SoundDevice */
		set_sound_device(values[0]);
		break;
	case 3: /* AudiofileMatching */
		if (!set_audiofile_matching(values, nrvals))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 4: //WarnDelay
		if (!set_warn_delay((unsigned int)cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 5: //SkipFrames
		if (!set_skip_frames((unsigned int)cf_type_number(values[0])))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 6: //WrapAround
		globalopts.wraplist = (cf_type_yesno(values[0]) != 0);
		break;
	case 7: //PlaylistMatching
		if (!set_playlist_matching(values, nrvals))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	//now the lot of colour settings.
	case 8: globalopts.colours.default_fg = cf_type_colour(values[0]); break;
	case 9: globalopts.colours.default_bg = cf_type_colour(values[0]); break;
	case 10: globalopts.colours.popup_fg = cf_type_colour(values[0]); break;
	case 11: globalopts.colours.popup_bg = cf_type_colour(values[0]); break;
	case 12: globalopts.colours.popup_input_fg = cf_type_colour(values[0]); break;
	case 13: globalopts.colours.popup_input_bg = cf_type_colour(values[0]); break;
	case 14: globalopts.colours.error_fg = cf_type_colour(values[0]); break;
	case 15: globalopts.colours.error_bg = cf_type_colour(values[0]); break;
	case 16: globalopts.colours.button_fg = cf_type_colour(values[0]); break;
	case 17: globalopts.colours.button_bg = cf_type_colour(values[0]); break;
	case 18: globalopts.colours.progbar_bg = cf_type_colour(values[0]); break;
	case 19: globalopts.colours.bartoken_fg = cf_type_colour(values[0]); break;
	case 20: globalopts.colours.bartoken_bg = cf_type_colour(values[0]); break;
	case 21: globalopts.colours.shortcut_fg = cf_type_colour(values[0]); break;
	case 22: globalopts.colours.shortcut_bg = cf_type_colour(values[0]); break;
	case 23: globalopts.colours.label_fg = cf_type_colour(values[0]); break;
	case 24: globalopts.colours.label_bg = cf_type_colour(values[0]); break;
	case 25: globalopts.colours.number_fg = cf_type_colour(values[0]); break;
	case 26: globalopts.colours.number_bg = cf_type_colour(values[0]); break;
	case 27: globalopts.colours.file_mp3_fg = cf_type_colour(values[0]); break;
	case 28: globalopts.colours.file_dir_fg = cf_type_colour(values[0]); break;
	case 29: globalopts.colours.file_lst_fg = cf_type_colour(values[0]); break;
	case 30: globalopts.colours.file_win_fg = cf_type_colour(values[0]); break;
	//keybindings
	case 31: bindkey(CMD_SELECT_FILES, cf_type_key(v)); break;
	case 32: bindkey(CMD_ADD_GROUP, cf_type_key(v)); break;
	case 33: bindkey(CMD_LOAD_PLAYLIST, cf_type_key(v)); break;
	case 34: bindkey(CMD_WRITE_PLAYLIST, cf_type_key(v)); break;
	case 35: bindkey(CMD_SET_GROUP_TITLE, cf_type_key(v)); break;
	case 36: bindkey(CMD_TOGGLE_REPEAT, cf_type_key(v)); break;
	case 37: bindkey(CMD_TOGGLE_SHUFFLE, cf_type_key(v)); break;
	case 38: bindkey(CMD_TOGGLE_PLAYMODE, cf_type_key(v)); break;
	case 39: error = BADKEYWORD; return 0;
	case 40: bindkey(CMD_CHANGE_THREAD, cf_type_key(v)); break;
	case 41: bindkey(CMD_TOGGLE_MIXER, cf_type_key(v)); break;
	case 42: bindkey(CMD_MIXER_VOL_DOWN, cf_type_key(v)); break;
	case 43: bindkey(CMD_MIXER_VOL_UP, cf_type_key(v)); break;
	case 44: bindkey(CMD_MOVE_AFTER, cf_type_key(v)); break;
	case 45: bindkey(CMD_MOVE_BEFORE, cf_type_key(v)); break;
	case 46: bindkey(CMD_QUIT_PROGRAM, cf_type_key(v)); break;
	case 47: bindkey(CMD_HELP, cf_type_key(v)); break;
	case 48: bindkey(CMD_DEL, cf_type_key(v)); break;
	case 49: bindkey(CMD_SELECT, cf_type_key(v)); break;
	case 50: bindkey(CMD_ENTER, cf_type_key(v)); break;
	case 51: bindkey(CMD_REFRESH, cf_type_key(v)); break;
	case 52: bindkey(CMD_PREV_PAGE, cf_type_key(v)); break;
	case 53: bindkey(CMD_NEXT_PAGE, cf_type_key(v)); break;
	case 54: bindkey(CMD_UP, cf_type_key(v)); break;
	case 55: bindkey(CMD_DOWN, cf_type_key(v)); break;
	//case 56: bindkey(CMD_FILE_DOWN, cf_type_key(v)); break;
	//case 57: bindkey(CMD_FILE_UP, cf_type_key(v)); break;
	//case 58: bindkey(CMD_FILE_PREV_PAGE, cf_type_key(v)); break;
	//case 59: bindkey(CMD_FILE_NEXT_PAGE, cf_type_key(v)); break;
	case 56:
	case 57:
	case 58: 
	case 59: error = BADKEYWORD; return 0;
	case 60: bindkey(CMD_FILE_ENTER, cf_type_key(v)); break;
	case 61: bindkey(CMD_FILE_SELECT, cf_type_key(v)); break;
	case 62: bindkey(CMD_FILE_ADD_FILES, cf_type_key(v)); break;
	case 63: bindkey(CMD_FILE_INV_SELECTION, cf_type_key(v)); break;
	case 64: bindkey(CMD_FILE_RECURSIVE_SELECT, cf_type_key(v)); break;
	case 65: bindkey(CMD_FILE_SET_PATH, cf_type_key(v)); break;
	case 66: bindkey(CMD_FILE_DIRS_AS_GROUPS, cf_type_key(v)); break;
	case 67: bindkey(CMD_FILE_MP3_TO_WAV, cf_type_key(v)); break;
	case 68: bindkey(CMD_FILE_ADD_URL, cf_type_key(v)); break;
	case 69: bindkey(CMD_FILE_START_SEARCH, cf_type_key(v)); break;
	case 70: bindkey(CMD_FILE_UP_DIR, cf_type_key(v)); break;
	case 71: bindkey(CMD_PLAY_PREVIOUS, cf_type_key(v)); break;
	case 72: bindkey(CMD_PLAY_PLAY, cf_type_key(v)); break;
	case 73: bindkey(CMD_PLAY_NEXT, cf_type_key(v)); break;
	case 74: bindkey(CMD_PLAY_REWIND, cf_type_key(v)); break;
	case 75: bindkey(CMD_PLAY_STOP, cf_type_key(v)); break;
	case 76: bindkey(CMD_PLAY_FORWARD, cf_type_key(v)); break;
	case 77: bindkey(CMD_HELP_PREV, cf_type_key(v)); break;
	case 78: bindkey(CMD_HELP_NEXT, cf_type_key(v)); break;
	case 79: bindkey(CMD_FILE_MARK_BAD, cf_type_key(v)); break;
	case 80: //PlaylistDir
		if (!set_playlist_dir(values[0]))
		{
			error = BADVALUE;
			return 0;
		}
		break;
	case 81: bindkey(CMD_CLEAR_PLAYLIST, cf_type_key(v)); break;
	case 82: bindkey(CMD_DEL_MARK, cf_type_key(v)); break;
	case 83: globalopts.fw_hideothers = cf_type_yesno(values[0]); break;
	case 84:
		if (set_sort_mode(values[0]) < 0) { error = BADVALUE; return 0; }
		break;
	case 85: globalopts.want_id3names = cf_type_yesno(values[0]); break;
	case 86: bindkey(CMD_FILE_DELETE, cf_type_key(v)); break;
	case 87: bindkey(CMD_PLAY_SKIPEND, cf_type_key(v)); break;
	case 88: bindkey(CMD_PLAY_NEXTGROUP, cf_type_key(v)); break;
	case 89: bindkey(CMD_PLAY_PREVGROUP, cf_type_key(v)); break;
	case 90: globalopts.selectitems_unselectfirst = cf_type_yesno(values[0]);
		break;
	case 91: globalopts.selectitems_searchusingregexp = cf_type_yesno(values[0]);
		break;
	case 92: globalopts.selectitems_caseinsensitive = cf_type_yesno(values[0]);
		break;
	case 93: globalopts.scan_mp3s = cf_type_yesno(values[0]);
		break;
	case 94: bindkey(CMD_FILE_RENAME, cf_type_key(v)); break;
	case 95: set_mixer_device(values[0]); break;
	case 96: //CharsetTable
		if (!set_charset_table(values[0])) { error = BADVALUE; return 0; }
		break;
	case 97: bindkey(CMD_FILE_TOGGLE_SORT, cf_type_key(v)); break;
	case 98: bindkey(CMD_TOGGLE_DISPLAY, cf_type_key(v)); break;
	case 99: bindkey(CMD_LEFT, cf_type_key(v)); break;
	case 100: bindkey(CMD_RIGHT, cf_type_key(v)); break;
	case 101: bindkey(CMD_JUMP_TOP, cf_type_key(v)); break;
	case 102: bindkey(CMD_JUMP_BOT, cf_type_key(v)); break;
	case 103:
		if (!set_pan_size(cf_type_number(v)))
		{ 
			error = BADVALUE;
			return 0;
		}
		break;
	case 104: //audio driver
		if (!set_audio_driver(values[0])) { error = BADVALUE; return 0; }
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
		break;
	case BADVALUE:
		strcpy(dummy, "Bad Value");
		break;
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
