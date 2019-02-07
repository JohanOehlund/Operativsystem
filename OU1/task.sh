#!/bin/bash
# change directory to the location of this script
cd "$(dirname "$0")"
echo $0
for i in {1..5}
	do
		dd if=/dev/zero of=test oflag=nocache,dsync bs=1M count=1 &
	done



