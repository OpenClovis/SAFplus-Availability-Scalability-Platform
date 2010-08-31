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

if [ `whoami` != "root" ] ; then
/bin/echo -e "\nYou must be root to run this installer."
exit 2
fi

function install () {
echo "Installing $1 $2 $3 $4 $5"
apt-get -y --force-yes install $1 $2 $3 $4 $5
return $?
}

function installRequired () {
echo "Installing $1 $2 $3 $4 $5"
apt-get -y --force-yes install $1 $2 $3 $4 $5
if test $? != 0 ; then
echo "Installation of $1 $2 $3 $4 $5 failed.  You will have to install this yourself before continuing."
exit 1
fi
}

function installOneOf () {
echo "Installing $1"
apt-get -y --force-yes install $1
if test $? != 0 ; then
  echo "Installation of $1 failed, trying $2 instead..."
  apt-get -y --force-yes install $2
  if test $? != 0 ; then
    echo "Installation of $2 failed, trying $3 instead..."
    apt-get -y --force-yes install $3
    if test $? != 0 ; then
        echo "Installation of $3 failed, trying $4 instead..."
        apt-get -y --force-yes install $4
        if test $? != 0 ; then
            echo "Installation of $1 failed.  You will have to install one of $1 $2 $3 or $4 yourself before continuing."
            exit 1
        fi
    fi
  fi
fi
}

# Make sure the package list and server IPs are up-do-date
apt-get update

# Start installing packages
installRequired build-essential
installRequired linux-headers-`uname -r`
#installRequired pkg-config
installRequired gettext
installRequired uuid-dev
#installRequired unzip
installRequired bison
installRequired flex
installRequired gawk
installRequired pkg-config
installRequired libglib2.0-dev
installRequired libgdbm-dev
installOneOf libdb4.6-dev libdb4.5-dev libdb4.4-dev libdb4.3-dev
installRequired libsqlite3-0 libsqlite3-dev
installRequired e2fsprogs
installRequired libperl-dev
installRequired libltdl3-dev
installRequired e2fslibs-dev

if [ $(uname -m) ==  "x86_64" ]; then
    installRequired ia32-libs
fi

echo -e "\nInstallation of prerequisite packages successful"
exit 0
