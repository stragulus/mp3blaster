#ifndef _MP3BLASTER_HISTORY_H
#define _MP3BLASTER_HISTORY_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>

class History
{
public:
	History();
	~History();

	short add(const char*);
	short atEnd();
	short atStart();
	void previous();
	void next();
	const char *element();

private:
	std::vector<char *> v;
	unsigned int curpos;
	std::vector<char *>::iterator it;
};

#endif /* _MP3BLASTER_HISTORY_H */
