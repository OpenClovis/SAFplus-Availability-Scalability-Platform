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
# File        : package.sh
################################################################################
# Description :
## Package script for SAFplus
################################################################################
##############################################################
## Initializing Env variables and creating Temp Directories ##
##############################################################

ASP_VER=2.3
PACKAGE_NAME=SAFplus-$ASP_VER-$CL_TARGET_PLATFORM-$CL_TARGET_OS-RT

PACKAGE_DIR=$CLOVIS_ROOT/SAFplus/$PACKAGE_NAME
PACKAGE_LIB_DIR=$PACKAGE_DIR/lib
PACKAGE_BIN_DIR=$PACKAGE_DIR/bin
PACKAGE_MOD_DIR=$PACKAGE_DIR/modules
PACKAGE_CONFIG_DIR=$PACKAGE_DIR/config
PACKAGE_SCRIPTS_DIR=$PACKAGE_DIR/scripts

echo -n "Creating Temporary Directory For Package ..."
mkdir -p $PACKAGE_DIR
mkdir -p $PACKAGE_LIB_DIR
mkdir -p $PACKAGE_BIN_DIR
mkdir -p $PACKAGE_MOD_DIR
mkdir -p $PACKAGE_CONFIG_DIR
mkdir -p $PACKAGE_SCRIPTS_DIR
echo "Done"

##############################
## Copying shared libraries ##
##############################

echo -n "Copying Shared Libraries ..."
cp -pr $CLOVIS_ROOT/SAFplus/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib/shared-$ASP_VER $PACKAGE_LIB_DIR/
cp -pr $CLOVIS_ROOT/SAFplus/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib/*.so $PACKAGE_LIB_DIR
echo "Done"

######################
## Copying binaries ##
######################

echo -n "Copying Executables ..."
cp -pr $MODEL_BIN/* $PACKAGE_BIN_DIR
echo "Done"

############################
## Copying kernel modules ##
############################

echo -n "Copying Kernel Modules ..."
cp -pr $CLOVIS_ROOT/SAFplus/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/kmod/* $PACKAGE_MOD_DIR
echo "Done"

#####################
## Copying scripts ##
#####################

echo -n "Copying Install/Uninstall Scripts ..."
cp -pr $CLOVIS_ROOT/SAFplus/build/rt/scripts/uninstall.sh $PACKAGE_SCRIPTS_DIR
cp -pr $CLOVIS_ROOT/SAFplus/build/rt/scripts/asp $PACKAGE_SCRIPTS_DIR
#cp -pr $CLOVIS_ROOT/SAFplus/build/asp-debug $PACKAGE_SCRIPTS_DIR
cp -pr $CLOVIS_ROOT/SAFplus/build/rt/scripts/install.sh $PACKAGE_DIR
cp -pr $CLOVIS_ROOT/SAFplus/scripts/clIocLoadModule.sh $PACKAGE_SCRIPTS_DIR
cp -pr $CLOVIS_ROOT/SAFplus/scripts/clIocUnloadModule.sh $PACKAGE_SCRIPTS_DIR
#cp -pr $CLOVIS_ROOT/SAFplus/components/debug/script/clDebugStart.sh $PACKAGE_SCRIPTS_DIR
echo "Done"

#################################
## Copying configuration files ##
#################################

echo -n "Copying Configuration Files ..."
cp -pr $MODEL_PATH/config/*.xml $PACKAGE_CONFIG_DIR
cp -pr $MODEL_PATH/config/*.conf $PACKAGE_CONFIG_DIR
cp -pr $CLOVIS_ROOT/SAFplus/build/rt/conf/* $PACKAGE_CONFIG_DIR

#Replace CL_TARGET_PLATFORM and CL_TARGET_OS in install.sh and uninstall.sh
sed -e s/CL_TARGET_PLATFORM=ia32/CL_TARGET_PLATFORM=$CL_TARGET_PLATFORM/g -e s/CL_TARGET_OS=linux-2.4/CL_TARGET_OS=$CL_TARGET_OS/g $PACKAGE_DIR/install.sh >>$PACKAGE_DIR/install.sh.bak
mv -f $PACKAGE_DIR/install.sh.bak $PACKAGE_DIR/install.sh

sed -e s/CL_TARGET_PLATFORM=ia32/CL_TARGET_PLATFORM=$CL_TARGET_PLATFORM/g -e s/CL_TARGET_OS=linux-2.4/CL_TARGET_OS=$CL_TARGET_OS/g $PACKAGE_SCRIPTS_DIR/uninstall.sh >> $PACKAGE_SCRIPTS_DIR/uninstall.sh.bak
mv -f $PACKAGE_SCRIPTS_DIR/uninstall.sh.bak $PACKAGE_SCRIPTS_DIR/uninstall.sh

chmod +x $PACKAGE_DIR/install.sh
chmod +x $PACKAGE_SCRIPTS_DIR/uninstall.sh
   
echo "Done"

#####################################
## Creating RT package and cleanup ##
#####################################

echo -n "Creating Tarball ..."
cp -f $CLOVIS_ROOT/SAFplus/build/rt/README $PACKAGE_NAME
tar -czvf $PACKAGE_NAME.tgz $PACKAGE_NAME 2>&1 >/dev/null
echo "Done"

echo -n "Removing Temporary Directory ..."
rm -fr $PACKAGE_DIR
echo "Done"
