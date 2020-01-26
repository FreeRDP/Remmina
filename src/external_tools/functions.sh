#!/bin/sh

# Awaits user key press.
pause ()
{
 echo "Press any key to continue…"
 OLDCONFIG=$(stty -g)
 stty -icanon -echo min 1 time 0
 dd count=1 2>/dev/null
 stty $OLDCONFIG
}

# Set terminal title for gnome-terminal and many others
settitle() {
  echo -n -e "\033]0;${remmina_term_title}\007"
}
