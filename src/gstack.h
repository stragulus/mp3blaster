#ifndef _GSTACK_CLASS_
#define _GSTACK_CLASS_

#include "mp3blaster.h"
#include "scrollwin.h"

/* gStack is a double linked list of scrollWin's */
class gStack
{
public:
	gStack() { cstack = NULL; ssize = 0; }
	~gStack();

	void add(scrollWin *a);
	short del(unsigned int stack_index);
	short next();
	short previous();
	scrollWin* entry(unsigned int stack_index);
	scrollWin* first();
	scrollWin* current();
	scrollWin* last();
	unsigned int stackSize();
	
private:
	//short del(); no longer wanted function.

	struct stack
	{
		scrollWin *win;
		struct stack *next;
		struct stack *prev;
		unsigned int index;
	} *cstack;

	unsigned int ssize;
};

#endif /* _GSTACK_CLASS_ */
