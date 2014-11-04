#ifndef _CLASS_PLAYWINDOW_
#define _CLASS_PLAYWINDOW_
#include <nmixer.h>
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif

class playWindow
{
public:
	playWindow();
	~playWindow();

	void setFileName(const char *flname);
	void setStatus(playstatus ps);
	void setProperties(int mpeg_version, int layer, int freq, int bitrate,
		bool stereo);
	playstatus getStatus(void) { return status; }
	chtype getInput();
	WINDOW *getCursesWindow() { return interface; }
	void redraw();
	void setProgressBar(int percentage);
	void setSongName(const char *sn);
	void setArtist(const char *ar);
	void setAlbum(const char *tp);
	void setSongYear(const char *yr);
	void setSongInfo(const char *inf);
	void setSongGenre(const char);
	void setTotalTime(int time);
	void updateTime(int time);

	NMixer* getMixerHandle() { return mixer; }
#ifdef DEBUG
	void setFrames(int frames);
#endif

private:
	WINDOW *interface;
	short nrlines, nrcols;
	playstatus status;
	int progress[2];
	NMixer *mixer;
	int totaltime;

	char *getGenre(const char);
};

#endif /* _CLASS_PLAYWINDOW_ */
