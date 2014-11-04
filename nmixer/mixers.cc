/*\
|*|  Base class for handling different mixer types
\*/
#include <nmixer.h>
#include <string.h>

#define MIXER_ID_SHIFT 12
#define MIXER_ID_MASK (~((1 << MIXER_ID_SHIFT) - 1))

baseMixer::baseMixer(const char *mixerDevice, baseMixer *next)
{
	this->next = next;
	if (!mixerDevice)
		mixerDevice = MIXER_DEVICE;
	this->mixerDevice = strdup(mixerDevice);
	mixerID = 0;
	if (next) {
		devs = next->GetDevices(&num_devs);
		if (num_devs) {
			mixerID = (devs[num_devs - 1] & MIXER_ID_MASK) +
					(1 << MIXER_ID_SHIFT);
		}
	} else {
		devs = NULL;
		num_devs = 0;
	}
}
 
baseMixer::~baseMixer()
{
	if (devs)
		free(devs);
	if (mixerDevice)
		free(mixerDevice);
}

int *baseMixer::GetDevices(int *num)
{
	int *ret = new int[num_devs];
	*num = num_devs;
	memcpy(ret, devs, sizeof(int) * num_devs);
	return ret;
}

void baseMixer::AddDevice(int device)
{
	++num_devs;
	devs = (int *)realloc(devs, sizeof(int) * num_devs);
	devs[num_devs - 1] = device | mixerID;
}

bool baseMixer::SetMixer(int device, struct volume *vol)
{
	if ((device & MIXER_ID_MASK) == mixerID)
		return Set((device & ~MIXER_ID_MASK), vol);
	if (next) return next->SetMixer(device, vol);
	return false;
}

bool baseMixer::GetMixer(int device, struct volume *vol)
{
	if ((device & MIXER_ID_MASK) == mixerID)
		return Get(device & ~MIXER_ID_MASK, vol);
	if (next) return next->GetMixer(device, vol);
	return false;
}

const char *baseMixer::GetMixerLabel(int device)
{
	if ((device & MIXER_ID_MASK) == mixerID)
		return Label(device & ~MIXER_ID_MASK);
	if (next) return next->GetMixerLabel(device);
	return "<none>";
}
