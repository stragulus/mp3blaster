mp3blaster
==========

The official repo of mp3blaster

# Installation (downloaded tarball)

## Quick & dirty

    ./configure --prefix=$HOME
    make
    make install

If compilation fails, you are probably missing required system packages. At a minimum, you will need a support sound
driver and ncurses5 development files. On Debian stable, you can install them as such:

    sudo apt-get install build-essential libncurses5-dev libsdl1.2-dev

Suggested system packages that include relevant audio output drivers and all the supported audio formats:

    sudo apt-get install build-essential libncurses5-dev libsdl1.2-dev libvorbisfile libogg-dev libvorbis-dev libsidplay1-dev

Suggested configure commandline to go with that:

    ./configure --with-oggvorbis --with-sidplay --with-sdl

# Installation (git checkout)

You will need to install automake for this to work.

Change to the checked out directory, and then run the following command to generate all the autotools-related files:

    autoreconf -i

Now, follow the instructions for the tarball install.

# Running

I'd suggest compiling in SDL support for audio playback. This way, mp3blaster will support more audio output devices
out of the box without requiring any additional configuration.
