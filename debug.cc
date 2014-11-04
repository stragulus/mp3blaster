#include <stdio.h>

void 
debug(const char *txt)
{
	fprintf(stderr, "%s", txt);
}
