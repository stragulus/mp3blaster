/*\
|*|  NullMixer
|*|  Dummy mixer class when nothing usable is available
\*/

#include <nmixer.h>

NullMixer::NullMixer(const char *mixerDevice, baseMixer *next) : baseMixer(mixerDevice, next)
{
	(void)mixerDevice;
	(void)next;
	setting = 0;
	AddDevice(0); //a single dummy device
}

bool NullMixer::Set(int device, struct volume *vol)
{
	this->setting = (vol->left & 0xFF) + ((vol->right & 0xFF) << 8);
	return true;
}

bool NullMixer::Get(int device, struct volume *vol)
{
	vol->left  = setting & 0x00FF;
	vol->right = (setting & 0xFF00) >> 8;
	return true;
}

const char *NullMixer::Label(int device)
{
	return "null";
}

bool NullMixer::CanRecord(int device)
{
	(void)device;
	return false;
}

bool NullMixer::SetRecord(int device, bool set)
{
	(void)device;
	(void)set;
	return true;
}

bool NullMixer::GetRecord(int device)
{
	(void)device;
	return false;
}
