#!/bin/bash

for i in {1..5}
	do
		dd if=/dev/zero of=/home/c15aen/Operativsystem/Operativsystem/OU1/test oflag=nocache oflag=dsync bs=1M count=500 &
	done



