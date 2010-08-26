#!/bin/bash

# This is supposed to be run once every night, after midnight
# It assumes that it is run from the directory where the program
# resides (the dashboard/bugzilla dir)
set -x

scp=scp

# Run daily.py to get lates daily data from bugzilla

month=`date +%b | awk '{print tolower($1)}'`
dat_file="history_$month.dat"
touch $dat_file

./daily.py > today.dat

# Sanity checking: verify that tmp.dat contains real data
# Check if each line has 4 columns
if ! wc -wl today.dat | awk '{if ($1==0 || $1*4!=$2) exit(1)}'; then
    echo "Could not extract daily snapshot from bugzilla on `date`" \
        | tee /tmp/dashboard.log
    exit 1
fi

# Before adding content, remove any previous one with same date
today=`head -n 1 today.dat | awk '{print $1}'`
grep -v $today $dat_file > tmp.dat
mv tmp.dat $dat_file && cat today.dat >> $dat_file

# Generate/update both png and pdf files over all data available

./make_chart.py --format=pdf --output=bug_snapshot.pdf history_*.dat
./make_chart.py --format=png --output=bug_snapshot.png history_*.dat

# Upload new snapshot files to the wiki server (this is a bit klugy as we
# actually done this by hand first, then checked where Wiki stores the
# actual file, and now we just copy it there...)

wiki_server="192.168.0.94"
png_image_path="/var/www/intranet/wiki/images/4/47/Bug_snapshot.png"
thumb_image_path="/var/www/intranet/wiki/images/thumb/4/47/Bug_snapshot.png/."
pdf_image_path="/var/www/intranet/wiki/images/a/a3/Bug_snapshot.pdf"

$scp bug_snapshot.png root@$wiki_server:$png_image_path
$scp bug_snapshot.pdf root@$wiki_server:$pdf_image_path

# Create thumb nail photo
convert bug_snapshot.png -resize 200 -quality 100 200px-Bug_snapshot.png
convert bug_snapshot.png -resize 320 -quality 100 320px-Bug_snapshot.png
$scp *px-Bug_snapshot.png root@$wiki_server:$thumb_image_path
