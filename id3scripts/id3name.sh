#!/bin/sh

#MP3TAG is the location of the mp3tag-program which is used to edit the
#ID3-tag of MP3-files
MP3TAG=/usr/local/bin/mp3tag

echo "This program can automatically put the songname in the ID3-tag of" \
     "an MP3-file, based on the filename (if it doesn't have an ID3-Tag " \
	 "yet, one will be added)." 
echo "Ofcourse, this means that there are some requirements for the " \
	 "syntax of the filename. First, the program will propose a few " \
	 "possible filename-syntaxes. Chose the syntax that resembles your "\
	 "filenaming first. When you have done this, it will propose a " \
	 "songname for all MP3-files in the current directory, and if you " \
	 "answer the proposal with a y[es], it will put the songname into the "\
	 "ID3-tag of the file (unless it fails to do so for some reason).\n"\
	 "If you have a suggestion for a file-syntax that is not listed " \
	 "here, and you just can't live without that specific syntax, you " \
	 "can mail me (Bram Avontuur, brama@stack.nl) with the request " \
	 "specifying the filename-syntax as exact as possible, with some " \
	 "examples. Oh, if you give '-y' as parameter to id3tag, it won't " \
	 "ask for confirmation to change the id3tag for each mp3."
echo "Hit enter now, or CTRL-C at any time to abort this " \
	 "shell-script program."
read foobar

#File format option-menu
echo 'Syntax choices:'
echo '1  %02d <Songname>.mp3 (songname only)'
echo '2  %02d <Artist> - <Songname>.mp3'
echo '3 <Artist> - <Songname>.mp3'
echo -n '1, 2 or 3? '
read choice

if [ "$choice" = "1" ] ; then
  for i in *.[mM][pP][23]
  do
    songname=`echo "$i"|cut -b 4-|sed -e s+'.mp3$'+''+`
	if [ "$1" != "-y" ]
	then 
      echo "Use songname \"$songname\" for $i?"
      read yesno
      if [ "$yesno" -a "`echo \"$yesno\" | cut -b -1`" = "y" ]
      then
	    $MP3TAG -f "$i" -rs "$songname"
	  else
	    echo "Ok, skipping this file."
      fi
	else
	  echo "Using songmae \"$songname\" for $i."
	  $MP3TAG -f "$i" -rs "$songname"
	fi
  done
elif [ "$choice" = "2" ] ; then
  for i in *.[mM][pP][23]
  do
    basename=`echo "$i"|cut -b 4-|sed -e s+'.mp3$'+''+`
    artist=`echo "$basename"|cut -d'-' -f1|sed -e s+' $'+''+`
    songname=`echo "$basename"|cut -d'-' -f2-|sed -e s+'^ '+''+`
	if [ "$1" != "-y" ]
	then
      echo "Use artist \"$artist\" & songname \"$songname\" for $i?"
      read yesno
      if [ "$yesno" -a "`echo \"$yesno\" | cut -b -1`" = "y" ]
      then
	    $MP3TAG -f "$i" -rs "$songname" -a "$artist"
	  else
	    echo "Ok, skipping this file."
	  fi
	else
      echo "Using artist \"$artist\" & songname \"$songname\" for $i."
      $MP3TAG -f "$i" -rs "$songname" -a "$artist"
    fi
  done
elif [ "$choice" = "3" ] ; then
  for i in *.[mM][pP][23]
  do
    basename=`echo "$i"|sed -e s+'.mp3$'+''+`
    artist=`echo "$basename"|cut -d'-' -f1|sed -e s+' $'+''+`
    songname=`echo "$basename"|cut -d'-' -f2-|sed -e s+'^ '+''+`
	if [ "$1" != "-y" ]
	then
      echo "Use artist \"$artist\" & songname \"$songname\" for $i?"
      read yesno
      if [ "$yesno" -a "`echo \"$yesno\" | cut -b -1`" = "y" ]
      then
	    $MP3TAG -f "$i" -rs "$songname" -a "$artist"
	  else
	    echo "Ok, skipping this file."
	  fi
	else
      echo "Using artist \"$artist\" & songname \"$songname\" for $i."
      $MP3TAG -f "$i" -rs "$songname" -a "$artist"
    fi
  done
else
  echo "No such option.."
fi
