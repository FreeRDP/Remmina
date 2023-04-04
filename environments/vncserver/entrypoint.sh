#!/bin/bash

echo "remmina" >> ~/.vnc/passwd

# Start the vnc server while only accepting connections through the unix-socket.
# This will prompt the user for a password
vncserver -rfbport=5555 -localhost no -Log *:stderr:100 -xstartup /usr/bin/xterm
tail -f ~/.vnc/*.log
