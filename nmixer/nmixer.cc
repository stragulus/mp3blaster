#include "nmixer.h"
#include <string.h>

#define BOTH_CHANNELS 0x11
#define RIGHT_CHANNEL 0x10
#define LEFT_CHANNEL  0x01

#ifndef mvwchgat
#define mvwchgat(a, b, c, d, e, f, g)
#endif

/*
 * Parameters
 * mixwin    : window in which to draw nmixer
 * mixdev    : mixer-device to use (NULL for default MIXER_DEVICE)
 * xoffset   : horizontal offset in mixwin to start drawing (0=first column)
 * yoffset   : vertical offset in mixwin to start drawing (0=first row)
 * nrlines   : Number of rows that can be used to draw mixer in (min.=8 for
 *           : large mode, mini-mode always uses 2 lines)
 * pairs     : int-array with size 4 that contain the 4 colorpair numbers
 *           : that can be used for nmixer (NULL means colorpairs 0 thru 3
 *           : are used). For mini-mode:
 *           : pairs[0]: Colour for mixer devicename, '+' preceding percentage
 *           :           and '%' sign.
 *           : pairs[1]: Colour for [ and ].
 *           : pairs[2]: Colour for < and >
 *           : pairs[3]: Colour for percentage
 * bgcolor   : background color (COLOR_x) to use for nmixer. foreground colours
 *           : are chosen by nmixer.
 * minimode  : 0 (default) for multiple-scrollbar large look, 1 for mini-mode
 *           : which looks like (but without border, thus 11x2 chars only!):
 *           : +-----------+
 *           : |MixerDevNam|
 *           : |[<]+100%[>]|
 *           : +-----------+
 */
NMixer::NMixer(WINDOW *mixwin, const char *mixdev, int xoffset, int yoffset,
	int nrlines, const int *pairs, int bgcolor, short minimode)
{
	if (!mixdev)
	{
		this->mixdev = new char[strlen(MIXER_DEVICE)+1];
		strcpy(this->mixdev, MIXER_DEVICE);
	}
	else
	{
		this->mixdev = new char[strlen(mixdev)+1];
		strcpy(this->mixdev, mixdev);
	}
	this->mixers = NULL;
	this->mixwin = mixwin;
	this->minimode = (minimode ? 1 : 0);
	this->xoffset = xoffset;
	this->yoffset = yoffset;
	this->nrlines = nrlines;
	this->bgcolor = bgcolor;
	getmaxyx(mixwin, maxy, maxx);
	if (!(this->nrlines))
		this->nrlines = maxy - this->yoffset;
	
	cpairs = new int[4];

	/* init colors */
	for (int i = 0; i < 4; i++)
	{
		if (pairs)
			cpairs[i] = pairs[i];
		else
			cpairs[i] = i;
	}
	setDefaultCmdKeys();
}

NMixer::~NMixer()
{
	if (supported) delete[] supported;
	if (cpairs) delete[] cpairs;
	if (mixers) delete mixers;
	if (mixdev) delete[] mixdev;
}

void
NMixer::setMixerCommandKey(mixer_cmd_t cmd, int key)
{
	cmdkeys[cmd] = key;
}

void
NMixer::setDefaultCmdKeys()
{
	cmdkeys[MCMD_NEXTDEV] = KEY_DOWN;
	cmdkeys[MCMD_PREVDEV] = KEY_UP;
	cmdkeys[MCMD_VOLUP] = KEY_RIGHT;
	cmdkeys[MCMD_VOLDN] = KEY_LEFT;
}

short
NMixer::NMixerInit()
{
	nrbars = 0;
	supported = NULL;
	//sbars = NULL;

	if (!minimode)
	{
		init_pair(cpairs[0], COLOR_WHITE, bgcolor);
		init_pair(cpairs[1], COLOR_GREEN, bgcolor);
		init_pair(cpairs[2], COLOR_YELLOW, bgcolor);
		init_pair(cpairs[3], COLOR_RED, bgcolor);
	}
	else
	{
		init_pair(cpairs[0], COLOR_WHITE, bgcolor); // MixDevName, '+', '%'
		init_pair(cpairs[1], COLOR_WHITE, bgcolor); // [ and ]
		init_pair(cpairs[2], COLOR_MAGENTA, bgcolor); // < and >
		init_pair(cpairs[3], COLOR_YELLOW, bgcolor);
		
	}

	if (!minimode && (maxx < 80 || nrlines < 8))
	{
		fprintf(stderr, "Window too small for mixer!\n");
		fflush(stderr);
		return 0;
	}
	else if (minimode && ((maxx - xoffset) < 11 || nrlines < 2))
		return 0;

	InitMixerDevice();

	DrawFixedStuff();

	/* Draw all Scrollbars */
	RedrawBars();

	/* initialiation succeeded. */
	return 1;
}

