#!/bin/bash
###############################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
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
#
###############################################################################
# This script installs packages that you might not have previously installed
echo -e "\nPrerequisite installation\n\nThis script installs required software using your distribution's package manager.\nIt will either need your CDROMs or access the packages over the internet, depending on your package manager's configuration.\n\nIt must be run as root.\n\n"

# nice to have:
#yast -i subversion
#yast -i emacs

echo "Installing development packages. gcc gcc-c++ pkgconfig and e2fsprogs-devel"
yast -i gcc gcc-c++ pkgconfig e2fsprogs-devel 

VER=`grep -c "10.2" /etc/SuSE-release`
if [ $VER -gt 0 ]; then
echo "Installing libtool, needed for SUSE 10.2"
yast -i libltdl libtool
fi

# installed kernel-sources and kernel-docs
echo "Installing kernel sources. kernel-source"
yast -i kernel-source

# install berkeleyDB
echo "Installing BerkeleyDB. db db-devel db-utils"
yast -i db db-devel db-utils
#yast -i db42 db42-devel

# install gdbm
echo "Installing gdbm. gdbm"
yast -i gdbm gdbm-devel

echo -e "\nInstallation of prerequisite packages successful"
exit 0
