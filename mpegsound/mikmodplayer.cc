/*\
|*|  Class for playing mod tunes
\*/
#include "config.h"

#include "mpegsound.h"
#include "mpegsound_locals.h"

#ifdef HAVE_MIKMODPLAYER

#include <mikmod.h>

/* This excerpt is a modified version of /drivers/drv_nos.c of mikmod */
/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: drv_nos.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for no output

==============================================================================*/

/*

	Written by Jean-Paul Mikkers <mikmak@via.nl>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

//#include "mikmod_internals.h"

#define ZEROLEN 32768

static	SBYTE *zerobuf=NULL;
static  ULONG samples = 0;

static BOOL MM_IsThere(void)
{
	return 1;
}

static BOOL MM_Init(void)
{
	zerobuf=(SBYTE*)malloc(ZEROLEN);
	return VC_Init();
}

static void MM_Exit(void)
{
	VC_Exit();
	free(zerobuf);
}

static void MM_Update(void)
{
	if (zerobuf)
		samples = VC_WriteBytes(zerobuf,ZEROLEN);
}

MIKMODAPI MDRIVER drv_mikmod={
	NULL,
	"Mp3blaster Sound",
	"Mp3Blaster Driver v1.0",
	(UBYTE)255, // hardware voice limit
        (UBYTE)255, // software voice limit
	"mp3blaster", // alias
	NULL, //cmdline helper
        NULL, //cmdline
	MM_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	MM_Init,
	MM_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	MM_Update,
	NULL,
	VC_VoiceSetVolume,
	VC_VoiceGetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceGetFrequency,
	VC_VoiceSetPanning,
	VC_VoiceGetPanning,
	VC_VoicePlay,
	VC_VoiceStop,
	VC_VoiceStopped,
	VC_VoiceGetPosition,
	VC_VoiceRealVolume
};
/* END: This excerpt is a modified version of /drivers/drv_nos.c of mikmod */

bool MIKMODfileplayer::initialized = false;

MIKMODfileplayer::MIKMODfileplayer(audiodriver_t audiodriver)
{
	set_driver(audiodriver);

	module = NULL;
	if (!initialized) {
		init();
	}

}


void MIKMODfileplayer::init() {

	MikMod_RegisterDriver(&drv_mikmod);
	MikMod_RegisterAllLoaders();

	md_mode |= DMODE_SOFT_MUSIC | DMODE_16BITS | DMODE_STEREO;

	if (MikMod_Init((char*)"")) {
		debug("Could not initialize sound, reason: %s\n", MikMod_strerror(MikMod_errno));
		return;
	}

	atexit(MikMod_Exit);

	initialized = true;
	debug("MIKMOD initialized\n");
}


bool MIKMODfileplayer::openfile(const char* filename, const char* device, soundtype write2file)
{

	if (!initialized)
		return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

	if (!opendevice(device, write2file))
		return seterrorcode(SOUND_ERROR_DEVOPENFAIL);

	module = Player_Load((char*)filename, 128, 0);

	if (!module) {
		debug("Could not load module, reason: %s\n", MikMod_strerror(MikMod_errno));
		return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
	}

	if (module->songname) {
		memset(info.songname, 0x0, 31);
		strncpy(info.songname, module->songname, 30);
	}
	if (module->comment) {
		memset(info.comment, 0x0, 31);
		strncpy(info.comment, module->comment, 30);
	}
	if (module->modtype) {
		memset(info.artist, 0x0, 31);
		strncpy(info.artist, module->modtype, 30);
	}

	info.samplerate = 44100;	// TODO: check mikmod parameters if we rlly play with 44100Hz

	// TODO: check for md_mode & DMODE_16BITS resp. md_mode & DMODE_STEREO
	player->setsoundtype(2, 16, md_mixfreq);

	return true;
}

void MIKMODfileplayer::closefile(void)
{
	if (!initialized)
		return;

	Player_Stop();

	if (module != NULL) {
		Player_Free(module);
		module = NULL;
	}

}


bool MIKMODfileplayer::run(int /*sec*/)
{
	if (!initialized)
		return false;

	if (!Player_Active())
		Player_Start(module);

	MikMod_Update();
	return player->putblock(zerobuf, samples);
}

bool MIKMODfileplayer::playing()
{
	if (!initialized || module == NULL)
		return false;
	return Player_Active();
}

int MIKMODfileplayer::elapsed_time()
{
	if (!initialized || module == NULL)
		return 0;
	return module->sngtime/1024 + 1;
}

void MIKMODfileplayer::skip(int sec)
{
	if (!initialized || module == NULL || !Player_Active())
		return;

	if (sec == 0)
		return;
	else if (sec < 0)
		Player_PrevPosition();
	else
		Player_NextPosition();
}

bool MIKMODfileplayer::pause()
{
	if (!initialized || module == NULL || !Player_Active())
		return false;
	if (!Player_Paused())
		Player_TogglePause();
	return true;
}

bool MIKMODfileplayer::unpause()
{
	if (!initialized || module == NULL || !Player_Active())
		return false;
	if (Player_Paused())
		Player_TogglePause();
	return true;
}

bool MIKMODfileplayer::stop()
{
	if (!initialized || module == NULL || !Player_Active())
		return false;
	Player_Stop();
	return true;
}

#endif // HAVE_MIKMODPLAYER
