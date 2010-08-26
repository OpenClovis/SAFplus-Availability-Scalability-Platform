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

##
# displays usage
##
usage() {
    echo ""
    echo "preinstall-rhel4.sh - This script installs third-party packages required"
    echo "      to install the OpenClovis SDK from your installation media as well"
    echo "      as third-party sources."
    echo "Usage:"
    echo "      $(basename $0) [-m <installation-media-path>]"
    echo ""
}

##
# check if we're root
##
if [ "$(whoami)" != "root" ]; then
    echo ""
    echo "You must be root to run this installer."
    exit 2
fi

##
# set up variables
##
AUTODETECT_MEDIA=1
THIRDPARTY_TARBALL=3rdparty-base-1.14.tar
INSTALL_BEECRYPT=0
REMOVE_NETSNMP=0
INSTALL_NETSNMP=0
INSTALL_SQLITE=0
WORKING_DIR=$(pwd)

##
# parse options
##
while getopts :m:h options; do
    case $options in
        h)
            usage
            exit 0
            ;;
        m)
            AUTODETECT_MEDIA=0
            INSTALL_MEDIA=$OPTARG
            ;;
        ?)
            echo ""
            echo "Invalid option"
            usage
            exit 1
            ;;
    esac
done

##
# check for 3rdparty-base tarball
##
if [ -f ../$THIRDPARTY_TARBALL ]; then
    echo "3rd-party package tarball found."
    echo ""
else
    echo ""
    echo "3rd-party package tarball not found in parent directory.  Please ensure that"
    echo "$THIRDPARTY_TARBALL is located adjacent to this installation directory and"
    echo "try again."
    echo ""
    exit 1
fi

##
# create build directory if necessary
##
if [ -d build ]; then
    echo -n ""
else
    mkdir -p build
fi

##
# beecrypt-devel installation from RHEL4 media if necessary
##
rpm -q beecrypt-devel > /dev/null 2>&1
if [ $? -eq 1 ]; then
    if [ $AUTODETECT_MEDIA -eq 1 ]; then
        mounts=$(mount | grep '9660' | wc -l)
        if [ "$mounts" == "0" ]; then
            echo ""
            echo "No mounted media detected.  Please specify the path to the Red Hat EL4"
            echo "installation media using the -m option to this script and try again."
            echo ""
            cd $WORKING_DIR
            exit 1
        fi
        if [ $mounts -gt 1 ]; then
            echo ""
            echo "More than one ISO9660 filesystem is mounted.  Please specify the path to"
            echo "the Red Hat EL4 installation media using the -m option to this script and"
            echo "try again"
            echo ""
            cd $WORKING_DIR
            exit 1
        fi
        INSTALL_MEDIA=$(mount | grep '9660' | cut -d\  -f 3)
        echo ""
        echo "Mounted ISO9660 filesystem detected at $INSTALL_MEDIA."
    fi

    if [ -d $INSTALL_MEDIA/RedHat/RPMS ]; then
        echo -n ""
    else
        echo ""
        echo "No Red Hat EL4 installation found at $INSTALL_MEDIA"
        echo "Please specify the path to the Red Hat EL4 installation media using the -m"
        echo "option to this script and try again."
        echo ""
        cd $WORKING_DIR
        exit 1
    fi

    echo -n "Installing beecrypt-devel from installation media ... "
    rpm -iUvh $INSTALL_MEDIA/RedHat/RPMS/beecrypt-devel*.rpm > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "done."
    else
        echo "failed."
        echo ""
        echo "ERROR: Unable to install beecrypt-devel.  Please install beecrypt-devel from"
        echo "       the Red Hat EL4 installation media and run this script again."
        echo ""
        cd $WORKING_DIR
        exit 1
    fi
else
    echo ""
    echo "beecrypt-devel is present."
fi

##
# net-snmp installation from source if necessary
##
if [ "$(which net-snmp-config | wc -l)" == "0" ]; then
    INSTALL_NETSNMP=1
else
    netsnmp_maj_ver=$(net-snmp-config --version | cut -d. -f 1)
    netsnmp_min_ver=$(net-snmp-config --version | cut -d. -f 2)
    if [ $netsnmp_maj_ver -ge 5 ]; then
        if [ $netsnmp_min_ver -ge 4 ]; then
            INSTALL_NETSNMP=0
        else
            REMOVE_NETSNMP=1
            INSTALL_NETSNMP=1
        fi
    else
        REMOVE_NETSNMP=1
        INSTALL_NETSNMP=1
    fi
fi

if [ $REMOVE_NETSNMP -eq 1 ]; then
    echo ""
    echo -n "Removing old net-snmp installation ... "
    rpm -e net-snmp > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "done."
    else
        echo "failed."
        echo ""
        echo "ERROR: Unable to remove existing net-snmp installation.  Please remove"
        echo "       the existing version of net-snmp from your system using the"
        echo "       following command:"
        echo "           rpm -e net-snmp"
        echo "       and run this script again."
        echo ""
        cd $WORKING_DIR
        exit 1
    fi
fi

if [ $INSTALL_NETSNMP -eq 0 ]; then
    echo ""
    echo "net-snmp is present."
else
    echo ""
    echo "Installing net-snmp (5.4.2)"

    # extract
    cd build
    tar xmf ../../$THIRDPARTY_TARBALL net-snmp-5.4.2.tar.gz
    tar xzf net-snmp-5.4.2.tar.gz
    cd net-snmp-5.4.2

    # configure
    ./configure \
        --with-default-snmp-version="2" \
        --without-rpm \
        --with-defaults
    if [ $? -ne 0 ]; then
        echo ""
        echo "ERROR: configuring net-snmp failed.  Please install net-snmp manually and run"
        echo "       this script again.  The source tarball can be found at:"
        echo "       $WORKING_DIR/build/net-snmp-5.4.2.tar.gz"
        echo ""
        cd $WORKING_DIR
        exit 1
    fi

    # make
    make
    if [ $? -ne 0 ]; then
        echo ""
        echo "ERROR: building net-snmp failed.  Please install net-snmp manually and run"
        echo "       this script again.  The source tarball can be found at:"
        echo "       $WORKING_DIR/build/net-snmp-5.4.2.tar.gz"
        echo ""
        cd $WORKING_DIR
        exit 1
    fi

    # make install
    make install
    if [ $? -ne 0 ]; then
        echo ""
        echo "ERROR: installing net-snmp failed.  Please install net-snmp manually and run"
        echo "       this script again.  The source tarball can be found at:"
        echo "       $WORKING_DIR/build/net-snmp-5.4.2.tar.gz"
        echo ""
        cd $WORKING_DIR
        exit 1
    fi
    cd ..
    rm net-snmp-5.4.2.tar.gz
    rm -rf net-snmp-5.4.2
    cd $WORKING_DIR
    echo ""
    echo "net-snmp installed."
fi

##
# sqlite installation from source if necessary
##

echo ""
echo "Installation of prerequisite packages completed.  You can now install OpenClovis"
echo "SDK using the install script (./install.sh)."
echo ""

exit 0
