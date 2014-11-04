//TODO: mp3tag with NEWTHREADS is quite inefficient!
#include "mp3blaster.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <mpegsound.h>
#include <stdarg.h>
#include "id3parse.h"

extern char *genre_table[];
_globalopts globalopts;

int
	cdnr = -1,
	add2sql = 0,
	mergeID3 = 0,
	change = 0;
struct
	id3header *hdr = NULL;
char
	*fullpath = NULL,
	*cdtype = NULL,
	*dbname = NULL,
	*table = NULL,
	*user = NULL,
	*passwd = NULL,
	*albumname = NULL,
	*owncomment = NULL;

struct _filelist
{
	char *filename;
	_filelist *next;
} *filelist;

struct _mp3info
{
	int bitrate;
	int layer;
	int version;
	int protection;
	int frequency;
	char mode[20];
} mp3info;

void addfiletolist(const char*);
int parse_mp3(const char*);
void debug(const char *,...);

void
usage(const char *bla="mp3tag2")
{
	fprintf(stderr, "Usage:\t%s [-a <artist>] [-s <songname>] [-l <album>] "\
		"[-y <year>] [-e etcetera] [-g genre] [-k track] [-r] filename "\
		"[..filename]\n",
		bla);
	fprintf(stderr,
		"Flag -r merges new ID3-info with existing ID3-tag in file.\n"\
		"For a list of supported genres, use '-g list' "\
		"(the genres are compatible with WinAmp)\n" \
		"Maximum lengths:\n" \
		"\tArtist   : 30 characters\n"\
		"\tSongname : 30 characters\n"\
		"\tAlbum    : 30 characters\n"\
		"\tYear     :  4 characters\n"\
		"\tComment  : 30 characters\n"\
		"\tGenre    : 1 integer [0..254]\n"\
		"\tTrack    : 1 integer [0..255]\n\n"\
		"If you don't specify any of the id3-tag fields, the file's id3tag"\
		" will be shown only.\n"\
		"The TRACK option is only supported for mp3players that support the "\
		"ID3V1.1 standard (other will probably just ignore it). This cuts off"\
		" the length of the comment field to 28 characters if used.\n");
	exit(1);
}

void
copystring(char *a, const char *b, int n)
{
	memset(a, 0, (n + 1) * sizeof(char)); // clean string a
	strncpy(a, b, n);
}

void
clearheader(struct id3header *myhdr)
{
	memset(myhdr->songname, 0, 31 * sizeof (char));
	memset(myhdr->artist, 0, 31 * sizeof (char));
	memset(myhdr->type, 0, 31 * sizeof (char));
	memset(myhdr->year, 0, 5 * sizeof (char));
	memset(myhdr->etc, 0, 31 * sizeof (char));
	myhdr->genre = 255;
	myhdr->track = -1;
}

