#!/bin/sh

####################
# Main Script
####################
#gnome-terminal -e $(dirname $0)/$1 
gnome-terminal -e $1 &

#if [ "$2" = "1" ]
#then
#  echo "Hit a key to continue ..."
#  Pause
#fi
