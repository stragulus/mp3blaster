#define PTHREADEDMPEG
#define HAVE_PTHREAD_H
#include "mp3player.h"

int
main()
{
	mp3Player
		*bla = new mp3Player();
		
	if (!bla->openfile("/tmp/track05.mp3", "/dev/dsp"))
	{
		fprintf(stderr, "Can't open file or audio-device.\n");
		exit(1);
	}
	printf("Hier ging het nog goed.\n"); fflush(stdout);
	bla->playingwiththread(1, 100);

	return 0;
}
