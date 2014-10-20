#!/bin/sh

. $(dirname $0)/pause.sh

nslookup $server

pause
