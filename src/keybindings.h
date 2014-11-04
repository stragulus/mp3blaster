/* The order in the help window in mp3blaster is determined by the order of
 * keys in this array
 */

/* all program modes, defines instead of an enum so it can be logically OR'd */
#define PM_NORMAL 1
#define PM_FILESELECTION 2
#define PM_HELP 4
#define PM_ANY ((unsigned short)~0x0)

keybind_t keys[] = 
{
	{ KEY_F(1), CMD_SELECT_FILES, PM_NORMAL, "Select Files" },
	{ KEY_F(2), CMD_ADD_GROUP, PM_NORMAL, "Add Group" },
	{ KEY_F(5), CMD_SET_GROUP_TITLE, PM_NORMAL, "Set Group Title" },
	{ KEY_F(3), CMD_LOAD_PLAYLIST, PM_NORMAL, "Load/Add Playlist" },
	{ KEY_F(4), CMD_WRITE_PLAYLIST, PM_NORMAL, "Write Playlist" },
	{ 'C', CMD_CLEAR_PLAYLIST, PM_NORMAL, "Clear Playlist" },
	{ 'm', CMD_MOVE_AFTER, PM_NORMAL, "Move Files After" },
	{ 'M', CMD_MOVE_BEFORE, PM_NORMAL, "Move Files Before" },
	{ KEY_F(1), CMD_FILE_ADD_FILES, PM_FILESELECTION, "Add Files To List" },
	{ KEY_F(2), CMD_FILE_INV_SELECTION, PM_FILESELECTION, "Invert Selection" },
	{ KEY_F(3), CMD_FILE_RECURSIVE_SELECT, PM_FILESELECTION,
	  "Recurs. Select All" },
	{ KEY_F(4), CMD_FILE_SET_PATH, PM_FILESELECTION, "Enter New Path" },
	{ KEY_F(5), CMD_FILE_DIRS_AS_GROUPS, PM_FILESELECTION, "Add Dirs As Groups" },
	{ KEY_F(6), CMD_FILE_MP3_TO_WAV, PM_FILESELECTION, "Convert MP3 To WAV" },
	{ KEY_F(7), CMD_FILE_ADD_URL, PM_FILESELECTION, "Add URL(shoutcast)" },
	{ '/', CMD_FILE_START_SEARCH, PM_FILESELECTION | PM_NORMAL, "Start Search" },
	{ 's', CMD_FILE_TOGGLE_SORT, PM_FILESELECTION, "Toggle File Sort" },
	{ 'f', CMD_TOGGLE_DISPLAY, PM_NORMAL | PM_FILESELECTION, 
		"Toggle File Display" },
	{ KEY_BACKSPACE, CMD_FILE_UP_DIR, PM_FILESELECTION, "Go Up One Dir" },
	{ ' ', CMD_FILE_SELECT, PM_FILESELECTION, "Select File" },
	{ 'r', CMD_FILE_RENAME, PM_FILESELECTION, "Rename File" },
	{ KEY_F(6), CMD_TOGGLE_REPEAT, PM_NORMAL, "Toggle Repeat" },
	{ KEY_F(7), CMD_TOGGLE_SHUFFLE, PM_NORMAL, "Toggle GroupShuffle" },
	{ KEY_F(8), CMD_TOGGLE_PLAYMODE, PM_NORMAL, "Toggle Play Mode" },
	{ '[', CMD_PLAY_PREVGROUP, PM_ANY, "Previous Group" },
	{ ']', CMD_PLAY_NEXTGROUP, PM_ANY, "Next Group" },
	{ KEY_F(10), CMD_CHANGE_THREAD, PM_ANY, "Change #Threads" },
	{ ' ', CMD_SELECT, PM_NORMAL, "Select File" },
	{ 'S', CMD_SELECT_ITEMS, PM_ANY, "Select Some Items" },
	{ 'U', CMD_DESELECT_ITEMS, PM_ANY, "Unselect All" },
	{ 'b', CMD_FILE_MARK_BAD, PM_FILESELECTION, "Mark file(s) as bad" },
	{ 'D', CMD_FILE_DELETE, PM_FILESELECTION, "Delete file" },
	{ 't', CMD_TOGGLE_MIXER, PM_ANY, "Toggle Mixer Device" },
	{ '<', CMD_MIXER_VOL_DOWN, PM_ANY, "MixerVolume Down" },
	{ '>', CMD_MIXER_VOL_UP, PM_ANY, "MixerVolume Up" },
	{ 'q', CMD_QUIT_PROGRAM, PM_ANY, "Quit Mp3blaster" },
	{ '?', CMD_HELP, PM_ANY, "Show Help" },
	{ 'd', CMD_DEL, PM_NORMAL, "Delete" },
	{ 'D', CMD_DEL_MARK, PM_NORMAL, "Delete And Mark" },
	{ 12, CMD_REFRESH, PM_ANY, "Refresh Screen" },
	{ 13, CMD_ENTER, PM_NORMAL, "Enter" },
	{ 13, CMD_FILE_ENTER, PM_FILESELECTION, "Enter" },
	{ KEY_PPAGE, CMD_PREV_PAGE, PM_ANY, "Previous Page" },
	{ KEY_NPAGE, CMD_NEXT_PAGE, PM_ANY, "Next Page" },
	{ KEY_UP, CMD_UP, PM_ANY, "Move Scrollbar Up" },
	{ KEY_DOWN, CMD_DOWN, PM_ANY, "Move Scrolbar Down" },
	{ KEY_HOME, CMD_JUMP_TOP, PM_ANY, "Start Of List" },
	{ KEY_END, CMD_JUMP_BOT, PM_ANY, "End Of List" },
	{ KEY_LEFT, CMD_LEFT, PM_ANY, "Pan Window Left" },
	{ KEY_RIGHT, CMD_RIGHT, PM_ANY, "Pan Window Right" },
	{ 'w', CMD_TOGGLE_WRAP, PM_ANY, "Toggle List Wrap" },
	{ '4', CMD_PLAY_PREVIOUS, PM_ANY, "Previous Song" },
	{ '5', CMD_PLAY_PLAY, PM_ANY, "Play Song" },
	{ '6', CMD_PLAY_NEXT, PM_ANY, "Next Song" },
	{ '1', CMD_PLAY_REWIND, PM_ANY, "Rewind Song" },
	{ '2', CMD_PLAY_STOP, PM_ANY, "Stop Song" },
	{ '3', CMD_PLAY_FORWARD, PM_ANY, "Forward Song" },
	{ '-', CMD_HELP_PREV, PM_ANY, "Scroll Up Helptext" },
	{ '+', CMD_HELP_NEXT, PM_ANY, "Scroll Down Helptxt" },
	{ 'E', CMD_PLAY_SKIPEND, PM_ANY, "Skip to End Song" },
	{ 0, CMD_NONE, PM_ANY, "" } //this should *always* be the last struct here */
};

#ifndef NEWINTERFACE
keylabel_t klabels[] = 
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
#endif
