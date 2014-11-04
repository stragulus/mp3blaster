
  MP3Tag
  ------

   MP3Tag is a simple command line program to view, or edit the tags contained
 in MP3 files.  These tags include the songname, albumn name, etc.

   This was originally written by Bram Avontuur (brama@stack.nl), and 
  modified a little by myself to allow it to compile under Windows NT.

  
  Compiling with VC 5
  -------------------

   To compile from the sources ensure that the Developers studio binaries
 are on your PATH, then type 

   nmake -f mp3tag.mak

  
  Compiling with GCC
  ------------------

   To compile with GCC, (I used GNU Mingw32 v2.8.1), ensure the compiler is on your
  PATH, and setup properly, then type

   gcc -o mp3tag.exe id3parse.cpp mp3tag.cpp getopt.c


Steve
---
http://www.tardis.ed.ac.uk/~skx/