#!/bin/sh

####################
# Main Script
####################

TERMINALS="x-terminal-emulator gnome-terminal bash sh"
for t in $TERMINALS; do
    if command -v "$t" >/dev/null 2>&1; then
        TERMNAME="$t"
        continue
    fi
done

if [ -z "$TERMNAME" ]; then
    echo "Can't found a terminal"
    exit 1
fi

$TERMNAME -- "$1" &
