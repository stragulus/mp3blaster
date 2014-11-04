#ifdef NEWINTERFACE
#include "interface.h"
#include <stdio.h>
#include <string.h>
#include NCURSES

char *
Interface::getKeyLabelFromCode(int keyCode)
{
	char
		*label = new char[4];
	int
		i;
	struct keylabel_t
	{
		int key;
		char desc[4];
	};
	static keylabel_t klabels[] = 
	{
		{ KEY_C1, "kp1" },
		{ KEY_C3, "kp3" },
		{ KEY_B2, "kp5" },
		{ KEY_A1, "kp7" },
		{ KEY_A3, "kp9" },
		{ ' ', "spc" },
		{ 13, "ent" },
		{ KEY_IC, "ins" },
		{ KEY_DC, "del" },
		{ KEY_END, "end" },
		{ KEY_HOME, "hom" },
		{ KEY_PPAGE, "pup" },
		{ KEY_NPAGE, "pdn" },
		{ KEY_LEFT, "lft" },
		{ KEY_RIGHT, "rig" },
		{ KEY_UP, " up" },
		{ KEY_DOWN, "dwn" },
		{ KEY_BACKSPACE, "bsp" },
		{ 12, " ^L" },
		{ ERR, "" } //this must be the last entry!
	};
	struct keylabel_t
		kl;


	label[3] = '\0';

	if (keyCode > 32 && keyCode < 177)
	{
		sprintf(label, " %c ", keyCode);
		return;
	}
	for (i = 0; i < 64; i++)
	{
		if (keyCode == KEY_F(i))
		{
			sprintf(label, "F%2d", i);
			return;
		}
	}

	i = 0; kl = klabels[i];
	while (kl.key != ERR)
	{
		if (kl.key == keyCode)
		{
			sprintf(label, "%s", kl.desc);
			return;
		}
		kl = klabels[++i];
	}

	sprintf(label, "s%02x", (unsigned int)k);
	return;
}

const char *
Interface::getPlaymodeDescription(playmode mode)
{
	static const char *playmodes_desc[] = {
		/* PLAY_NONE */
		"",
		// PLAY_GROUP
		"Global Playback: Play current group, but not its subgroups",
		//PLAY_GROUPS
		"Global Playback: Play current group, including subgroups",
		//PLAY_GROUPS_RANDOMLY
		"Global Playback: You Should Not See This",
		//PLAY_SONGS
		"Global Playback: Shuffle all songs from all groups",
	};

	return playmodes_desc[mode];
}

const char *
Interface::getSortModeDescription(sortmodes_t mode)
{
	static const char *dsc[] = {
		"Don't sort",
		"Sort alphabetically, case-insensitive",
		"Sort alphabetically, case-sensitive",
		"Sort by modification date, newest first",
		"Sort by modification date, oldest first",
		"Sort by file size, smallest first",
		"Sort by file size, biggest first",
	};

	return dsc[mode];
}
#endif /* NEWINTERFACE */
