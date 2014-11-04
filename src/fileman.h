#ifndef _FILEMANAGER_CLASS_
#define _FILEMANAGER_CLASS_

#include "mp3blaster.h"
#include "scrollwin.h"

class fileManager : public scrollWin
{
public:
	fileManager(const char *dirname, int, int, int, int, short, short);
	~fileManager() { if (path) delete[] path; }

	int changeDir(const char *newpath);
	char *getPath();
	short isDir(int index);

private:
	void getCurrentWorkingPath();	
	void readDir();
	char *path;
};

#endif /* _FILEMANAGER_CLASS_ */
