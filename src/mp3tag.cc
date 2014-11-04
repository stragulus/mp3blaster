#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <mpegsound.h>
#ifdef WANT_MYSQL
#include MYSQL_H
#endif
#include "id3parse.h"
#include "mp3blaster.h"

extern char *genre_table[];
_globalopts globalopts;

//MySQL global variables.
int
	cdnr = -1;
#ifdef WANT_MYSQL
int
	add_mysql_record(const struct id3header*);
#endif
char
	*fullpath = NULL,
	*cdtype = NULL,
	*flnam = NULL,
	*dbname = NULL,
	*table = NULL,
	*user = NULL,
	*passwd = NULL,
	*albumname = NULL,
	*owncomment = NULL;

struct _mp3info
{
	int bitrate;
	int layer;
	int version;
	int protection;
	int frequency;
	char mode[20];
} mp3info;

void
usage(const char *bla="mp3tag2")
{
	fprintf(stderr, "Usage:\t%s -f <filename> [-a <artist>] [-s <songn" \
		"ame>] [-l <album>] [-y <year>] [-e etcetera] [-g genre] [-r]\n",
		bla);
#ifdef WANT_MYSQL
	fprintf(stderr,
		"or\t%s -m mysql_dbname -f <filename> -n <pathoncd> "\
		"-t tablename -u user -p passwd -c cdnr -d <cdtype:(CD|HQ)> "\
		"[-l non-ID3 album] [-e <non-ID3 comment>]\n\n" \
		"With the -m option (which must be the first option given!) info "\
		"of the mp3 is added to a mysql database (all the other options"\
		"to alter the ID3 tag are thus useless)\n\n", bla);
#endif
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
		"\tGenre    : 1 integer [0..254]\n\n"\
		"If you don't specify any of the id3-tag fields, the file's id3tag"\
		" will be shown only.\n");
	exit(1);
}

void
copystring(char *a, const char *b, int n)
{
	memset(a, 0, (n + 1) * sizeof(char)); // clean string a
	strncpy(a, b, n);
}

int
main(int argc, char *argv[])
{
	int c, mergeID3 = 0, change = 0, add2sql = 0;
	struct id3header *hdr = new id3header;

	globalopts.debug = 0;
	memset(hdr->songname, 0, 31 * sizeof (char));
	memset(hdr->artist, 0, 31 * sizeof (char));
	memset(hdr->type, 0, 31 * sizeof (char));
	memset(hdr->year, 0, 5 * sizeof (char));
	memset(hdr->etc, 0, 31 * sizeof (char));
	hdr->genre = 255;

	while ( (c =  getopt(argc, argv, "f:a:c:d:e:g:l:m:n:p:rs:t:u:y:")) != EOF)
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
			case 'f':
				free(flnam);
				flnam = strdup(optarg);
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
			case 'm':
#ifdef WANT_MYSQL
				add2sql = 1;
				free(dbname);
				dbname = strdup(optarg);
#else
				fprintf(stderr, "WARNING: Mp3tag is not compiled with "\
					"MySQL support, bailing out!!!\n");
				usage();
#endif
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

	if (!flnam)
		 usage(argv[0]);

	Mpegtoraw *mp3;
	Soundinputstream *loader;
	Soundplayer *player = NULL;

	loader = new Soundinputstreamfromfile;
	if (!loader->open(flnam))
	{
		fprintf(stderr, "Error opening file.\n");
		exit(1);
	}
	
	mp3 = new Mpegtoraw(loader,player);
	if (!mp3->initialize(flnam))
	{
		fprintf(stderr, "Error initializing mp3.\n");
		exit(1);
	}

	struct id3header *oldtag = new id3header;

	memset(oldtag->songname, 0, 31 * sizeof (char));
	memset(oldtag->artist, 0, 31 * sizeof (char));
	memset(oldtag->type, 0, 31 * sizeof (char));
	memset(oldtag->year, 0, 5 * sizeof (char));
	memset(oldtag->etc, 0, 31 * sizeof (char));
	oldtag->genre = 255;

	//store all info about this mp3.
	strncpy(oldtag->songname, mp3->getname(), 30);
	strncpy(oldtag->artist, mp3->getartist(), 30);
	strncpy(oldtag->type, mp3->getalbum(), 30);
	strncpy(oldtag->year, mp3->getyear(), 4);
	strncpy(oldtag->etc, mp3->getcomment(), 30);
	mp3info.bitrate = mp3->getbitrate();
	mp3info.layer = mp3->getlayer();
	mp3info.version = mp3->getversion() + 1;
	mp3info.protection = (mp3->getcrccheck() ? 1 : 0);
	mp3info.frequency = mp3->getfrequency();
	memset(mp3info.mode, 0, 20 * sizeof(char));
	strncpy(mp3info.mode, mp3->getmodestring(), 19);

	//delete mp3;

#ifdef WANT_MYSQL
	if (add2sql)
	{
		return add_mysql_record(oldtag);
	}
#endif

	id3Parse *mp3tje = new id3Parse(flnam);
	if (change && mergeID3 && mp3tje->parseID3())
	{
		printf("ID3 tag in file detected. Going to merge!\n");

		if (!strlen(hdr->songname))
			copystring(hdr->songname, oldtag->songname, 30);
		if (!strlen(hdr->artist))
			copystring(hdr->artist, oldtag->artist, 30);
		if (!strlen(hdr->type))
			copystring(hdr->type, oldtag->type, 30);
		if (!strlen(hdr->year))
			copystring(hdr->year, oldtag->year, 4);
		if (!strlen(hdr->etc))
			copystring(hdr->etc, oldtag->etc, 30);
		if (hdr->genre == 255)
			hdr->genre = oldtag->genre;
	}

	if (change && !(mp3tje->writeID3(hdr)))
	{
		fprintf(stderr, "%s: Failed to write ID3 tag!\n", argv[0]);
		exit(1);
	}
	else if (change)
		printf("Wrote ID3 tag successfully!\n");
	
	if (!change) //only display current ID3 tag.
	{
		delete hdr;
		hdr = oldtag;
	}

	printf(
		"Artist    : %-30s\n"\
		"Songname  : %-30s\n"\
		"Album     : %-30s  Year: %-4s\n"\
		"Etcetera  : %-30s\n"\
		"Genre     : %-40s\n"\
		"Info      : Mpeg-%d layer %d at %dHz, %dkb/s (%s)\n",
		hdr->artist, hdr->songname, hdr->type,
		hdr->year, hdr->etc, genre_table[hdr->genre],
		mp3info.version, mp3info.layer, mp3info.frequency, mp3info.bitrate,
		mp3info.mode);
	
	delete mp3tje;

	return 0;
}

