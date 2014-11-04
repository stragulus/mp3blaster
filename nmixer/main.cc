/* A soundmixer for linux with a nice text-interface for those non-X-ers.
 * (C)1999 Bram Avontuur (bram@avontuur.org)
 */
#include "nmixer.h"
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_GETOPT_H
#  include <getopt.h>
#else
#  include "getopt_local.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WANT_OSS
#include SOUNDCARD_HEADERFILE
#endif

#ifdef __cplusplus
}
#endif

/* Prototypes */
short is_mixer_device(const char *mdev);
void usage();
void parse_setting(const char *arg);

extern char **environ; //needed for getenv!

int
	*initdevs = NULL,
	*initsets = NULL,
	initnr = 0;

int
main(int argc, char *argv[])
{
	NMixer *bla;
	int
		no_interface = 0;
	char
		*sound_device = NULL;

	/* initialize curses screen management */
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);

	border(0, 0, 0, 0, 0, 0, 0, 0);
	if (COLS < 80 || LINES < 8)
	{
		mvprintw(1,1, "You'll need at least an 80x8 terminal, sorry.");
		getch();
		endwin();
		exit(1);
	}

	/* parse arguments */
	while (1)
	{
		int
			c;

		c = getopt(argc, argv, "s:d:q");
	
		if (c == EOF)
			break;

		switch(c)
		{
		case '?':
		case ':':
			usage();
			break;
		case 'd':
			if (sound_device)
				free(sound_device);
			sound_device = strdup(optarg);
			break;
		case 'q':
			no_interface = 1;
			break;
		case 's':
			parse_setting(optarg);
			break;
		default:
			usage();
			break;
		}
	}

	mvprintw(0, (COLS - strlen(MYVERSION)) / 2, MYVERSION);

	if (!sound_device)
		sound_device = strdup(MIXER_DEVICE);

	if (!strstr(argv[0], "nasmixer"))
		bla = new NMixer(stdscr, sound_device, 0, 2,  LINES - 4);
	else //use NAS-mixer!
	{
		const char *devnam = getenv("DISPLAY");
		if (!devnam)
		{
			endwin();
			fprintf(stderr, "Set DISPLAY environment variable if you want "\
				"to use the NAS mixer.\n");
			exit(1);
		}
#if defined(HAVE_NASPLAYER)
		bla = new NMixer(stdscr, "NAS", 0, 2, LINES - 4);
#else
		endwin();
		fprintf(stderr, "Nmixer was not compiled with NAS support.\n");
		exit(1);
#endif
	}

	if (!(bla->NMixerInit()))
	{
		endwin();
		fprintf(stderr, "Couldn't initialize mixer.\n");
		exit(1);
	}
	
	for (int i = 0; i < initnr; i++)
	{
		struct volume tmpvol;

		tmpvol.left = tmpvol.right = initsets[i];
		bla->SetMixer(initdevs[i], tmpvol, 0);
	}
	if (initnr > -1)
		bla->RedrawBars();

	if (!no_interface)
		/* get input */
		while (bla->ProcessKey(getch()));

	endwin();
	return 0;
}

short
is_mixer_device(const char *mdev)
{
#ifdef WANT_OSS
	const char *devlabels[] = SOUND_DEVICE_NAMES;
	short isdev = -1;

	for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
	{
		if (!strcmp(mdev, devlabels[i]))
		{
			isdev = i;
		}
	}

	return isdev;
#else
	return 0;
#endif
}

/* Function   : usage
 * Description: Prints help on command line usage and exits program.
 *            : Program must have initialized ncurses when calling this fun.
 * Parameters : None.
 * Returns    : Nothing.
 * SideEffects: Ends ncurses, and program.
 */
void
usage()
{
	endwin();
	fprintf(stderr, "Usage: nmixer [-q] [-d device] [[-s <mixerdevice=volume>] ...]\n\n" \
		"\t-d  device   Mixer hardware device (default = %s)\n"\
		"\t-q           Quit program (combine with -s)\n"\
		"\t-s  dev=vol  Set volume for mixer device\n\n"\
		"Valid mixer-devices for your system (not necessarily supported " \
		"by your sound hardware!) are:", MIXER_DEVICE);

#ifdef WANT_OSS
	const char *devs[] = SOUND_DEVICE_NAMES;
	for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
		fprintf(stderr, " %s", devs[i]);
#else
		fprintf(stderr, " none");
#endif
	fprintf(stderr, ".\n");
	exit(1);
}

void
parse_setting(const char *arg)
{
	char soundlabel[10];
	int
		soundvol,
		devindex;

	if (sscanf(arg, "%9[^=]=%d", soundlabel, &soundvol) != 2)
	{
		usage();
		return;
	}

	if ((devindex = is_mixer_device(soundlabel)) < 0)
	{
		fprintf(stderr, "%s is not a valid mixer-device.\n", soundlabel);
		usage();
		return;
	}
	if (soundvol <0 || soundvol > 100)
	{
		fprintf(stderr, "Volume for %s out of range (range is 0..100)\n",
			soundlabel);
		usage();
		return;
	}

	/* device exists, whow whow whow yippee yo yippee yay */
	if (!(initdevs = (int*)realloc(initdevs, (++initnr * sizeof(int)))) ||
		!(initsets = (int*)realloc(initsets, (initnr * sizeof(int)))))
	{
		fprintf(stderr, "Allocation error!\n");
		usage();
		exit(1);
	}
	initdevs[initnr-1] = devindex;
	initsets[initnr-1] = soundvol;
}
