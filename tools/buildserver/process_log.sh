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

in=$EMAIL_LOG
out=log.processed

# Grab original header (3 lines)
head -n 3 $in > $out

# Append summary
$autobuild_dir/charts/get_score_from_log.py $in > tmp.log
head -n -1 tmp.log >> $out
echo >> $out

# Add back original log content (from line 4 onward)
tail -n +4 $in >> $out

# Also add to history and create history plot
tail -n 1 tmp.log >> $WORKDIR/../history/history.dat
$autobuild_dir/charts/plot_scores.py $WORKDIR/../history/history.dat > history.png

# Do some cleanup
rm -f tmp.log
