#!/bin/sh
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
################################################################################
# ModuleName  : rt
# File        : uninstall.sh
################################################################################
# Description :
## Un-Install script for ASP RT
################################################################################
##############################
## Initialize env variables ##
##############################

# Note : CL_TARGET_OS and CL_TARGET_PLATFORM will be 
# set to the appropriate value by package.sh
CL_TARGET_PLATFORM=ia32
CL_TARGET_OS=linux-2.4

ASP_VER=2.3

source /etc/asp-rt-$ASP_VER.conf

PREFIX=$CLOVIS_ROOT
RT_PREFIX=$PREFIX/clovis
BOOT_DIR=/etc/rc.d/init.d
INIT_LINK=/etc/rc.d/rc5.d/S96asp
STOP_LINK=/etc/rc.d/rc3.d/K02asp

cd $CLOVIS_ROOT/..


########################################
## Checkk if user has root priveleges ##
########################################
USER=`whoami`
if [ $USER != "root" ]
then
    echo "Please run Make with root privileges ... Exiting"
    exit
fi

################################
## Shutting down existing ASP #
################################
/etc/init.d/asp stop

##########################################
## Remove Clovis Installation directory ##
## and /etc/asp-rt-2.2.conf file
echo -n "Deleting $RT_PREFIX/rt : "
rm -rf $RT_PREFIX/rt
echo "Done"

echo -n "Deleting asp_$ASP_VER.conf : "
rm -f /etc/asp-rt-$ASP_VER.conf
echo "Done"

##########################################
## Remove soft links and init.d scripts ##
##########################################

echo -n "Deleting soft-links : "
rm -f $INIT_LINK
rm -f $STOP_LINK
rm -f $BOOT_DIR/asp
echo "Done"

