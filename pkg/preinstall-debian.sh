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
echo -e "\nPrerequisite installation\n\nThis script installs required software using your distribution's package manager.\nIt will either need your CDROMs or access the packages over the internet, depending on your package manager's configuration.\n\nIt must be run as root.\n\n"

function install () {
echo "Installing $1 $2 $3 $4 $5"
apt-get -y install $1 $2 $3 $4 $5
return $?
}

function installRequired () {
echo "Installing $1 $2 $3 $4 $5"
apt-get -y install $1 $2 $3 $4 $5
if test $? != 0 ; then
echo "Installation of $1 $2 $3 $4 $5 failed.  You will have to install this yourself before continuing."
exit 1
fi
}

function installOneOf () {
echo "Installing $1"
apt-get -y install $1
if test $? != 0 ; then
  echo "Installation of $1 failed."
  apt-get -y install $2
  if test $? != 0 ; then
    echo "Installation of $3 failed."
    apt-get -y install $3
    if test $? != 0 ; then
      echo "Installation of $3 failed.  You will have to install one of $1 $2 or $3 yourself before continuing."
      exit 1
    fi
  fi
fi
}

installRequired build-essential
installRequired pkg-config
installRequired gettext
installRequired uuid-dev
installRequired unzip
installRequired gawk
installRequired libgdbm-dev
installRequired e2fsprogs
installRequired libperl-dev
installRequired libltdl3-dev
installRequired e2fslibs-dev

installOneOf libdb4.4-dev libdb4.3-dev libdb4.2-dev
installRequired kernel-headers-`uname -r`

TWOSIX=`uname -a | grep -c "2.6"`
if [ $TWOSIX == 0 ]; then
echo "Linux kernel version 2.6.x is required.  To get this version, use apt-get,"
echo "for example: apt-get install kernel-image-2.6.8-3-386 kernel-headers-2.6.8-386"
echo "for 386 compatible machines.  You can do 'apt-cache search kernel-image' to see"
echo "the list."
echo ""
echo "After installing the new kernel, you must REBOOT your system"
fi


echo -e "\nInstallation of prerequisite packages successful"
exit 0