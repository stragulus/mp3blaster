#include <nmixer.h>
#include <string.h>
#define MIXER_DEVICE "/dev/mixer"

#define BOTH_CHANNELS 0x11
#define RIGHT_CHANNEL 0x10
#define LEFT_CHANNEL  0x01

#ifndef mvwchgat
#define mvwchgat(a, b, c, d, e, f, g)
#endif

NMixer::NMixer(WINDOW *mixwin, const char *mixdev, int yoffset, int nrlines,
	int *pairs, int bgcolor)
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
	this->mixwin = mixwin;
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
}

NMixer::~NMixer()
{
	if (supported) delete[] supported;
	if (cpairs) delete[] cpairs;
	if (mixers) delete mixers;
	if (mixdev) delete[] mixdev;
}

short
NMixer::NMixerInit()
{
	nrbars = 0;
	supported = NULL;
	//sbars = NULL;
	
	init_pair(cpairs[0], COLOR_WHITE, bgcolor);
	init_pair(cpairs[1], COLOR_GREEN, bgcolor);
	init_pair(cpairs[2], COLOR_YELLOW, bgcolor);
	init_pair(cpairs[3], COLOR_RED, bgcolor);

	if (maxx < 80 || nrlines < 8)
	{
		fprintf(stderr, "Window too small for mixer!\n");
		fflush(stderr);
		return 0;
	}

#ifdef HAVE_NASPLAYER
	if (mixdev && !strcmp(mixdev, "NAS")) //NAS-mixer
		mixers = new NASMixer(NULL);
	else
#endif
		mixers = new OSSMixer(NULL);
	//mixers = new OSSMixer(new NASMixer());
	supported = mixers->GetDevices(&nrbars);

	/* maxspos == max# bars ON-SCREEN */
	maxspos = ((nrlines - 1) / 3) - 1; /* 1 nonbar-line[s], 3 lines/bar */
	if ( (maxspos + 1) > nrbars)
		maxspos = nrbars - 1;
	currentspos = 0; /* current ON-SCREEN bar */
	minbar = 0;
	currentbar = 0; /* current bar (index) */

	for ( int i = 0; i < MYMIN(nrbars, (maxspos + 1)); i++)
	{
		mvwprintw(mixwin, yoffset + 2 + 3 * i, 65, "L");
		mvwprintw(mixwin, yoffset + 3 + 3 * i, 65, "R");
	}
	mvwprintw(mixwin, yoffset, 10,
		"0        1         2         3         4         5");
	wrefresh(mixwin);

	/* Draw all Scrollbars */
	RedrawBars();

	/* initialiation succeeded. */
	return 1;
}

void
NMixer::DrawScrollbar(short i, int spos)
{
	int j;
	int my_x, my_y;
	struct volume vol;
	const char *source = mixers->GetMixerLabel(supported[i]);
	const char empty_scrollbar[] = \
		"                                                          ";

	my_x = 2;
	my_y = yoffset + 2 + (3 * spos);
	
	//clear 2x58 positions on window
	mvwprintw(mixwin, my_y - 1, my_x, empty_scrollbar);
	mvwprintw(mixwin, my_y, my_x, empty_scrollbar);
	mvwprintw(mixwin, my_y + 1, my_x, empty_scrollbar);

	//draw new bar
	mvwprintw(mixwin, my_y - 1, my_x, source);
	if (i == currentbar)
	{
		mvwchgat(mixwin, my_y - 1, my_x, strlen(source), A_REVERSE, 
			cpairs[0], NULL);
	}

	/* get sound-settings */
	if (!(mixers->GetMixer(supported[i], &vol)))
		return;
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

	for (i = 0; i < (maxspos + 1); i++)
		DrawScrollbar(minbar + i, i);
}

short
NMixer::ProcessKey(int key)
{
	short oldbar = currentbar;

	if (key == KEY_F(12) || key == 'q')
		return 0;

	switch(key)
	{
		case KEY_DOWN:
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
		case KEY_UP:
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
		case KEY_RIGHT:
			ChangeBar(currentbar,  2, 0, BOTH_CHANNELS);
			break;
		case KEY_LEFT:
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
