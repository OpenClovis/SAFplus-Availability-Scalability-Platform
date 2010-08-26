#!/bin/bash

#
# prepping work area, allowing also hand-off from autobuild
#
if [ -z "$WORKDIR" ]; then
	WORKDIR=/opt/c3po/run-tae;
	mkdir -p $WORKDIR 2> /dev/null
	# rm -fr $WORKDIR/* $WORKDIR/.??*
fi
if [ -z "$EMAIL_LOG" ]; then
	EMAIL_LOG=$WORKDIR/log
	touch $EMAIL_LOG
fi
if [ -z "$BINDIR" ]; then
	BINDIR=$PWD
fi

in=$EMAIL_LOG
out=$WORKDIR/email_body

# Grab original header (3 lines)
head -n 3 $in > $out

# Append summary
cmd="$BINDIR/charts/get_score_from_log.py $in"
echo "Running: $cmd"
$cmd > tmp.log
head -n -1 tmp.log >> $out
echo >> $out

# Add back original log content (from line 4 onward)
tail -n +4 $in >> $out

# Also add to history and create history plot
echo -n "`basename $WORKDIR` " >> $WORKDIR/../history/history.dat
tail -n 1 tmp.log >> $WORKDIR/../history/history.dat
cmd="$BINDIR/charts/plot_scores.py $WORKDIR/../history/history.dat"
echo "Running: $cmd"
$cmd > $WORKDIR/history.png

# Do some cleanup
rm -f tmp.log
