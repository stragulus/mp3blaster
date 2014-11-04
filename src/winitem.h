/* 13 Feb 2000: Initial creation
 */
#ifndef _WINITEM_CLASS_
#define _WINITEM_CLASS_

#include "mp3blaster.h"
#include NCURSES_HEADER

#define NAMEDIM 9

enum itemtype { TEXT, SUBWIN };

typedef struct { char *value;
		 char *(*fetch)(void *);
		 void *arg;
	       } item_t;
class winItem
{
public:
	winItem();
	//Make sure the subclass destructor gets called too if it exists.
	virtual ~winItem();

	void setName(const char *, short index=0);
	void setNameFun(char *(*func)(void *), void *arg, short index=0);
	const char *getName(short index=0);
	void setColour(short);
	short getColour() { return colour; }
	void *getObject() { return object; }
	void setObject(void *, itemtype type=SUBWIN);
	void setType(itemtype);
	void select(int selID) { selectID = selID; }
	void deselect() { selectID = 0; }
	int selected() { return selectID; }
	itemtype getType() { return type; }

	winItem *prev, *next; //this should be protected and befriended to scrollWin

private:
	//name to be displayed in window, indexed on status (f.e. full path,
	//pathless, id3tag-name,...)
	item_t names[NAMEDIM];
	void *object; /* a window inside this one. */
	itemtype type; 
	short colour; //which colourpair is used for displaying. (-1 for global)
	int selectID;
};
#endif /* _WINITEM_CLASS_ */