void
NMixer::InitMixerDevice() {
	if (mixers)
		delete mixers;
#ifdef HAVE_NASPLAYER
	if (mixdev && !strcmp(mixdev, "NAS")) //NAS-mixer
		mixers = new NASMixer(mixdev);
#endif

#ifdef WANT_OSS
	if (!mixers)
		mixers = new OSSMixer(mixdev);
#endif

	if (!mixers)
	{
		//fallback option if no mixers are available
		mixers = new NullMixer(mixdev);
	}

	supported = mixers->GetDevices(&nrbars);

	/* maxspos == max# bars ON-SCREEN */
	maxspos = ((nrlines - 1) / 3) - 1; /* 1 nonbar-line[s], 3 lines/bar */
	if ( (maxspos + 1) > nrbars)
		maxspos = nrbars - 1;
	currentspos = 0; /* current ON-SCREEN bar */
	minbar = 0;
	currentbar = 0; /* current bar (index) */
}

// Pre: 0 <= mixNr < 10
void
NMixer::SwitchMixerDevice(int mixNr)
{
	if (mixdev != NULL)
		delete[] mixdev;
	if (mixNr == 0)
	{
		mixdev = new char[strlen(MIXER_DEVICE) + 1];
		strcpy(mixdev, MIXER_DEVICE);
	}
	else
	{
		mixdev = new char[strlen(MIXER_DEVICE) + 2];
		sprintf(mixdev, "%s%d", MIXER_DEVICE, mixNr);
	}
	InitMixerDevice();

	for (int i = yoffset; i < yoffset + nrlines; i++)
		mvwprintw(mixwin, i, 2, "%64s", "");
	redraw();
}

/* TODO: Don't redraw label every time */
void
NMixer::DrawScrollbar(short i, int spos)
{
	int j;
	int my_x, my_y;
	struct volume vol;
	const char *source = mixers->GetMixerLabel(supported[i]);
	const char empty_scrollbar[] = \
		"                                                          ";

	if (minimode && i != currentbar) //minimode only draws 1 `bar'
		return;

	if (!minimode)
	{
		my_x = 2;
		my_y = yoffset + 2 + (3 * spos);
		
		//clear 2x58 positions on window
		mvwprintw(mixwin, my_y - 1, my_x, (char*)empty_scrollbar);
		mvwprintw(mixwin, my_y, my_x, (char*)empty_scrollbar);
		mvwprintw(mixwin, my_y + 1, my_x, (char*)empty_scrollbar);
	
		//draw new bar
		mvwprintw(mixwin, my_y - 1, my_x, (char*)source);
		if (i == currentbar)
		{
			mvwchgat(mixwin, my_y - 1, my_x, strlen(source), A_REVERSE, 
				cpairs[0], NULL);
		}

		//draw recording checkbox
		if (mixers->CanRecord(supported[i]))
		{
			if (mixers->GetRecord(supported[i]))
				mvwprintw(mixwin, my_y, my_x, "[X]");
			else
				mvwprintw(mixwin, my_y, my_x, "[ ]");
			mvwchgat(mixwin, my_y, my_x, 3, A_BOLD, cpairs[3], NULL);
		}
	}
	else //minimode
	{
		//draw MixDevNam
		wmove(mixwin, yoffset, xoffset);
		waddstr(mixwin, "           ");
		wmove(mixwin, yoffset, xoffset);
		waddnstr(mixwin, source, 11);
		mvwchgat(mixwin, yoffset, xoffset, strlen(source), A_NORMAL,
			cpairs[0], NULL);
	}

	/* get sound-settings */
	if (!(mixers->GetMixer(supported[i], &vol)))
		return;

	if (!minimode)
	{
		/* setting = setting / 2; */
		vol.left  /= 2;
		vol.right /= 2;
		if (vol.left  > 50) vol.left  = 50;
		if (vol.right > 50) vol.right = 50;
	
		mvwaddch(mixwin, my_y, my_x + 7, '[');
		mvwaddch(mixwin, my_y + 1, my_x + 7, '[');
		mvwaddch(mixwin, my_y, my_x + 58, ']');
		mvwaddch(mixwin, my_y + 1, my_x + 58, ']');
		wmove(mixwin, my_y, my_x + 8);
		for (j = 0; j < vol.left; j++)
			waddch(mixwin, '#');
		for (j = vol.left; j < 50; j++)
			waddch(mixwin, ' ');
	
		wmove(mixwin, my_y + 1, my_x + 8);
		for (j = 0; j < vol.right; j++)
			waddch(mixwin, '#');
		for (j = vol.right; j < 50; j++)
			waddch(mixwin, ' ');
	
		/* fix up some nice colors */
		mvwchgat(mixwin, my_y + 0, my_x + 8,  17, A_BOLD, cpairs[1], NULL);
		mvwchgat(mixwin, my_y + 0, my_x + 25, 16, A_BOLD, cpairs[2], NULL);
		mvwchgat(mixwin, my_y + 0, my_x + 41, 17, A_BOLD, cpairs[3], NULL);
		mvwchgat(mixwin, my_y + 1, my_x + 8,  17, A_BOLD, cpairs[1], NULL);
		mvwchgat(mixwin, my_y + 1, my_x + 25, 16, A_BOLD, cpairs[2], NULL);
		mvwchgat(mixwin, my_y + 1, my_x + 41, 17, A_BOLD, cpairs[3], NULL);
	}
	else //minimode
	{
		int my_vol = (vol.left + vol.right) / 2;
		mvwprintw(mixwin, yoffset + 1, xoffset + 4, "%03d", my_vol);
		mvwchgat(mixwin, yoffset + 1, xoffset + 4, 3, A_BOLD, cpairs[3],
			NULL);
	}

	wrefresh(mixwin);
	move(0, 0);
}

