#!/bin/sh

####################
# Main Script
####################

TERMINALS="xfce4-terminal gnome-terminal x-terminal-emulator"

for t in $TERMINALS; do
  TERMBIN="$(command -v "$t")"
  if [ "$?" -eq 0 ]; then
    case "$t" in
      xfce4-terminal)
        TERMBIN="$TERMBIN --disable-server"
        break
        ;;
      gnome-terminal)
        break
        ;;
      x-terminal-emulator)
        break
        ;;
    esac
  fi
done

$TERMBIN -e "$1" &

