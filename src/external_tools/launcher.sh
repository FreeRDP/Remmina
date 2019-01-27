#!/bin/sh

####################
# Main Script
####################
#gnome-terminal -e $(dirname $0)/$1

if [ -x "/usr/bin/x-terminal-emulator" ];
then
 TERMNAME="/usr/bin/x-terminal-emulator"
else
 TERMNAME="gnome-terminal"
fi
$TERMNAME -e "$1" &

#if [ "$2" = "1" ]
#then
#  echo "Hit a key to continue…"
#  Pause
#fi
