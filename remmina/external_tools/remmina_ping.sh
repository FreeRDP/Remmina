#!/bin/sh

. $(dirname $0)/pause.sh

ping -c3 $server

pause
