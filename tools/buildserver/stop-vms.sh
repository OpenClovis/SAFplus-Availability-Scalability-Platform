#!/bin/bash

# Requirements to run this script
# - run as root
# What it does
# - it will kill all running vmware sessions without merci

# Step 1: Killing running vmware sessions

#sudo killall -9 vmware 2>/dev/null
#sleep 5
#find /opt/vmware -name "*LOCK" | xargs sudo rm -f

cd /opt/vmware
for n in node-?; do
	echo "Stopping [$n]..."
	sudo vmrun stop $n/*.vmx
	echo "Done"
done

