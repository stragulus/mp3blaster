/* A soundmixer for linux with a nice text-interface for those non-X-ers.
 * (C)1999 Bram Avontuur (brama@stack.nl)
 */
#include "nmixer.h"
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif

#ifdef __cplusplus
}
#endif

/* Prototypes */
short is_mixer_device(const char *mdev);
void usage(const char *);

extern char **environ; //needed for getenv!

int
main(int argc, char *argv[])
{
	NMixer *bla;
	int
		*initdevs = NULL,
		*initsets = NULL,
		initnr = 0,
		no_interface = 0;

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
	for (int i = 1; i < (argc); i++)
	{
		char soundlabel[10];
		int soundvol, devindex;

		if (sscanf(argv[i], "%9[^=]=%d", soundlabel, &soundvol) != 2)
		{
			if (!strcmp(argv[i], "-q"))
			{
				no_interface=1;
				continue;
			}
			else
			{
				endwin();
				usage(argv[0]);
				exit(1);
			}
		}
		if ((devindex = is_mixer_device(soundlabel)) < 0)
		{
			endwin();
			fprintf(stderr, "%s is not a valid mixer-device.\n", soundlabel);
			usage(argv[0]);
			exit(1);
		}
	
		if (soundvol <0 || soundvol > 100)
		{
			endwin();
			fprintf(stderr, "Volume for %s out of range (range is 0..100)\n",
				soundlabel);
			exit(1);
		}

		/* device exists, whow whow whow yippee yo yippee yay */
		if (!(initdevs = (int*)realloc(initdevs, (++initnr * sizeof(int)))) ||
			!(initsets = (int*)realloc(initsets, (initnr * sizeof(int)))))
		{
			endwin();
			fprintf(stderr, "Allocation error!\n");
			exit(1);
		}
		initdevs[initnr-1] = devindex;
		initsets[initnr-1] = soundvol;
	}

	mvprintw(0, (COLS - strlen(MYVERSION)) / 2, MYVERSION);

	if (!strstr(argv[0], "nasmixer"))
		bla = new NMixer(stdscr, (const char*)NULL, 2, LINES - 4);
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
		bla = new NMixer(stdscr, "NAS", 2, LINES - 4);
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
}

void
usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [<mixerdevice=volume> ...]\n\n" \
		"Valid mixer-devices for your system (not necessarily supported " \
		"by your sound hardware!) are:", progname);

	const char *devs[] = SOUND_DEVICE_NAMES;
	for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
		fprintf(stderr, " %s", devs[i]);
	fprintf(stderr, ".\n");
}
