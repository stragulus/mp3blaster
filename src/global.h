#ifndef _MP3B_GLOBAL_
#define _MP3B_GLOBAL_
#include <stdio.h>

enum cf_error { CF_NONE, TOOMANYVALS, BADVALTYPE, BADKEYWORD, BADVALUE,
				NOSUCHFILE };

char *get_homedir(const char*);
char *expand_path(const char*);
void debug(const char*);
char *readline(FILE *);
char *crop_whitespace(const char*);
short is_dir(const char*);

#endif /* _MP3B_GLOBAL_ */
