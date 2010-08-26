#!/bin/bash
################################################################################
#
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
# 
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the 
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
################################################################################
#
# Build: 4.2.0
#
################################################################################
# ModuleName  : 
# File        : count-lines.sh
################################################################################
# Description :
################################################################################

#
# Prints the number of lines in .c and .h files in a directory
# hierarchy. Usage:
#
#    count-lines.sh <file or directory>
#
# If the argument is a file, then it simply runs 'wc -l' on the file.
#
# If the argument is a directory, then it runs through the entire
# directory hierarchy and prints the total number of lines in all the
# .c and .h files. It will print a summary for all the files and
# directories in the given directory and a total for the given
# directory.
# 
# If no argument is given, then the current directory is used.
#
# To get sorted list, do 'count-lines.sh | sort -n -k 2'
#
# Enjoy! Dont Panic!
#

# Parse command line argument, if any.
if [ "x$1" != "x" ]; then
    # Directory in the argument?
    if [ "x$1" != "x-h" ]; then
	if [ -d $1 ]; then
	    cd $1
	else
	    wc -l $1
	    exit
	fi
    else
	# usage
	name=`basename $0`
	echo
	echo "${name} [-h] [directory] [file]"
	echo
	echo "Script that recursively counts the number of lines of .c and .h"
	echo "files and presents a summary. It does not count files in"
	echo "directories named \"test\""
	echo
	echo "Options: "
	echo
	echo "    -h: Display this help"
	echo
	echo "    directory: If present, ${name} will move to that directory before executing"
	echo
	echo "    file: It will run 'wc -l' on the file"
	echo
	echo "If no options are given, then script will execute in the current directory"
	echo "To sort the results, run: ${name} | sort -n -k 2"
	exit
    fi
fi

# Now start processing.
total=0
for i in `ls`; do
    if [ -d $i ]; then
	if [ "x$i" != "xtest" ]; then
	    # If we are dealing with a directory, then run through the
	    # directory and compute total. we also skip any test
	    # directories underneath.
	    dirtotal=`find $i -name "*.[ch]" | grep -v "/test/" | \
		 xargs wc -l | tail -n 1 | awk '{print $1}'`
	else
	    # we skip test directories
	    continue;
	fi
    else
        # This is not a directory. Just calculate lines for this file,
        # if it is a .c or .h file and add to total. If it is not a .c
        # or .h file, continue the loop
        tmp_value=`find $i -name "*.[ch]"`
        if [ "x${tmp_value}" != "x" ]; then
            # this is a .c or .h file
            dirtotal=`wc -l $i | awk '{print $1}'`
        else
	    continue
        fi
    fi
    printf "%15s\t\t%d\n" $i ${dirtotal}
    # compute overall total 
    total=`expr ${total} + ${dirtotal}`
done

# Print overall total
printf "\t\tTotal: %d\n" ${total}
