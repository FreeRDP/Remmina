#!/bin/bash

echo "remmina" >> ~/.vnc/passwd

# Start the vnc server while only accepting connections through the unix-socket.
# This will prompt the user for a password
vncserver -rfbport=-1 -rfbunixpath /vnc-server-tmp/vnc.socket -xstartup /usr/bin/xterm

cat ~/.ssh/authorized_keys

echo "SSH Server up!"
$(which sshd) -D
