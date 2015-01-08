#!/bin/bash

#Define your log directory here - easiest to symlink to your wine DIR
LOGDIR=~/eqLogs

#Don't mess with this!
IFS=$'\n'

while :; do
	for f in `find ${LOGDIR}/eqlog* -newerct "1 minute ago"`; do
		echo "Working with: $f"
		curl -F "dump=$(tail -n100 $f|grep -E '[A-Za-z] auctions, ')" http://ahungry.com/aucDump.php
	done
	sleep 60
done
