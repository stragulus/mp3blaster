/* Sound Player Version

   Copyright (C) 1997 by Jung woo-jae */

// Header files

#include <config.h>

#define MAXLISTSIZE        1000     // Too many?
#define MAXFILENAMELENGTH   300     // Too long?

#undef PACKAGE
//splay is not maintained anymore, I'll rename the packagename to make clear 
//this is not the stand-alone (old) splay
#define PACKAGE "splay-mp3blaster"
extern int  splay_verbose;
extern char *splay_progname;
extern const char *splay_devicename;

extern char *splay_list[MAXLISTSIZE];
extern int  splay_listsize;
extern int  splay_downfrequency;
extern bool splay_shuffleflag,
            splay_repeatflag,
            splay_forcetomonoflag;

extern const char *splay_Sounderrors[];

#ifdef PTHREADEDMPEG
extern int  splay_threadnum;
#endif

/***************/
/* Manage list */
/***************/
void arglist(int argc,char *argv[],int start);
void killlist(void);
void addlist(const char *path,const char *filename);
void readlist(char *filename);
void shufflelist(void);