/* bar     : Which bar to modify (prolly currentsbar)
 * amount  : Amount by which to change OR (if absolute != 0) the absolute
 *         : value (0<=value<=100) to change to.
 * absolute: !0 if amount is an absolute value (and not relative to current
 *         : setting)
 * update  : Wether to update the screen or not..
 * channels: One of LEFT_CHANNEL/RIGHT_CHANNEL/BOTH_CHANNELS
 */
void
NMixer::ChangeBar(short bar, short amount, short absolute, short channels,
	short update)
{
	struct volume vol;

	if (!(mixers->GetMixer(supported[bar], &vol)))
		return;

	if (channels == BOTH_CHANNELS || channels == LEFT_CHANNEL)
	{
		if (absolute) vol.left = 0;
		vol.left += amount;

		if (vol.left < 0) vol.left = 0;
		if (vol.left > 100) vol.left = 100;
	}

	if (channels == BOTH_CHANNELS || channels == RIGHT_CHANNEL)
	{	
		if (absolute) vol.right = 0;
		vol.right += amount;

		if (vol.right < 0) vol.right = 0;
		if (vol.right > 100) vol.right = 100;
	}
	if (mixers->SetMixer(supported[bar], &vol) && update)
		DrawScrollbar(bar, currentspos);
}

void
NMixer::RedrawBars()
{
	int i;

	if (!minimode)
		for (i = 0; i < (maxspos + 1); i++)
			DrawScrollbar(minbar + i, i);
	else
		DrawScrollbar(currentbar, 0);
}

