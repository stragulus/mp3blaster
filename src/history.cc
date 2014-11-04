#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "history.h"

History::History()
{
	curpos = 0;
	it = v.end();
}

History::~History()
{
	std::vector<char *>::iterator i;

	for (i = v.begin(); i < v.end(); i++)
	{
		if (*i)
			free((char *)(*i));
	}
}

/* side effect: current position is set to end of list */
short
History::add(const char *m)
{
	v.insert(v.end(), strdup(m));
	it = v.end();
	return 1;
}

short
History::atStart()
{
	if (it == v.begin())
		return 1;
	return 0;
}

short
History::atEnd()
{
	if (it == v.end())
		return 1;
	return 0;
}

void
History::previous()
{
	if (it > v.begin())
		it--;
}

void
History::next()
{
	if (it < v.end())
		it++;
}

const char *
History::element()
{
	return (const char *)(*it);
}
