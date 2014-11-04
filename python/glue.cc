/*
 * C interface for libmpegsound. This is the quickest way to make the library
 * available in python using ctypes.
 */
#include <mpegsound.h>

#include "config.h"

extern "C" {

/* 
 * Audio Device Code
 */

// Labels for all types of audio drivers
const int AUDIODEV_OSS = 0;
const int AUDIODEV_SDL = 1;
const int AUDIODEV_ESD = 2;
const int AUDIODEV_NAS = 3;
const int AUDIODEV_FILE_RAW = 4; //write to file
const int AUDIODEV_FILE_WAV = 5; //write to file

// All Audio Devices supported by this compiled instance
const int supported_audio_devices[] = {
#ifdef WANT_OSS
    AUDIODEV_OSS,
#endif 
#ifdef WANT_NAS
    AUDIODEV_NAS,
#endif 
#ifdef WANT_ESD
    AUDIODEV_ESD,
#endif 
#ifdef WANT_SDL
    AUDIODEV_SDL,
#endif 
    AUDIODEV_FILE_RAW,
    AUDIODEV_FILE_WAV
};

Soundplayer* opendevice(const int devtype, const char *device) {
    Soundplayer *driver = NULL;

    switch (devtype) {
    case AUDIODEV_FILE_RAW:
        driver = Rawtofile::opendevice(device);
        if (!((Rawtofile*)driver)->setfiletype(RAW)) {
            delete driver;
            driver = NULL;
        }
        break;
    case AUDIODEV_FILE_WAV:
        driver = Rawtofile::opendevice(device);
        if (!((Rawtofile*)driver)->setfiletype(WAV)) {
            delete driver;
            driver = NULL;
        }
        break;
#ifdef WANT_OSS
    case AUDIODEV_OSS:
        driver = Rawplayer::opendevice(device);
        break;
#endif
#ifdef WANT_NAS
    case AUDIODEV_NAS:
        driver = NASplayer::opendevice(device);
        break;
#endif    
#ifdef WANT_ESD
    case AUDIODEV_ESD:
        driver = new EsdPlayer(); //TODO: accept HOST param
        break;
#endif
#ifdef WANT_SDL
    case AUDIODEV_SDL:
        driver = new SDLPlayer();
        break;
#endif
    }

    return driver;
}

}
