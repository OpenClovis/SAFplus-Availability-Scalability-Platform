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
# ModuleName  : common
# File        : uninstall.sh
################################################################################
# Description :
## Un-Install script for ASP SDK
################################################################################
######################################
## Initialize Environment variables ##
######################################

ASP_VER=2.3
source /etc/asp-sdk-$ASP_VER.conf
SDK_PREFIX=$PREFIX/clovis/

cd $CLOVIS_ROOT/..

################################
## Check for root permissions ##
################################

USER=`whoami`
if [ $USER != "root" ]
then
    echo "Please run Make with root privileges ... Exiting"
    exit
fi

#################################
## Remove Clovis SDK Directory ##
#################################

echo -n "Deleting $SDK_PREFIX : "
rm -rf $SDK_PREFIX/
echo "Done"

################################
## Removing asp-sdk-2.3.conf ##
################################

echo -n "Deleting asp_$ASP_VER.conf : "
rm -f /etc/asp-sdk-$ASP_VER.conf
echo "Done"

##############################
## Deleting asp-sdk-config ##
##############################

echo -n "Deleting Links : "
rm -rf /usr/bin/asp-sdk-config
echo "Done"
