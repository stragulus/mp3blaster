/*\
|*|  Class for playing C=64 tunes
\*/

#include <mpegsound.h>
#include "mpegsound_locals.h"

#ifdef HAVE_SIDPLAYER
#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif

SIDfileplayer::SIDfileplayer()
{
	bufSize = 0;
	buffer = 0;
	tune = 0;
	emu.getConfig(emuConf);
#if 0
	emuConf.frequency = 32000;
	emuConf.channels = SIDEMU_MONO;
	/*\
	emuConf.autoPanning = SIDEMU_CENTEREDAUTOPANNING;
	emuConf.volumeControl = SIDEMU_FULLPANNING;
	\*/
	emuConf.bitsPerSample = SIDEMU_8BIT;
	emuConf.sampleFormat = SIDEMU_SIGNED_PCM;
#endif
}

SIDfileplayer::~SIDfileplayer()
{
	if (tune) delete tune;
	if (buffer) delete buffer;
}

bool SIDfileplayer::openfile(char *filename, char *device, soundtype write2file)
{
	int ssize;

	if (!opendevice(device, write2file))
		return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
	if (tune) delete tune;
	tune = new sidTune(filename);
	if ((!tune) || (!*tune))
		return seterrorcode(SOUND_ERROR_FILEOPENFAIL);
	
	ssize = (emuConf.bitsPerSample == SIDEMU_16BIT ? AFMT_S16_NE : AFMT_S8);

	player->setsoundtype((emuConf.channels == SIDEMU_STEREO?1:0),
				ssize, emuConf.frequency);
	emu.setConfig(emuConf);
	if (bufSize != player->getblocksize()) {
		bufSize = player->getblocksize();
		if (buffer) delete buffer;
		buffer = new char[bufSize];
		if (!buffer) {
			bufSize = 0;
			return seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
		}
	}
	tune->getInfo(sidInfo);
	song = sidInfo.startSong;
	if (!sidEmuInitializeSong(emu, *tune, song))
		return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	tune->getInfo(sidInfo);
	return true;
}

void SIDfileplayer::closefile(void)
{
	if (tune)
		delete tune;
}

void SIDfileplayer::setforcetomono(short flag)
{
	/*\
	emuConf.channels = flag ? SIDEMU_MONO : SIDEMU_STEREO;
	emu.setConfig(emuConf);
	\*/
}

void SIDfileplayer::setdownfrequency(int value)
{
	emuConf.frequency = value;
	emu.setConfig(emuConf);
}

bool SIDfileplayer::run(int frames)
{
	tune->getInfo(sidInfo);
	if (song != sidInfo.currentSong)
		if (!sidEmuInitializeSong(emu, *tune, song))
			return seterrorcode(SOUND_ERROR_FILEREADFAIL);
	while (--frames >= 0) {
		sidEmuFillBuffer(emu, *tune, buffer, bufSize);
		if (!player->putblock(buffer, bufSize)) return false;
	}
	return true;
}

bool SIDfileplayer::playing()
{
	while (run(1));
	return false;
}
#endif /* HAVE_SIDPLAYER */
