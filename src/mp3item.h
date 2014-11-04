#include "winitem.h"
#include "mp3win.h"

class mp3Item : public winItem
{
public:
	mp3Item();
	~mp3Item();
	
	void setPlayed() { played = 1; }
	void setNotPlayed() { played = 0; }
	short isPlayed() { return played; }

private:
	short played;
};
