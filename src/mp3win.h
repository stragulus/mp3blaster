#ifndef _MP3WIN_CLASS_
#define _MP3WIN_CLASS_

#include "scrollwin.h"
#include "winitem.h"

class mp3Win : public scrollWin
{
public:
	mp3Win(int, int, int, int, char **, int, short, short);
	~mp3Win();
	//ovverides scrollWin's addItem, because mp3Win represents the internal
	//double linked list with mp3Item's i/o winItems.
	short addItem(const char*, short status=0, short colour=0, int index=-1,
	              short before=0, void *object=NULL, itemtype type=TEXT);
	short addItem(const char **, short *, short colour=0, int index=-1,
	             short before=0, void *object=NULL, itemtype type=TEXT);
	void delItem(int, int del=1);
	void setPlaymode(short pm) { playmode = pm; }
	short getPlaymode() { return playmode; }
	int writeToFile(FILE *); //c
	void addGroup(mp3Win*, const char* groupname=NULL);
	short isGroup(int);
	short isPreviousGroup(int);
	mp3Win* getGroup(int);
	void resetSongs(int recursive=1, short setplayed=0);
	void resetGroups(int recursive=1);
	short isPlayed() { return played; }
	void setNotPlayed() { played = 0; }
	void setPlayed() { played = 1; }
	void setPlaying() { playing = 1; }
	void setNotPlaying() { playing = 0; }
	short isPlaying();
	//unplayed index [0..unplayedSongs-1], set played
	const char *getUnplayedSong(int, short set_played = 1, short recursive=1); 
	mp3Win *getUnplayedGroup(int, short set_played = 1, short recursive=1);
	int getUnplayedSongs(short recursive=1); //#of unplayed songs, including those in groups.
	int getUnplayedGroups(short recursive=1);
	void setAllSongsPlayed(short recursive=0);

	void resize(int width, int height, int recursive = 1);

private:
	int getUnplayedItems(short type=0,short recursive=1);

	short playmode; //0: normal playmode, 1: shuffle playmode
	short played; //1 if this group has been playing in group-mode
	short playing; //1 if this group is currently being played in group-mode
};

#endif
