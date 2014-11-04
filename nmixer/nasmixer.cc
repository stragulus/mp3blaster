#include "config.h"

/*\
|*| Network Audio System specific mixer routines
\*/
#ifdef HAVE_NASPLAYER
#include <string.h>
#include <nmixer.h>
#include <audio/audiolib.h>
#include <audio/audio.h>

static AuDeviceAttributes *
au_list_devices(AuServer *aud, int *ndev)
{
	AuDeviceAttributes *d;
	int i;
	i = *ndev = AuServerNumDevices(aud);
	if (i < 1) return NULL;
	d = (AuDeviceAttributes *)malloc(i * sizeof(AuDeviceAttributes));
	while (--i >= 0) {
		AuDeviceAttributes *t = AuServerDevice(aud, i);
		memcpy(d + i, t, sizeof(AuDeviceAttributes));
	}
	return d;
}

NASMixer::NASMixer(const char *mixerDevice, baseMixer *next) : baseMixer(mixerDevice, next)
{
	aud = AuOpenServer(0, 0, 0, 0, 0, 0);
	if (!aud) return;
	ada = au_list_devices(aud, &num_ada);
	for (int i = 0; i < num_ada; i++) AddDevice(i);
}

NASMixer::~NASMixer()
{
	if (!aud) return;
	AuFreeDeviceAttributes(aud, num_ada, ada);
	AuCloseServer(aud);
}

bool NASMixer::Set(int device, struct volume *vol)
{
	if (!aud) return false;
	AuDeviceAttributes *da = &ada[device];
	AuDeviceGain(da) = AuFixedPointFromSum((vol->left + vol->right) / 2, 0);
	AuSetDeviceAttributes(aud, AuDeviceIdentifier(da),
			AuCompDeviceGainMask, da, NULL);
	AuFlush(aud);
	return true;
}

bool NASMixer::Get(int device, struct volume *vol)
{
	if (!aud) return false;
	AuDeviceAttributes *da = &ada[device];
	vol->left = vol->right = AuFixedPointIntegralAddend(AuDeviceGain(da));
	return true;
}

const char *NASMixer::Label(int device)
{
	if ((device < 0) || (device >= num_ada))
		return "<NAS:none>";
	return AuDeviceDescription(&ada[device])->data;
}
#endif
