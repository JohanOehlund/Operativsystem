#!/bin/bash
# change directory to the location of this script
cd "$(dirname "$0")"
for i in {1..5}
	do
		dd if=/dev/zero of=test  bs=100K count=1000 &
	done



