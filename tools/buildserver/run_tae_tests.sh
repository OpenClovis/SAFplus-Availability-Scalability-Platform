#!/bin/bash

autobuild_dir=$PWD

#
# prepping work area, allowing also hand-off from autobuild
#
if [ -z "$WORKDIR" ]; then
	WORKDIR=/opt/c3po/run-tae;
	mkdir -p $WORKDIR 2> /dev/null
	# rm -fr $WORKDIR/* $WORKDIR/.??*
fi
if [ -z "$DETAILED_LOG" ]; then
	DETAILED_LOG=$WORKDIR/detailed_log
	touch $DETAILED_LOG
fi
if [ -z "$EMAIL_LOG" ]; then
	EMAIL_LOG=$WORKDIR/log
	touch $EMAIL_LOG
fi

#
# Setting up some shorthands and printing the workarea info
#
wd=$WORKDIR
log=$EMAIL_LOG
dlog=$DETAILED_LOG
rlog=$wd/run_tae_tests.log
echo "WORKDIR:      $wd"
echo "EMAIL_LOG:    $log"
echo "DETAILED_LOG: $dlog"

myexit () {
	echo $1 | tee -a $rlog
	# copying script's own log to $log before exiting, so that parent autobuild can
	# report on the problem
	cat $rlog >> $log
	exit 1
}

#
# Powering up VMWARE nodes for the tests
#
#snapshot=no-ltt-kernel
snapshot=tipc
# args 1: vmdir name
#      2: IP address
vmstart () {
	if ! $autobuild_dir/start-vm.sh $1 $snapshot $2; then
		echo "Sometimes the vm network connection does not come up..." | tee -a $rlog
		echo "We give $1 another chance" | tee -a $rlog
		if ! $autobuild_dir/start-vm.sh $1 $snapshot $2 > tmp.log; then
			cat tmp.log | tee -a $rlog
			myexit "Could not start $1; exiting"
		fi
		cat tmp.log | tee -a $rlog
	fi
}

restart_vms () {
	$autobuild_dir/stop-vms.sh
	sleep 3
	vmstart node-1 172.16.253.101
	vmstart node-2 172.16.253.102
	vmstart node-3 172.16.253.103
	vmstart node-4 172.16.253.104
}

#
# Launch TAE on unittest model and test cases
#
tae=$HOME/tae/tae
cfdir=$HOME/autobuild

# args 1: name of test model, which will be used to look up config files and for
#         dirnames, but not for the actual ASP model name. The latter is
#         specified in the model ocnfig file.
run_tae_with_model () {
	#
	# Running TAE
	#
	echo "Running TAE tests for [${1}]:" | tee -a $rlog
	echo "============================================" | tee -a $rlog
	echo "(if what you see below is not a list of testcases but a Python backtrace," | tee -a $rlog
	echo "it means that TAE could not even get to the stage of running the tests...)" | tee -a $rlog
	mkdir -p $wd/${1} 2>/dev/null
	cd $wd/${1}
	cmd="$tae -F $cfdir/${1}-fixture.cfg -E $cfdir/env.cfg -M $cfdir/${1}-model.cfg -P $cfdir/${1}-mapping.cfg -T $cfdir/${1}-tests-to-run -qqqqqqqqqq -c -r -s -w 100"
	echo "Running command in dir [$wd]: [$cmd]" | tee -a $rlog

	set -o pipefail
	$cmd 2>&1 | tee results.log
	res=$?

	#
	# Post-process results
	#
	echo >> $log
	echo >> $log
	echo >> $log
	echo >> $log
	echo "TAE test results for [${1}] test model on build [$BUILD_NUMBER] (see details in attachment):" >> $log
	if [ $res != 0 ]; then
		# The test run failed, lets make it more obvious in the $log
		echo "====================================================================" >> $log
		echo "TAE run for [${1}] failed! See error message/backtrace below" >> $log
		echo "====================================================================" >> $log
	fi
	cat results.log >> $log
	cat log/tae.log >> $dlog
	cp log/tae.log $wd/${1}-tae.log
	logtar=tae-all-logs-${1}.tar.bz
	tar cjf $wd/$logtar log
	echo >> $log
	echo "All TAE-captured log files for this run are available at:" >> $log
	echo "$wd/$logtar" >> $log
	echo >> $dlog
	echo "All TAE-captured log files for this run are available at:" >> $dlog
	echo "$wd/$logtar" >> $dlog
	return $res
}

if [ "$1" == "" ]; then
	models="bft-vm unittests-vm stresstest-vm mgmcorfunctest-vm mgmend2endcfg-vm td-vm"
	models="$models bft-hp unittests-hp stresstest-hp"
else
	models="$@"
fi

global_res=0
echo "Running TAE for [$models] ..."
for model in $models; do
	if echo $model | grep -e -vm; then
		echo "Restarting VMs..." | tee -a $rlog
		restart_vms
	fi

	run_tae_with_model $model
	res=$?
	if [ $res != 0 ]; then
		global_res=$res
	fi
done

$autobuild_dir/stop-vms.sh

exit $global_res
