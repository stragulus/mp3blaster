#include <nmixer.h>
#define MIXER_DEVICE "/dev/mixer"

#define BOTH_CHANNELS 0x11
#define RIGHT_CHANNEL 0x10
#define LEFT_CHANNEL  0x01

NMixer::NMixer(WINDOW *mixwin, int yoffset, int nrlines, int *pairs,
	int bgcolor)
{
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
	if (supported)
		delete[] supported;
	if (cpairs)
		delete[] cpairs;
	if (mixer >= 0)
		close(mixer);
}

short
NMixer::NMixerInit()
{
	int
		i;
	unsigned int setting;
	unsigned int j;

	nrbars = 0;
	supported = NULL;
	//sbars = NULL;
	
	if ( (mixer = open(MIXER_DEVICE, O_RDWR)) < 0)
	{
		//perror("Can't open /dev/mixer");
		mixer = -1;
		return 0;
	}

	ioctl(mixer, MIXER_READ(SOUND_MIXER_DEVMASK), &setting);

	/* Determine supported devices */
	for (j = 0; j < SOUND_MIXER_NRDEVICES; j++)
	{
		if (setting & (1 << j)) /*Device SOUND_DEV_LABELS[j] is supported.*/
		{
			supported = (int *)realloc(supported, (++nrbars) * sizeof(int));
			supported[nrbars - 1] = j;
		}
	}

	/* p->pcm,v->vol,s->spk,b->bas,t->tre,l->line1,m->mic,c->cd,f->fm */
	//while ( (c = getopt(argc, argv, "p:v:s:b:t:l:"
	//sbars = (WINDOW**)malloc(nrbars * sizeof(WINDOW*));

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

	/* On my system, the mozart souncard I have (with OTI-601 chipset) 
	 * produces a lot of noise and fuss after initialisation. Setting 
	 * SOUND_MIXER_LINE2 to zero stops this (what actual setting does this
	 * change on the mozart card???)
	 */
#ifdef MOZART
	volume = 0;
	ioctl(mixer, MIXER_WRITE(SOUND_MIXER_LINE2), &volume);
#endif

	/* maxspos == max# bars ON-SCREEN */
	maxspos = ((nrlines - 1) / 3) - 1; /* 1 nonbar-line[s], 3 lines/bar */
	if ( (maxspos + 1) > nrbars)
		maxspos = nrbars - 1;
	currentspos = 0; /* current ON-SCREEN bar */
	minbar = 0;
	currentbar = 0; /* current bar (index) */

	for ( i = 0; i < MYMIN(nrbars, (maxspos + 1)); i++)
	{
		mvwprintw(mixwin, yoffset + 1 + 3 * i, 65, "L");
		mvwprintw(mixwin, yoffset + 2 + 3 * i, 65, "R");
	}
	mvwprintw(mixwin, yoffset, 10,
		"0        1         2         3         4         5");
	/* mvprintw(2, 10, "12345678901234567890123456789012345678901234567890");*/
	//mvwprintw(mixwin, maxy - 2, 2, "(C)1999 Bram Avontuur (brama@stack.nl)");
	wrefresh(mixwin);

	/* Draw all Scrollbars */
	RedrawBars();

	/* initialiation succeeded. */
	return 1;
}

void
NMixer::DrawScrollbar(short i, int spos)
{
	unsigned int
		j;
	int
		my_x, my_y;
	unsigned int
		setting, volumes[2];
	const char *sources[] = SOUND_DEVICE_LABELS;
	const char empty_scrollbar[] = \
		"                                                          ";

	my_x = 2;
	my_y = yoffset + 1 + (3 * spos);
	
	//clear 2x58 positions on window
	mvwprintw(mixwin, my_y, my_x, empty_scrollbar);
	mvwprintw(mixwin, my_y + 1, my_x, empty_scrollbar);

	//draw new bar
	mvwprintw(mixwin, my_y, my_x, sources[supported[i]]);
	if (i == currentbar)
	{
		mvwchgat(mixwin, my_y, my_x, strlen(sources[supported[i]]), A_REVERSE, 
			cpairs[0], NULL);
	}

	/* get sound-settings */
	ioctl(mixer, MIXER_READ(supported[i]), &setting);
	/* setting = setting / 2; */
	
	volumes[0] = setting & 0x000000FF;
	volumes[1] = (setting & 0x0000FF00)>>8;
	volumes[0] = MYMIN(100, volumes[0]);
	volumes[1] = MYMIN(100, volumes[1]);
	volumes[0] /= 2;
	volumes[1] /= 2;

	mvwaddch(mixwin, my_y, my_x + 7, '[');
	mvwaddch(mixwin, my_y + 1, my_x + 7, '[');
	mvwaddch(mixwin, my_y, my_x + 58, ']');
	mvwaddch(mixwin, my_y + 1, my_x + 58, ']');
	wmove(mixwin, my_y, my_x + 8);
	for (j = 0; j < volumes[0]; j++)
		waddch(mixwin, '#');
	for (j = volumes[0]; j < 50; j++)
		waddch(mixwin, ' ');

	wmove(mixwin, my_y + 1, my_x + 8);
	for (j = 0; j < volumes[1]; j++)
		waddch(mixwin, '#');
	for (j = volumes[1]; j < 50; j++)
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
	int
		volumes[2],
		setting;

	ioctl(mixer, MIXER_READ(supported[bar]), &setting);
	volumes[0] = setting & 0x000000FF;
	volumes[1] = (setting & 0x0000FF00)>>8;

	if (absolute)
	{
		if (amount < 0)
			amount = 0;
		else if (amount > 100)
			amount = 100;
	}

	if (channels == BOTH_CHANNELS || channels == LEFT_CHANNEL)
	{
		if (!absolute)
			volumes[0] += amount;
		else
			volumes[0] = amount;

		if (volumes[0] < 0)
			volumes[0] = 0;
		if (volumes[0] > 100)
			volumes[0] = 100;
	}

	if (channels == BOTH_CHANNELS || channels == RIGHT_CHANNEL)
	{	
		if (!absolute)
			volumes[1] += amount;
		else
			volumes[1] = amount;

		if (volumes[1] < 0)
			volumes[1] = 0;
		if (volumes[1] > 100)
			volumes[1] = 100;
	}
	setting = (volumes[1]<<8) + volumes[0];
	ioctl(mixer, MIXER_WRITE(supported[bar]), &setting);
	if (update)
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

short
NMixer::GetMixer(int device, struct volume *vol)
{
	short gotit = 0;
	struct volume tmp;

	for (int i = 0; i < nrbars; i++)
		if (supported[i] == device)
		{
			int setting;
			ioctl(mixer, MIXER_READ(supported[i]), &setting);
			tmp.left = setting & 0x000000FF;
			tmp.right = (setting & 0x0000FF00)>>8;
			gotit = 1;
			break;
		}

	if (gotit)
	{
		vol->left = tmp.left;
		vol->right = tmp.right;
		return 1;
	}
	else
		return 0;
}
