/* MP3Blaster - An Mpeg Audio-file player for Linux
 * Copyright (C) Bram Avontuur (brama@stack.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "mp3blaster.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mp3stack.h"

extern int current_group; /* from main.cc */

/* returns a playlist of MP3's in an order defined by playmode pm (see 
 * mp3blaster.h for possible playmodes). The returned pointer is allocated
 * using new, so delete[] it nicely (delete[] each member, than the pointer)
 * (it is stored in char **mp3list). The amount of filenames in mp3list is 
 * stored in nmp3s.
 */
char** mp3Stack::getShuffledList(playmode pm, unsigned int *nmp3s)
{
	time_t
		t;
	int
		*song_id = NULL;
	char
		**mp3list = NULL;
	unsigned int
		ngroups = this->stackSize(),
		i,
		nsongs = 0;

	srandom(time(&t));
	
	if (pm == PLAY_SONGS) /* play all songs in random order */
	{
		for (i = 0; i < ngroups; i++)
			nsongs += (this->entry(i))->getNitems();
			//OLD: nsongs += group_stack[i]->nitems;
		
		//OLD: playlist->listsize = nsongs;
		//OLD: playlist->song_id = (int *)malloc(nsongs * sizeof(int));
		song_id = new int[nsongs];
		
		for (i = 0; i < nsongs; i++)
			song_id[i] = i;
			//OLD: playlist->song_id[i] = i;

		/* the kewl 'card-shuffle' algorithm */
		for (i = 0; i < nsongs; i++)
		{
			int
				rn = random()%nsongs,
				tmp;

			//OLD: tmp = playlist->song_id[i];
			//OLD: playlist->song_id[i] = playlist->song_id[rn];
			//OLD: playlist->song_id[rn] = tmp;
			tmp = song_id[i];
			song_id[i] = song_id[rn];
			song_id[rn] = tmp;
		}

		/* song_id[0..nsongs-1], each element being [0..nsongs-1] */
	}
	else if (pm == PLAY_GROUP)
	{
		//OLD: sw_window *sw = group_stack[current_group - 1];
		scrollWin *sw = this->entry(current_group - 1);
		
		//OLD: playlist->song_id = (int *)malloc((sw->nitems) * sizeof(int));
		//playlist->listsize = sw->nitems;

		nsongs = sw->getNitems();
		song_id = new int[nsongs];

		srandom(time(&t));
		compose_group(current_group - 1, song_id, 0);
	}
	else if (pm == PLAY_GROUPS)
	{		
		int
			precount = 0;
		for (i = 0; i < ngroups; i++)
			//OLD: nsongs += group_stack[i]->nitems;
			nsongs += (this->entry(i))->getNitems();
		
		//OLD: playlist->listsize = nsongs;
		//OLD: playlist->song_id = (int *)malloc(nsongs * sizeof(int));
		song_id = new int[nsongs];
		//printf("nsongs=%d, ngroups=%d\n",nsongs, ngroups);fflush(stdout);

		for (i = 0; i < ngroups; i++)
		{
			srandom(time(&t));
			compose_group(i, song_id, precount);
			//OLD: precount += group_stack[i]->nitems;
			precount += (this->entry(i))->getNitems();
		}
	}
	else if (pm == PLAY_GROUPS_RANDOMLY)
	{
		int
			//OLD: *grouplist = (int *)malloc(ngroups * sizeof(int)),
			*grouplist = new int[ngroups],
			precount = 0;
		
		for (i = 0; i < ngroups; i++)
			//OLD: nsongs += group_stack[i]->nitems;
			nsongs += (this->entry(i))->getNitems();

		//OLD: playlist->listsize = nsongs;
		//OLD: playlist->song_id = (int *)malloc(nsongs * sizeof(int));
		song_id = new int[nsongs];
		
		for (i = 0; i < ngroups; i++)
			grouplist[i] = i;
		for (i = 0; i < ngroups; i++)
		{
			int
				rn = random()%ngroups,
				tmp = grouplist[i];

			grouplist[i] = grouplist[rn];
			grouplist[rn] = tmp;
		}

		/* grouplist is array with group-id's in random order */
		for (i = 0; i < ngroups; i++)
		{
			srandom(time(&t));
			compose_group(grouplist[i], song_id, precount);
			//OLD: precount += group_stack[grouplist[i]]->nitems;
			precount += (this->entry(grouplist[i]))->getNitems();
		}

		if (grouplist)
			//OLD: free(grouplist);
			delete[] grouplist;
	}
	
	//OLD: play_entire_list();
	// nsongs is #songs to be returned (as mp3 filenames)
	mp3list = NULL;
	mp3list = new char*[nsongs];

	for (i = 0; i < nsongs; i++)
	{
		unsigned int
			sid, gid, j;
		int
			a;

		a = song_id[i];
		sid = gid = 0;
		
		for (j = 0; j < ngroups; j++)
		{
			//OLD: a -= (group_stack[j]->nitems);
			a -= (this->entry(j))->getNitems();

			if (a < 0)
			{
				gid = j;
				//OLD: sid = a + group_stack[j]->nitems;
				sid = a + (this->entry(j))->getNitems();
				break;
			}
		}
	
		//OLD: filename = group_stack[gid]->items[sid];
		char *tmpbla = (this->entry(gid))->getItem(sid);
		//printf("tmpbla=%s\n", tmpbla);fflush(stdout);
		mp3list[i] = new char[strlen(tmpbla) + 1];
		strcpy(mp3list[i], tmpbla);
		//printf("mp3list[%d]=%s\n", i, mp3list[i]);fflush(stdout);
		delete[] tmpbla; /* yeah getItem allocates it..duh? */
	}

	*nmp3s = nsongs;
	return mp3list;
}

/* PRE: arr is malloced in at least range [offset, offset + group_stack[group]->
 * nitems-1] && srandom() is called with a seed like time()
 * POST: arr[offset..offset+group_stack[group]->nitems-1] is filled with song-
 * id's for the songs in group_stack[group]. The random/normal playmode for
 * that group is also considered.
 */
void
mp3Stack::compose_group(int group, int *arr, int offset)
{
	scrollWin 
		//OLD: [struct sw_window] *sw = group_stack[group];
		*sw = this->entry(group);
	int
		i,
		precount = 0;

	for (i = 0; i < group; i++)
		//OLD: precount += group_stack[i]->nitems;
		precount += (this->entry(i))->getNitems();
		
	//OLD: for (i = 0; i < sw->nitems; i++)
	for (i = 0; i < sw->getNitems(); i++)
		arr[offset + i] = precount + i;
	
	if (sw->sw_playmode == 1) /* randomly play songs in group */
	{
		//OLD: for (i = 0; i < sw->nitems; i++)
		for (i = 0; i < sw->getNitems(); i++)
		{
			int
				//OLD: rn = random()%sw->nitems,
				rn = random()%sw->getNitems(),
				tmp = arr[offset + i];

			arr[offset + i] = arr[offset + rn];
			arr[offset + rn] = tmp;
		}
	}
}
