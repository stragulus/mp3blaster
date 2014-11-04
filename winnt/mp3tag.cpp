#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include "getopt.h"
#else
#include <unistd.h>
#endif

#include <string.h>
#include "id3parse.h"

void
usage(const char *bla)
{
	fprintf(stderr, "Usage: %s -f <filename> [-a <artist>] [-s <songn" \
		"ame>] [-t <album>] [-y <year>] [-e etcetera] [-g genre] [-r]\n\n" \
		"Flag -r merges new ID3-info with existing ID3-tag in file.\n"\
		"For a list of supported genres, use '-g list' "\
		"(the genres are compatible with WinAmp)\n" \
		"Maximum lengths:\n" \
		"\tArtist   : 30 characters\n"\
		"\tSongname : 30 characters\n"\
		"\tAlbum    : 30 characters\n"\
		"\tYear     :  4 characters\n"\
		"\tEtcetera : 30 characters\n\n"\
		"If you don't specify any of the id3-tag fields, the file's id3tag"\
		" will be shown only.\n", bla);
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
	int c, mergeID3 = 0, change = 0;
	char *flnam = NULL;
	struct id3header *hdr = new id3header;
	
	memset(hdr->songname, 0, 31 * sizeof (char));
	memset(hdr->artist, 0, 31 * sizeof (char));
	memset(hdr->type, 0, 31 * sizeof (char));
	memset(hdr->year, 0, 5 * sizeof (char));
	memset(hdr->etc, 0, 31 * sizeof (char));
	hdr->genre = '\0';

	while ( (c =  getopt(argc, argv, "f:a:s:t:y:e:rg:")) != EOF)
	{
		switch(c)
		{
			case ':':
				usage(argv[0]);
				break;
			case '?':
				usage(argv[0]);
				break;
			case 'f':
				if (flnam) delete[] flnam;
				flnam = new char[strlen(optarg) + 1];
				strcpy(flnam, optarg);
				break;
			case 'a':
				copystring(hdr->artist, optarg, 30);
				change = 1;
				break;
			case 's':
				copystring(hdr->songname, optarg, 30);
				change = 1;
				break;
			case 't':
				copystring(hdr->type, optarg, 30);
				change = 1;
				break;
			case 'y':
				copystring(hdr->year, optarg, 4);
				change = 1;
				break;
			case 'e':
				copystring(hdr->etc, optarg, 30);
				change = 1;
				break;
			case 'r': //Read tag first.
				mergeID3 = 1;
				break;
			case 'g': //genres
				printf("Feature not supported *yet*.\n");
				break;
		}
	}

	if (!flnam)
		 usage(argv[0]);
	
	id3Parse *mp3tje = new id3Parse(flnam);

	struct id3header *oldtag;

	if (change && mergeID3 && (oldtag = mp3tje->parseID3()))
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
		if (!(hdr->genre))
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
		hdr = mp3tje->parseID3();

	if (hdr)
	{
		printf(
			"Artist    : %-30s\n"\
			"Songname  : %-30s\n"\
			"Album     : %-30s  Year: %-4s\n"\
			"Etcetera  : %-30s\n"\
			"Genre     : %-40s\n", hdr->artist, hdr->songname, hdr->type,
			hdr->year, hdr->etc, "Not supported yet!");
	}
	else
         printf(
            "No MP3 tags found\n" );
           
	delete mp3tje;

	return 0;
}
