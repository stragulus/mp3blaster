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

private:
	WINDOW *interface;
	short nrlines, nrcols;
	playstatus status;
};

#endif /* _CLASS_PLAYWINDOW_ */
