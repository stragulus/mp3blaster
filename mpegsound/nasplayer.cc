/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Nasplayer.cc
// Playing raw audio over Network Audio System (by NCD)
// By Willem (willem@stack.nl)

#ifdef HAVE_NASPLAYER
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mpegsound.h"
#include <unistd.h>

NASplayer::NASplayer(AuServer *aud)
{
	this->aud = aud;
	dev = AuNone;
	format = AuNone;
	flow = 0;
	samplerate = 0;
	channels = 0;
	buffer_ms = 2500;
	req_size = 0;
}

NASplayer::~NASplayer()
{
	if (aud) {
		if (flow) AuDestroyFlow(aud, flow, 0);
		AuCloseServer(aud);
	}
}

NASplayer *NASplayer::opendevice(char *server)
{
	AuServer *aud;
	aud = AuOpenServer(server, 0, 0, 0, 0, 0);
	if (!aud) return NULL;
	return new NASplayer(aud);
}

void NASplayer::abort(void)
{
	if (!aud || !flow) return;
	AuStopFlow(aud, flow, 0);
	AuHandleEvents(aud);
	req_size = 0;
	AuStartFlow(aud, flow, 0);
}

bool NASplayer::setsoundtype(int stereo, int samplesize, int speed)
{
	int changed = 0;
	unsigned char newf = AuNone;
	if (samplesize == 16) newf = AuFormatLinearSigned16LSB;
	//else newf = AuFormatLinearSigned8;
	else newf = AuFormatLinearUnsigned8;
	stereo = stereo ? 2 : 1;
	if (stereo != channels) { channels = stereo; changed++; }
	if (samplerate != speed) { samplerate = speed; changed++; }
	if (newf != format) { format = newf; changed++; }
	if (changed) return resetsoundtype();
	return true;
}

static AuBool static_event_handler(AuServer *, AuEvent *ev,
					AuEventHandlerRec *hnd)
{
	NASplayer *np = (NASplayer *)(hnd->data);
	return np->event_handler(ev);
}

bool NASplayer::resetsoundtype(void)
{
	if (!aud) return seterrorcode(SOUND_ERROR_DEVOPENFAIL);
	if (flow) {
		AuUnregisterEventHandler(aud, evhnd);
		AuDestroyFlow(aud, flow, 0);
	}
	flow = 0;
	int i = 0;
	while(true) {
		if (i >= AuServerNumDevices(aud))
			return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
		if ((AuDeviceKind(AuServerDevice(aud, i)) ==
				AuComponentKindPhysicalOutput) &&
			AuDeviceNumTracks(AuServerDevice(aud, i)) == channels)
		{
			dev = AuDeviceIdentifier(AuServerDevice(aud, i));
			break;
		}
		i++;
	}
	flow = AuCreateFlow(aud, 0);
	if (!flow) return seterrorcode(SOUND_ERROR_DEVCTRLERROR);
	AuElement elms[3];
	int buffer_size = (buffer_ms * samplerate) / 1000;
	if (buffer_size < 8192) buffer_size = 8192;
	AuMakeElementImportClient(elms, samplerate, format, channels, AuTrue,
				buffer_size, buffer_size / 2, 0, 0);
	AuMakeElementExportDevice(elms+1, 0, dev, samplerate,
				AuUnlimitedSamples, 0, 0);
	AuSetElements(aud, flow, AuTrue, 2, elms, 0);
	evhnd = AuRegisterEventHandler(aud, AuEventHandlerIDMask, 0, flow,
				static_event_handler, (AuPointer) this);
	AuStartFlow(aud, flow, 0);
	req_size = 0;
	return true;
}

bool NASplayer::putblock(void *buffer, int size)
{
	while (size > req_size) {
		USLEEP(3);
		AuHandleEvents(aud);
	}
	req_size -= size;
	AuWriteElement(aud, flow, 0, size, buffer, AuFalse, 0);
	return true;
}

int NASplayer::getblocksize(void)
{
	return 2048;
}

int NASplayer::setvolume(int volume)
{
	if (!aud) return 0;
	AuDeviceAttributes *da = AuGetDeviceAttributes(aud, dev, NULL);
	if (!da) return 0;
	if (volume > 100) volume = 100;
	if (volume > 0) {
		AuDeviceGain(da) = AuFixedPointFromSum(volume, 0);
		AuSetDeviceAttributes(aud, AuDeviceIdentifier(da),
					AuCompDeviceGainMask, da, NULL);
		AuFreeDeviceAttributes(aud, 1, da);
		da = AuGetDeviceAttributes(aud, dev, NULL);
	}
	return AuFixedPointRoundUp(AuDeviceGain(da));
}

AuBool NASplayer::event_handler(AuEvent *ev)
{
	switch (ev->type) {
	case AuEventTypeElementNotify:
		AuElementNotifyEvent *event = (AuElementNotifyEvent *)ev;

		switch (event->kind) {
		case AuElementNotifyKindLowWater:
			req_size += event->num_bytes;
			break;
		case AuElementNotifyKindState:
			switch (event->cur_state) {
			case AuStatePause:
				if (event->reason != AuReasonUser)
					req_size += event->num_bytes;
				break;
			}
		}
	}
	return AuTrue;
}
#endif /*\ HAVE_NASPLAYER \*/
