#include "mp3blaster.h"
#include NCURSES
#include <stdlib.h>
#include "scrollwin.h"
#include "gstack.h"
#include "mp3stack.h"
#include "mp3play.h"
#include "mp3player.h"

scrollWin *w;
mp3Stack *g;

int current_group;

int
main()
{
	current_group = 0;

	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	char **tstarr;
	tstarr = (char**)malloc(5 * sizeof(char*));
	tstarr[0] = (char*)malloc(100 * sizeof(char));
	tstarr[1] = (char*)malloc(100 * sizeof(char));
	tstarr[2] = (char*)malloc(100 * sizeof(char));
	tstarr[3] = (char*)malloc(100 * sizeof(char));
	tstarr[4] = (char*)malloc(100 * sizeof(char));
	strcpy(tstarr[0], "/tmp/ome.henk.mp3");
	strcpy(tstarr[1], "/tmp/track05.mp3");
	strcpy(tstarr[2], "/tmp/track05.mp3");
	strcpy(tstarr[3], "/tmp/track05.mp3");
	strcpy(tstarr[4], "/tmp/track05.mp3");

	unsigned int nmp3s;
	char **mp3list = NULL;

	w = new scrollWin(LINES, COLS, 0, 0, (char **)tstarr, 5, 1, 0);
	//w->swRefresh(1);

	g = new mp3Stack();
	g->add(w);

	mp3list = g->getShuffledList(PLAY_SONGS, &nmp3s);
	delete g;

	/* setup colours */
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_WHITE, COLOR_RED);
	init_pair(4, COLOR_WHITE, COLOR_BLUE);
	init_pair(5, COLOR_WHITE, COLOR_MAGENTA);

	mp3Play *player = new mp3Play((const char**)mp3list, nmp3s);
	player->setThreads(200);
	player->playMp3List();
	wgetch(stdscr);
	endwin();

#if 0
	mp3Player *bla = new mp3Player();

	printf("Starting MP3-play...\n"); fflush(stdout);

	if ( !(bla->openfile("/tmp/track05.mp3", "/dev/dsp")))
	{
		fprintf(stderr, "Can't open file or audio-device!\n");
		exit(1);
	}

	bla->playingwiththread(0, 100);
#endif
	return 0;
}
