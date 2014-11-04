#ifndef _MP3STACK_CLASS_
#define _MP3STACK_CLASS_

#include "mp3blaster.h"
#include "gstack.h"

class mp3Stack : public gStack
{
public:
	char** getShuffledList(playmode pm, unsigned int *mp3list);

private:
	void compose_group(int, int*, int);
};

#endif