int
main(int argc, char *argv[])
{
	int c;

	globalopts.debug = 0;
	hdr = new id3header;
	clearheader(hdr);
	filelist = NULL;

	while ( (c =  getopt(argc, argv, "a:c:d:e:g:k:l:m:n:p:rs:t:u:y:")) != EOF)
	{
		switch(c)
		{
			case ':':
				usage(argv[0]);
				break;
			case '?':
				usage(argv[0]);
				break;
			case 'a':
				if (add2sql) usage();
				copystring(hdr->artist, optarg, 30);
				change = 1;
				break;
			case 'c':
				if (!add2sql) usage();
				cdnr = atoi(optarg);
				if (cdnr < 0)
				{
					fprintf(stderr, "Value for cdnr must be >= 0.\n");
					exit(1);
				}
				break;
			case 'd':
				if (!add2sql) usage();
				free(cdtype);
				cdtype = strdup(optarg);
				break;
			case 'e':
				if (!add2sql)
				{
					copystring(hdr->etc, optarg, 30);
					change = 1;
				}
				else
				{
					free(owncomment);
					owncomment = strdup(optarg);
				}
				break;
			case 'g': //genres
				if (add2sql) usage();
				if (!strcmp(optarg, "list"))
				{
					for (int i = 0; i < 255; i++)
						printf("%03d %s\n", i, genre_table[i]);
					exit(0);
				}
				hdr->genre = (unsigned char)atoi(optarg);
				change = 1;
				break;
			case 'k':
				if (add2sql) usage();
				hdr->track = atoi(optarg);
				if (hdr->track < 0 || hdr->track > 255)
					usage();
				change = 1;
				break;
			case 'l':
				if (!add2sql)
				{
					copystring(hdr->type, optarg, 30);
					change = 1;
				}
				else
				{
					free(albumname);
					albumname = strdup(optarg);
				}
				break;
			case 'n':
				if (!add2sql) usage();
				free(fullpath);
				fullpath = strdup(optarg);
				break;
			case 'p':
				if (!add2sql) usage();
				free(passwd);
				passwd = strdup(optarg);
				break;
			case 'r': //Read tag first.
				mergeID3 = 1;
				break;
			case 's':
				copystring(hdr->songname, optarg, 30);
				change = 1;
				break;
			case 't':
				if (!add2sql) usage();
				free(table);
				table = strdup(optarg);
				break;
			case 'u':
				if (!add2sql) usage();
				free(user);
				user = strdup(optarg);
				break;
			case 'y':
				copystring(hdr->year, optarg, 4);
				change = 1;
				break;
		}
	}

	if (optind < argc) /* rest of arguments are filename[s] */
	{
		int i;

		for (i = optind; i < argc; i++)
			addfiletolist(argv[i]);
	}
	if (!filelist)
		 usage(argv[0]);

	struct _filelist *fl = filelist;
	while (fl)
	{
		const char *fln = fl->filename;
		int retval = parse_mp3(fln);

		switch(retval)
		{
		case 0 : break;
		case -1: fprintf(stderr, "[1mError opening \"%s\".[0m\n", fln); 
			break;
		case -2: fprintf(stderr, "[1mError initializing \"%s\".[0m\n", fln);
			break;
		case -3: fprintf(stderr, "[1m%s: Failed to write ID3 tag![0m\n", fln);
			break;
		}
		fl = fl->next;
	}
	return 0;
}

void
dupheader(struct id3header *h1, struct id3header *h2)
{
	clearheader(h1);
	strcpy(h1->songname, h2->songname);
	strcpy(h1->artist, h2->artist);
	strcpy(h1->type, h2->type);
	strcpy(h1->year, h2->year);
	strcpy(h1->etc, h2->etc);
	h1->genre = h2->genre;
	h1->track = h2->track;
}

