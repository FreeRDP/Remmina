#!/bin/sh

. $(dirname $0)/functions.sh
settitle

traceroute $server

pause
