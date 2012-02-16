#!/bin/bash
# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.

export CCOUNT=$CLOVIS_ROOT/ASP/mk/ccount.awk

echo "File statistics"
echo "==============="
echo "Total # of source files:        `find . -name '*.[hc]' | wc -l`"
echo "   Header files:                `find . -name '*.h' | wc -l`"
echo "      XDR headers:              `find . -name 'xdr*.h' | wc -l`"
echo "   C source files:              `find . -name '*.c' | wc -l`"
echo "      XDR C source files:       `find . -name 'xdr*.c' | wc -l`"

echo
echo "Line number statistics"
echo "======================"
echo "Total # of raw source lines:    \
`find . -name '*.[ch]' |xargs wc -l|tail -n 1|awk '{print $1}'` (\
`find . -name '*.[ch]' |xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"
echo "   Total # of raw header lines: \
`find . -name '*.h' |xargs wc -l|tail -n 1|awk '{print $1}'` (\
`find . -name '*.h' |xargs $CCOUNT | tail -n 1|awk '{print $4}'`)"
echo "      XDR raw header lines:     \
`find . -name 'xdr*.h' |xargs wc -l|tail -n 1|awk '{print $1}'` (\
`find . -name 'xdr*.h' |xargs $CCOUNT |tail -n 1|awk '{print $4}'`)"
echo "   Total # of raw C lines:      \
`find . -name '*.c' |xargs wc -l|tail -n 1|awk '{print $1}'` (\
`find . -name '*.c' |xargs $CCOUNT | tail -n 1|awk '{print $4}'`)"
echo "      XDR raw C lines:          \
`find . -name 'xdr*.c' |xargs wc -l|tail -n 1|awk '{print $1}'` (\
`find . -name 'xdr*.c' |xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"

echo
echo "Module break-down"
echo "================="
printf "%30s%33s\n" "Number of files" "Number of lines"
printf "%17s%14s%16s%19s\n" ".h" ".c" ".h" ".c"
printf "%8s %6s %-6s %6s %-6s %8s %-10s %8s %-10s\n" module all '(xdr)' all '(xdr)' all '(xdr)' all '(xdr)'
echo  "----------------------------------------------------------------------------"
for dir in `find . -maxdepth 1 -mindepth 1 -type d`
do
    printf "%8s" `basename $dir`;
    printf " %6s" `find $dir -name '*.h' | wc -l`
    printf " %-6s" "(`find $dir -name 'xdr*.h' | wc -l`)"
    printf " %6s" `find $dir -name '*.c' | wc -l`
    printf " %-6s" "(`find $dir -name 'xdr*.c' | wc -l`)"

    printf " %8s" `find $dir -name '*.h' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`
    printf " %-10s" "(`find $dir -name 'xdr*.h' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"
    printf " %8s" `find $dir -name '*.c' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`
    printf " %-10s" "(`find $dir -name 'xdr*.c' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"
    printf "\n"
done
echo  "----------------------------------------------------------------------------"
printf "%8s" total
printf " %6s" `find . -name '*.h' | wc -l`
printf " %-6s" "(`find . -name 'xdr*.h' | wc -l`)"
printf " %6s" `find . -name '*.c' | wc -l`
printf " %-6s" "(`find . -name 'xdr*.c' | wc -l`)"

printf " %8s" `find . -name '*.h' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`
printf " %-10s" "(`find . -name 'xdr*.h' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"
printf " %8s" `find . -name '*.c' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`
printf " %-10s" "(`find . -name 'xdr*.c' | xargs $CCOUNT | tail -n 1 | awk '{print $4}'`)"
printf "\n"
