/*\
|*|  OSSMixer
|*|  Class for accessing the OSS mixer device (/dev/mixer)
\*/
#include "config.h"

#ifdef WANT_OSS

#include <nmixer.h>

#ifdef __cplusplus
extern "C" {
#endif

#include SOUNDCARD_HEADERFILE

#ifdef __cplusplus
}
#endif

OSSMixer::OSSMixer(const char *mixerDevice, baseMixer *next) : baseMixer(mixerDevice, next)
{
	mixer = open(mixerDevice, O_RDWR);
	if (mixer < 0) return;
	unsigned int setting;
	ioctl(mixer, MIXER_READ(SOUND_MIXER_DEVMASK), &setting);
	for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
		if (setting & (1 << i)) AddDevice(i);
}

OSSMixer::~OSSMixer()
{
	close(mixer);
}

bool OSSMixer::Set(int device, struct volume *vol)
{
	int setting = (vol->left & 0xFF) + ((vol->right & 0xFF) << 8);
	return (ioctl(mixer, MIXER_WRITE(device), &setting) >= 0);
}

bool OSSMixer::Get(int device, struct volume *vol)
{
	int setting;
	if (ioctl(mixer, MIXER_READ(device), &setting) < 0)
		return false;
	vol->left  = setting & 0x00FF;
	vol->right = (setting & 0xFF00) >> 8;
	return true;
}

const char *OSSMixer::Label(int device)
{
	static const char *sources[] = SOUND_DEVICE_LABELS;
	if ((device < 0) || (device >= SOUND_MIXER_NRDEVICES))
		return "<OSS:none>";
	return sources[device];
}

bool OSSMixer::CanRecord(int device)
{
	unsigned int setting;

	ioctl(mixer, MIXER_READ(SOUND_MIXER_RECMASK), &setting);
	return (setting & (1 << device));
}

bool OSSMixer::SetRecord(int device, bool set)
{
	unsigned int setting;
	ioctl(mixer, MIXER_READ(SOUND_MIXER_RECSRC), &setting);

	if (set)
	{
		setting |= (1 << (unsigned int)device);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_RECSRC), &setting);
	}
	else
	{
		setting &= ~(1 << (unsigned int)device);
		ioctl(mixer, MIXER_WRITE(SOUND_MIXER_RECSRC), &setting);
	}

	return true;
}

bool OSSMixer::GetRecord(int device)
{
	unsigned int setting;

	ioctl(mixer, MIXER_READ(SOUND_MIXER_RECSRC), &setting);
	return (setting & (1 << device));
}

#endif /* WANT_OSS */
