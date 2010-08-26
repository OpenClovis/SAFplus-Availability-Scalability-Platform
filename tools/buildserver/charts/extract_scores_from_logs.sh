#!/bin/bash

for log in `find /opt/c3po/build/clasp/clasp-root -maxdepth 2 -name log`; do
	stamp=`dirname $log`
	stamp=`basename $stamp`
#	rev=`awk '/Build number \(rev\):/{print $4}' $log`
#	if [ -z "$rev" ]; then
#		continue
#	fi
#	echo -n "$stamp $rev "
	echo -n "$stamp "
	cat $log | ./get_score_from_log.py | tail -n 1
done