int
parse_mp3(const char *flnam)
{
	Mpegtoraw *mp3;
	Soundinputstream *loader;

	//very dirty but hey, it works.
	Soundplayer *player = Rawtofile::opendevice("/dev/null");
	struct id3header *oldtag = NULL, *header = NULL;

#if 1
	loader = new Soundinputstreamfromfile;
	if (!loader->open((char*)flnam))
	{
		delete loader;
		return -1; //error opening file
	}
	
	mp3 = new Mpegtoraw(loader,player);
	debug(flnam);
	if (!mp3->initialize((char*)flnam))
	{
		delete mp3;
		return -2; //error initializing mp3
	}

	//store all info about this mp3.
	mp3info.bitrate = mp3->getbitrate();
	mp3info.layer = mp3->getlayer();
	mp3info.version = mp3->getversion() + 1;
	mp3info.protection = (mp3->getcrccheck() ? 1 : 0);
	mp3info.frequency = mp3->getfrequency();
	memset(mp3info.mode, 0, 20 * sizeof(char));
	strncpy(mp3info.mode, mp3->getmodestring(), 19);
#endif

	oldtag = new id3header;
	clearheader(oldtag);

	id3Parse *tmp3 = new id3Parse(flnam);
	id3header *bla = tmp3->parseID3();
	if (bla)
	{	
		strncpy(oldtag->songname, bla->songname, 30);
		strncpy(oldtag->artist, bla->artist, 30);
		strncpy(oldtag->type, bla->type, 30);
		strncpy(oldtag->year, bla->year, 4);
		strncpy(oldtag->etc, bla->etc, 30);
		oldtag->genre = bla->genre;
		oldtag->track = bla->track;
	}

	delete mp3;
	delete loader;
	delete tmp3;
	bla = NULL;

	id3Parse *mp3tje = new id3Parse(flnam);
	header = new id3header;
	dupheader(header, hdr);
	if (change && mergeID3 && mp3tje->parseID3())
	{
		printf("ID3 tag in file detected. Going to merge!\n");
		fprintf(stderr, "hdr->songname=%s, header->songname=%s\n", hdr->songname,
			header->songname);fflush(stderr);
		if (!strlen(header->songname))
			copystring(header->songname, oldtag->songname, 30);
		if (!strlen(header->artist))
			copystring(header->artist, oldtag->artist, 30);
		if (!strlen(header->type))
			copystring(header->type, oldtag->type, 30);
		if (!strlen(header->year))
			copystring(header->year, oldtag->year, 4);
		if (!strlen(header->etc))
			copystring(header->etc, oldtag->etc, 30);
		if (header->genre == 255)
			header->genre = oldtag->genre;
		if (header->track == -1)
			header->track = oldtag->track;
	}

	if (change && !(mp3tje->writeID3(header)))
	{
		delete mp3tje;
		delete oldtag;
		delete header;
		return -3;
	}
	else if (change)
		printf("Wrote ID3 tag successfully!\n");
	
	if (!change) //only display current ID3 tag.
	{
		delete header;
		header = oldtag;
		oldtag = NULL;
	}

	char track[50];
	if (header->track != -1)
		sprintf(track, "[1;32mTrack[0m: %02d", header->track);
	else
		strcpy(track, "");

	char balkje[81];
	memset(balkje, 0, 81);
	memset(balkje, '-', (strlen(flnam) > 80 ? 80 : strlen(flnam)));
	printf(
		"[1;33m%s[0m\n[1;34m%s[0m\n"\
		"[1;32mArtist[0m    [1;34m:[0m %-30s\n"\
		"[1;32mSongname[0m  [1;34m:[0m %-30s\n"\
		"[1;32mAlbum[0m     [1;34m:[0m %-30s  [1;32mYear[0m: %-4s\n"\
		"[1;32mEtcetera[0m  [1;34m:[0m %-30s %s\n"\
		"[1;32mGenre[0m     [1;34m:[0m %-40s\n"\
		"[1;32mInfo[0m      [1;34m:[0m Mpeg-[1;33m%d[0m layer "\
		"[1;33m%d[0m at [1;33m%d[0mHz, "\
		"[1;33m%d[0mkb/s ([1;33m%s[0m)\n\n",
		flnam, balkje, header->artist, header->songname, header->type,
		header->year, header->etc, track, genre_table[header->genre],
		mp3info.version, mp3info.layer, mp3info.frequency, mp3info.bitrate,
		mp3info.mode);
	
	delete mp3tje;
	delete oldtag;
	delete header;

	return 0;
}

//adds a record to myasql database. Gets info from global mp3info and
//from given id3header.

//needed for libmpegsound; not pretty.

void 
debug(const char *fmt, ... )
{
	va_list ap;
	va_start(ap,fmt);
	va_end(ap);
	if(fmt);
}
void
addfiletolist(const char *fn)
{
	struct _filelist *fl, *newfl;

	if (!filelist)
	{
		filelist = new _filelist;
		filelist->next = NULL;
		filelist->filename = strdup(fn);
	}
	else
	{
		fl = filelist;
		while (fl->next) fl = fl->next;
		newfl = new _filelist;
		newfl->next = NULL;
		newfl->filename = NULL;
		fl->next = newfl;
		newfl->filename = strdup(fn);
	}
}
