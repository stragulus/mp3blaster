#ifdef NEWINTERFACE
#ifndef _MP3BLASTER_INTERFACE_
#define _MP3BLASTER_INTERFACE_
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "mp3blaster.h"
#include <string>
#include <map>

using namespace std;

struct helpitem_t
{
	char label[4];
	char *description;
};

enum keytype_t { KEYTYPE_NONE, KEYTYPE_CD_PLAY, KEYTYPE_CD_PAUSE };
enum songtype_t { STYPE_UNKNOWN, STYPE_MP3, STYPE_OGG, STYPE_WAV, STYPE_SID };

typedef map< string, string, less< string > > charmap;

class Interface
{
public:
	Interface();
	virtual ~Interface();

	//TODO make everything virtual and set to '= 0'
	virtual void setHelpItems(helpitem_t *helpItems) = 0;
	virtual void setShuffle(bool enabled) = 0;
	virtual void setRepeat(bool enabled) = 0;
	virtual void setSongElapsedTime(int seconds) = 0;
	virtual void setSongTotalTime(int seconds) = 0;
	virtual void setSongRemainingTime(int seconds) = 0;
	virtual void setKeyLabel(keytype_t type, int keyCode) = 0;
	virtual void setMixerDevice(const char *deviceName) = 0;
	virtual void setMixerPercentage(unsigned short percentage) = 0;
	virtual void setPlayingStatus(playstatus_t status) = 0;
	virtual void setGlobalPlayback(playmode mode) = 0;
	virtual void setNextSong(const char *songName) = 0;
	virtual void setSongInfo(songtype_t songType, charmap songInfo) = 0;
	virtual void setMainWindowTitle(const char *title) = 0;
	virtual void setSortMode(sortmodes_t sortMode) = 0;

	//supporting functions
protected:
	virtual char *getKeyLabelFromCode(int keyCode);
	virtual const char *getPlaymodeDescription(playmode mode);
	virtual const char *getSortModeDescription(sortmodes_t mode);
};
#endif /* _MP3BLASTER_INTERFACE_ */
#endif /* this code has not been finished yet! */
