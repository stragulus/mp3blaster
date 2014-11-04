#include "id3parse.h"
#include "genretab.h"
#include <string.h>
#include <errno.h>

/* tampers with 's' to replace non-printable chars with dots. */
void
convert_to_sane_string(char *s)
{
	unsigned int
		cnt = strlen(s),
		i;

	for (i = 0; i < cnt; i++)
	{
		if (s[i] < (char)32)
			s[i] = '.';
	}
}

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
	song->track = -1;
	//memset(song->genre_txt, 0, 41 * sizeof (char));
	//strncpy(song->genre_txt, "Not supported yet!",40);
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

	if ( fseek(fp, -128, SEEK_END) < 0)
		return success;

	for(;;)
	{
		if ( (c = fgetc(fp)) < 0)
			break;
		if (c == 0x54)
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

const char *
id3Parse::getGenre(const unsigned char genre)
{
#if 0
  int i;
  struct _genre_table *tmp=&genre_table[0];

  for(i=0;tmp[i].text != NULL;i++) {
    if(tmp[i].genre == genre)
      return tmp[i].text;
  }

  return "Unknown type";
#endif
	return genre_table[genre];
}

struct id3header *
id3Parse::parseID3()
{
	FILE *fp;
	int success = 0;

	if (!(fp = fopen(flnam, "r")))
		return NULL;
	if (!search_header(fp))
	{
		fclose(fp);
		return NULL;
	}

	char buf[40];

	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->songname, buf, 30);
		song->songname[30] = '\0';
		convert_to_sane_string(song->songname);
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->artist, buf, 30);
		song->artist[30] = '\0';
		convert_to_sane_string(song->artist);
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->type, buf, 30);
		song->type[30] = '\0';
		convert_to_sane_string(song->type);
		success++;
	}
	if (fread(buf,  4, sizeof(char), fp) ==  sizeof(char))
	{
		strncpy(song->year, buf, 4);
		song->year[4] = '\0';
		convert_to_sane_string(song->year);
		success++;
	}
	if (fread(buf, 30, sizeof(char), fp) == sizeof(char))
	{
		strncpy(song->etc, buf, 30);
		song->etc[30] = '\0';
		if (buf[28] == '\0' && buf[29] != '\0')
		{
			//ID3V1.1 Standard - specify track in last byte of comment field
			song->track = buf[29];
		}
		convert_to_sane_string(song->etc);
		success++;
	}
	if (fread(buf, 1, sizeof(char), fp) == sizeof(char))
	{
		song->genre = (unsigned char)buf[0];
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
		0x54, 0x41, 0x47 };

	printf("Writing header..\n");
	if ( fwrite((void*)c, sizeof(char), 3, fp) != 3)
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

	if (newID3->track != -1)
	{
		//ID3V1.1 standard.
		newID3->etc[28] = '\0';
		newID3->etc[29] = newID3->track;
	}

	//In ANSI C it's required to do an intervening file positioning function
	//before mixing read/write on a stream..
	fseek(fp, ftell(fp), SEEK_SET);

	if (
		(fwrite(newID3->songname, sizeof(char), 30, fp) == 30) &&
		(fwrite(newID3->artist, sizeof(char), 30, fp) == 30) &&
		(fwrite(newID3->type, sizeof(char), 30, fp) == 30) &&
		(fwrite(newID3->year, sizeof(char), 4, fp) == 4) &&
		(fwrite(newID3->etc, sizeof(char), 30, fp) == 30) &&
		(fwrite(&(newID3->genre), sizeof(char), 1, fp) == 1))
		success = 1;

	fclose(fp);
	return success;
}
