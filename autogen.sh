#!/bin/sh
autoreconf -i
##Scan configure.ac for AM_ macros and stuff them in aclocal.m4
#aclocal
###Regenerate config.h.in
#autoheader
###Generate Makefile.in from Makefile.am
#automake --add-missing --copy
###Generate configure from configure.ac
#autoconf