//adds a record to myasql database. Gets info from global mp3info and
//from given id3header.
#ifdef WANT_MYSQL
int
add_mysql_record(const struct id3header *id3)
{
	MYSQL mysql;
	struct stat finf;
	const char *flnam2;

	//did the user specify enough/correct parameters?
	if ( cdnr < 0 || !fullpath || !cdtype || !user || !passwd || !table ||
		!dbname || (strcmp(cdtype, "CD") && strcmp(cdtype, "HQ")) || !flnam)
		usage();

	if (stat(flnam, &finf) == -1)
	{
		perror("stat");
		return 1;
	}
	//remove trailing path from filename.
	if ( (flnam2 = strrchr(flnam, '/')) )
		flnam2 += 1;
	else
		flnam2 = flnam;

	mysql_init(&mysql);
	if (!mysql_real_connect(&mysql, "localhost", user, passwd, dbname, 0,
		NULL, 0))
	{
		fprintf(stderr, "Failed to connect to database: Error: %s\n",
			mysql_error(&mysql));
		return 1;
	}
	char tmp_flnam[strlen(flnam2)*2 + 1],
		tmp_songname[strlen(id3->songname)*2 + 1],
		tmp_artist[strlen(id3->artist)*2 + 1],
		tmp_id3album[strlen(id3->type)*2 + 1],
		tmp_comment[strlen(id3->etc)*2 + 1],
		tmp_year[strlen(id3->year)*2 + 1],
		tmp_table[strlen(table)*2 + 1],
		tmp_owncomment[(owncomment ? (strlen(owncomment)*2 +1) : 1)],
		tmp_album[(albumname ? (strlen(albumname)*2 +1) : 1)],
		tmp_fullpath[strlen(fullpath)*2 + 1];
	mysql_escape_string(tmp_flnam, flnam2, strlen(flnam2));
	mysql_escape_string(tmp_songname, id3->songname, strlen(id3->songname));
	mysql_escape_string(tmp_artist, id3->artist, strlen(id3->artist));
	mysql_escape_string(tmp_id3album, id3->type, strlen(id3->type));
	mysql_escape_string(tmp_comment, id3->etc, strlen(id3->etc));
	mysql_escape_string(tmp_year, id3->year, strlen(id3->year));
	mysql_escape_string(tmp_table, table, strlen(table));
	mysql_escape_string(tmp_fullpath, fullpath, strlen(fullpath));
	if (owncomment)
		mysql_escape_string(tmp_owncomment, owncomment, strlen(owncomment));
	else
		tmp_owncomment[0] = '\0';
	if (albumname)
		mysql_escape_string(tmp_album, albumname, strlen(albumname));
	else
		tmp_album[0] = '\0';

	char sql_query[strlen(tmp_flnam)+strlen(tmp_table)+2000];
	sprintf(sql_query, "INSERT INTO %s (CD,CDTYPE,FILE,PATH,FILESIZE,MPEGVER,"\
		"LAYER,BITRATE,FREQUENCY,MODE,ID3_SONGNAME,ID3_ARTIST,"\
		"ID3_ALBUM,ID3_COMMENT,ID3_YEAR,ID3_GENRE,COMMENT,ALBUM) VALUES(%u,"\
		"'%s','%s','%s',%u,%u,%u,%u,%u,'%s','%s','%s','%s','%s','%s',"\
		"%u,'%s','%s')", tmp_table, (unsigned int)cdnr, cdtype, tmp_flnam,
		tmp_fullpath, (unsigned int)finf.st_size, mp3info.version,
		mp3info.layer, mp3info.bitrate, mp3info.frequency, mp3info.mode,
		tmp_songname, tmp_artist, tmp_id3album, tmp_comment, tmp_year, 
		(unsigned int)id3->genre,tmp_owncomment, tmp_album);
	int result = mysql_query(&mysql, sql_query);

	if (result)
	{
		fprintf(stderr, "SQL Query failed! Error: %s\n",
			mysql_error(&mysql));
		return 1;
	}
	return 0;
}
#endif

//needed for libmpegsound; not pretty.
void debug(const char*msg) { if(!msg); }
