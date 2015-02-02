#!/bin/sh

# Waits for user key press.
pause ()
{
 echo "Hit a key to continue ..."
 OLDCONFIG=`stty -g`
 stty -icanon -echo min 1 time 0
 dd count=1 2>/dev/null
 stty $OLDCONFIG
}

# set terminal title for gnome-terminal and many others
settitle() {
  echo -ne "\033]0;${remmina_term_title}\007"
}

