Name: mp3blaster
Summary: Mp3blaster is a text console audio player with an interactive interface
Version: 3.2.5
Release: 1
Group: Applications/Multimedia
Copyright: GPL
Url: http://mp3blaster.sourceforge.net/
Packager: Bram Avontuur <bram@avontuur.org>
Distribution: N/A 
Source: http://www.stack.nl/~brama/mp3blaster/src/%{name}-%{version}.tar.gz
Buildroot: /var/tmp/%{name}-%{version}-%{release}-root

%description
Mp3blaster is an audio player with a user-friendly interface that will run
on any text console. The interface is built using ncurses, and features all
common audio player controls. The playlist editor is very flexible and allows
nested groups (albums). Supported audio media: mp3, ogg vorbis, wav, sid and
streaming mp3 over HTTP.

%prep
%setup

%build
%configure
make

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README TODO
%{_bindir}/mp3blaster
%{_bindir}/mp3tag
%{_bindir}/splay
%{_bindir}/nmixer
%{_datadir}/mp3blaster/*
%{_mandir}/man1/mp3blaster.1*
%{_mandir}/man1/nmixer.1*
%{_mandir}/man1/splay.1*
