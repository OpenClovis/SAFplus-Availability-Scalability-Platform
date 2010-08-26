#!/bin/bash

# This should be called as:
# start-vm.sh <vmroot> <snapshotname> <ip-of-vm>
if [ "$3" == "" ]; then
    echo "Usage: start-vm.sh <vmdir> <snapshotname> <ip-address>"
    exit 1
fi

vmdir=$1
snapshotname=$2
ip=$3

export DISPLAY=:0.0

# Requirements to run this script
# - run as root
# - have X server running
# - have DISPLAY set
# - have access to open window in DISPLAY
# - IT ASSUMES THAT THE NEEDED VM IS NOT RUNNING YET!!!

# TODO test the above conditions here

# Side-effects of this script:
# - it will kill all running vmware sessions without merci

vmroot=/opt/vmware

my_exit () {
	echo ${1}
	echo "exiting"
	exit 1
}

# args 1: vm dir name, e.g., 'node-1'
#      2: snapshotname
revert_vm () {
	cd $vmroot
	sudo vmrun revertToSnapshot ${1}/*.vmx ${2}
	if [ $? -ne 0 ]; then
		my_exit "Could not revert VM ${1} to snapshot ${2}"
	fi
	echo "VM ${1} is reverted to snapshot ${2}"
}

# args 1: IP address
#      2: amx waiting time
wait_till_accessible () {
	n=${2}
	while true; do
		echo "Waiting for ${1} for $n more seconds"
		ping -c 1 ${1} 2>/dev/null >/dev/null
		if [ $? == 0 ]; then
			return
		fi
		sleep 1
		n=$((n-1))
		if [ $n == 0 ]; then
			break
		fi
	done
	echo "Could not ping ${1} for ${2} seconds; exiting"
	exit 1
}

# args 1: vm dir name, e.g., 'node-1'
#      2: IP address it is expected to respond to
start_vm () {
	cd $vmroot
	ssh -l root localhost env DISPLAY=:0.0 nohup vmrun start $vmroot/${1}/*.vmx &
	wait_till_accessible ${2} 8
	echo "VM ${1} is up"
}

# Step 1: Killing running vmware sessions

cd $vmroot
# killall -9 vmware 2>/dev/null
# rm -f $vmdir/*.WRITELOCK

# Step 2: Resetting vmware to desried snapshot

revert_vm $vmdir $snapshotname

# Step 3: Starting vmwares up and verifying that they are alive

start_vm $vmdir $ip

