#!/bin/sh

# User: Setup here the location of DosBox and DOS stuff in your system

DEFAULT_DIR=/usr/share/YaST2/clients/
Y2BASE_BINARY=/usr/lib/YaST2/bin/y2base

# Ask user for a module
# (Even after running it, keep asking, until Cancel button or Esc key are pressed)

while [ 1 ]
	do

MODULE=`kdialog --title "Choose a Module - YCC" \
         --getopenfilename $DEFAULT_DIR "*.ycp | Yast modules"`

if [ "$MODULE" ]; then
  $Y2BASE_BINARY "$MODULE" qt &
  $Y2BASE_BINARY "$MODULE" gtk
else
	exit 0;
fi

done
