#!/bin/sh

. $(dirname $0)/functions.sh
settitle

filezilla sftp://$username:"$password"@$server 

