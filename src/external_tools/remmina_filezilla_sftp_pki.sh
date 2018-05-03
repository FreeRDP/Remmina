#!/bin/sh

. $(dirname $0)/functions.sh
settitle

filezilla sftp://$ssh_username@$server 

