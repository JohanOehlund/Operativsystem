#!/bin/bash

for i in {1..5}
	do
		dd if=/dev/zero of=/home/c15aen/Operativsystem/Operativsystem/OU1/test bs=1G count=1 iflag=dsync iflag=nocache oflag=dsync oflag=nocache &
	done

