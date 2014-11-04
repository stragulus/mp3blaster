#include "id3parse.h"
#include <string.h>
#include <errno.h>

id3Parse::id3Parse(const char *filename)
{
	flnam = new char[strlen(filename)+1];
	strcpy(flnam, filename);

	song = new id3header;
	memset(song->songname, 0, 31 * sizeof (char));
	memset(song->artist, 0, 31 * sizeof (char));
	memset(song->type, 0, 31 * sizeof (char));
	memset(song->year, 0, 5 * sizeof (char));
	memset(song->etc, 0, 31 * sizeof (char));
	song->genre = '\0';
}

id3Parse::~id3Parse()
{
	delete song;
	delete[] flnam;
}

int
id3Parse::search_header(FILE *fp)
{
	int c, flag = 0, success = 0;

	if ( fseek(fp, -160, SEEK_END) < 0)
		return success;

	for(;;)
	{
		if ( (c = fgetc(fp)) < 0)
			break;
		else if (c == 0xFF)
			flag++;
		else if (c == 0x54 && flag >=28)
		{
			if ( fgetc(fp) == 0x41)
			{
				if (fgetc(fp) == 0x47)
				{
					success = 1;
					break;
				}
			}
			flag = 0;
		}
		else
			flag = 0;
	}

	return success;
}

struct id3header *
id3Parse::parseID3()
{
	FILE *fp;
	int success = 0;

	if (!(fp = fopen(flnam, "r")) || !search_header(fp))
		return NULL;

	char buf[40];

	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->songname, buf, 30);
		song->songname[30] = '\0';
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->artist, buf, 30);
		song->artist[30] = '\0';
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->type, buf, 30);
		song->type[30] = '\0';
		success++;
	}
	if (fread(buf,  4, sizeof(char), fp) ==  sizeof(char))
	{
		strncpy(song->year, buf, 4);
		song->year[4] = '\0';
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->etc, buf, 30);
		song->etc[30] = '\0';
		success++;
	}
	if (fread(buf, 1, sizeof(char), fp) == sizeof(char))
	{
		strncpy(&(song->genre), buf, 1);
		success++;
	}
	fclose(fp);

	if (success == 6)
		return song;
	else
		return NULL;
}

int
id3Parse::appendNewID3Header(FILE *fp)
{
	int success = 0;

	if (fseek(fp, 0, SEEK_END) < 0)
		return success;

	char c[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x54, 0x41, 0x47 };

	printf("Writing header..\n");
	if ( fwrite((void*)c, sizeof(char), 31, fp) != 31)
		return success;
	
	return (success = 1);
}

int
id3Parse::writeID3(struct id3header *newID3)
{
	int success = 0;
	FILE *fp = fopen(flnam, "r+");

	if (!fp)
		return success;

	if (!search_header(fp) && !(appendNewID3Header(fp)))
	{
		fclose(fp);
		return success;
	}

	size_t c;

	if (
		((c = fwrite(newID3->songname, sizeof(char), 30, fp)) == 30) &&
		(fwrite(newID3->artist, sizeof(char), 30, fp) == 30) &&
		(fwrite(newID3->type, sizeof(char), 30, fp) == 30) &&
		(fwrite(newID3->year, sizeof(char), 4, fp) == 4) &&
		(fwrite(newID3->etc, sizeof(char), 30, fp) == 30) &&
		(fwrite(&(newID3->genre), sizeof(char), 1, fp) == 1))
		success = 1;

	printf("c=%d\n", c); fflush(stdout);
	fclose(fp);
	return success;
}
