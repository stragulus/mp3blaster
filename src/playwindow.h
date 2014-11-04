#ifndef _CLASS_PLAYWINDOW_
#define _CLASS_PLAYWINDOW_

class playWindow
{
public:
	playWindow();
	~playWindow();

	void setFileName(const char *flname);
	void setStatus(playstatus ps);
	void setProperties(int layer, int freq, int bitrate, bool stereo);
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
#ifdef DEBUG
	void setFrames(int frames);
#endif

private:
	WINDOW *interface;
	short nrlines, nrcols;
	playstatus status;
	int progress[2];
};

#endif /* _CLASS_PLAYWINDOW_ */