short
NMixer::ProcessKey(int key)
{
	short oldbar = currentbar;
	const int cmds[] = { cmdkeys[MCMD_NEXTDEV], cmdkeys[MCMD_PREVDEV],
		cmdkeys[MCMD_VOLUP], cmdkeys[MCMD_VOLDN] };

	if (key == KEY_F(12) || key == 'q')
		return 0;


	/* very dirty and quick hack to get things working.. TODO: Neatify */
	if (key == cmdkeys[MCMD_NEXTDEV])
		key = 0;
	else if (key == cmdkeys[MCMD_PREVDEV])
		key = 1;
	else if (key == cmdkeys[MCMD_VOLUP])
		key = 2;
	else if (key == cmdkeys[MCMD_VOLDN])
		key = 3;

	switch(key)
	{
		case 0:
		case 'j':
			if (++currentbar >= nrbars) /* flip da dip, B! */
			{ 
				currentbar = 0;
				minbar = 0;
				currentspos = 0;
				RedrawBars();
			}
			else
			{
				if (currentspos == maxspos) /* scroll bars up by one */
				{
					minbar++;
					RedrawBars();
				}
				else
				{
					currentspos++;	
					DrawScrollbar(oldbar, currentspos - 1);
					DrawScrollbar(currentbar, currentspos);
				}
			}
			break;
		case 1:
		case 'k':
			if (--currentbar < 0)
			{
				currentbar = nrbars - 1;
				currentspos = maxspos;
				minbar = nrbars - maxspos - 1;
				RedrawBars();
			}
			else
			{
				if (!currentspos)
				{
					minbar--;
					RedrawBars();
				}
				else
				{
					currentspos--;
					DrawScrollbar(oldbar, currentspos + 1);
					DrawScrollbar(currentbar, currentspos);
				}
			}
			break;
		case 2:
		case 'l':
			ChangeBar(currentbar,  2, 0, BOTH_CHANNELS);
			break;
		case 3:
		case 'h':
			ChangeBar(currentbar, -2, 0, BOTH_CHANNELS);
			break;
		case KEY_PPAGE:
			ChangeBar(currentbar, 2, 0, LEFT_CHANNEL);
			break;
		case KEY_HOME:
			ChangeBar(currentbar, -2, 0, LEFT_CHANNEL);
			break;
		case KEY_NPAGE:
			ChangeBar(currentbar, 2, 0, RIGHT_CHANNEL);
			break;
		case '0':
			ChangeBar(currentbar, 0, 1, BOTH_CHANNELS);
			break;
		case '1':
			ChangeBar(currentbar, 20, 1, BOTH_CHANNELS);
			break;
		case '2':
			ChangeBar(currentbar, 40, 1, BOTH_CHANNELS);
			break;
		case '3':
			ChangeBar(currentbar, 60, 1, BOTH_CHANNELS);
			break;
		case '4':
			ChangeBar(currentbar, 80, 1, BOTH_CHANNELS);
			break;
		case '5':
			ChangeBar(currentbar, 100, 1, BOTH_CHANNELS);
			break;
		case KEY_END:
			ChangeBar(currentbar, -2, 0, RIGHT_CHANNEL);
			break;
		case ' ':
		{
			int barID = supported[currentbar];
			if (mixers->CanRecord(barID))
			{
				if (mixers->GetRecord(barID))
					mixers->SetRecord(barID, false);
				else
					mixers->SetRecord(barID, true);
				RedrawBars();
			}
			break;
		}
		case KEY_F(1):
			SwitchMixerDevice(0);
			break;
		case KEY_F(2):
			SwitchMixerDevice(1);
			break;
		case KEY_F(3):
			SwitchMixerDevice(2);
			break;
		case KEY_F(4):
			SwitchMixerDevice(3);
			break;
		case KEY_F(5):
			SwitchMixerDevice(4);
			break;
		case KEY_F(6):
			SwitchMixerDevice(5);
			break;
		case KEY_F(7):
			SwitchMixerDevice(6);
			break;
		case KEY_F(8):
			SwitchMixerDevice(7);
			break;
		case KEY_F(9):
			SwitchMixerDevice(8);
			break;
		case KEY_F(10):
			SwitchMixerDevice(9);
			break;
	}
	return 1;
}

void
NMixer::SetMixer(int device, struct volume value, short update)
{
	for (int i = 0; i < nrbars; i++)
		if (supported[i] == device)
		{
			ChangeBar(i, value.left, 1, LEFT_CHANNEL, update);
			ChangeBar(i, value.right, 1, RIGHT_CHANNEL, update);
			break;
		}
}

bool
NMixer::GetMixer(int device, struct volume *vol)
{
	return mixers->GetMixer(device, vol);
}

/* Draws static layout elements. Does not refresh window.
 */
void
NMixer::DrawFixedStuff()
{
	if (!minimode)
	{
		for ( int i = 0; i < MYMIN(nrbars, (maxspos + 1)); i++)
		{
			mvwprintw(mixwin, yoffset + 2 + 3 * i, 65, "L");
			mvwprintw(mixwin, yoffset + 3 + 3 * i, 65, "R");
		}
		mvwprintw(mixwin, yoffset, 10,
			"0        1         2         3         4         5");
		wrefresh(mixwin);
	}
	else
	{
		mvwprintw(mixwin, yoffset + 1, xoffset, "%s", "[<]+   %[>]");
		//colour []'s
		mvwchgat(mixwin, yoffset + 1, xoffset, 1, A_NORMAL, cpairs[1], NULL);
		mvwchgat(mixwin, yoffset + 1, xoffset + 2, 1, A_NORMAL, cpairs[1], NULL);
		mvwchgat(mixwin, yoffset + 1, xoffset + 8, 1, A_NORMAL, cpairs[1], NULL);
		mvwchgat(mixwin, yoffset + 1, xoffset + 10, 1, A_NORMAL, cpairs[1], NULL);
		//colour + and %
		mvwchgat(mixwin, yoffset + 1, xoffset + 3, 1, A_BOLD, cpairs[0], NULL);
		mvwchgat(mixwin, yoffset + 1, xoffset + 7, 1, A_BOLD, cpairs[0], NULL);
		//colour < and >
		mvwchgat(mixwin, yoffset + 1, xoffset + 1, 1, A_NORMAL,
			cpairs[2], NULL);
		mvwchgat(mixwin, yoffset + 1, xoffset + 9, 1, A_NORMAL,
			cpairs[2], NULL);
	}
}

void
NMixer::redraw()
{
	DrawFixedStuff();
	RedrawBars();
}
