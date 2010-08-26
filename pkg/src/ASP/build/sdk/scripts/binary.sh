#!/bin/sh
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
# ModuleName  : sdk
# File        : binary.sh
################################################################################
# Description :
## Binary Deploy script												
################################################################################
##################################################
## Initializing Log files and related variables ##
##################################################

source $CLOVIS_ROOT/ASP/clasp.env

PKG_LOG="`pwd`/pkg_log"

if [ -f $PKG_LOG ]
then
    rm -f $PKG_LOG
fi

#####################
## Input from User ##
#####################
echo -n "Enter Linux Distribution[RH9/RHEL/MVL]: "
read LINUX_DIST

if [ -z "$LINUX_DIST" ]
then
	echo "Linux Distribution is RH9"
	LINUX_DIST="RH9"
fi

echo

############################################
## Initializing variables and directories ##
############################################


echo -n "Initializing directory structure: "

BUILD_DIR=$CLOVIS_ROOT/ASP/build/binary
ASP_SRC_DIR=$CLOVIS_ROOT/ASP
TARGET_LOC=$CLOVIS_ROOT/ASP
DISK_TOP=$CLOVIS_ROOT/ASP


DOC_DIR=$DISK_TOP/ASP/doc
INCLUDE_DIR=$DISK_TOP/ASP/include
IPI_INCLUDE_DIR=$DISK_TOP/ASP/include/ipi
SERVERS_DIR=$DISK_TOP/ASP/servers
NEW_BUILD_DIR=$DISK_TOP/ASP/build
SCRIPT_DIR=$DISK_TOP/ASP/scripts
MK_DIR=$DISK_TOP/ASP/mk
MODEL_DIR=$DISK_TOP/ASP/models
TARGET_DIR=$DISK_TOP/ASP/target
MIB_DIR=$DISK_TOP/ASP/mib
TEST_DIR=$DISK_TOP/ASP/sample-tests
TEMPLATE_DIR=$DISK_TOP/ASP/templates

LIB_DIR=$DISK_TOP/ASP/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
KMOD_DIR=$DISK_TOP/ASP/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/kmod
BIN_DIR=$DISK_TOP/ASP/models/$ASP_MODEL_NAME/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/bin

rm -rf $DISK_TOP/ASP

mkdir -p $INCLUDE_DIR
mkdir -p $IPI_INCLUDE_DIR
mkdir -p $NEW_BUILD_DIR
mkdir -p $SERVERS_DIR
mkdir -p $SCRIPT_DIR 
mkdir -p $MK_DIR 
mkdir -p $MODEL_DIR
mkdir -p $TARGET_DIR
mkdir -p $MIB_DIR
mkdir -p $TEST_DIR
mkdir -p $TEMPLATE_DIR

mkdir -p $LIB_DIR
mkdir -p $KMOD_DIR
mkdir -p $BIN_DIR

mkdir -p $DISK_TOP/ASP/templates/hal
mkdir -p $DISK_TOP/ASP/templates/snmp

echo "Done"

#######################
## Copying Makefiles ##
#######################

echo -n "Copying Makefiles: "

cp -p $BUILD_DIR/mk/servers/Makefile $SERVERS_DIR/
cp -p $CLOVIS_ROOT/ASP/mk/* $MK_DIR
cp -f $CLOVIS_ROOT/ASP/build/binary/mk/make-common.mk $MK_DIR/

echo "Done"

##########################
## Copying Customer Docs #
##########################

################################
## Copying Build related files #
################################

echo -n "Copying Build related files: "
cp -pr  $CLOVIS_ROOT/ASP/build/common $NEW_BUILD_DIR
cp -pr  $CLOVIS_ROOT/ASP/build/binary $NEW_BUILD_DIR
cp -pr  $CLOVIS_ROOT/ASP/build/rt $NEW_BUILD_DIR
cd $NEW_BUILD_DIR/binary/autoconf
mkdir temp
cp configure.ac temp
cp acinclude.m4 temp
cd temp
aclocal
autoconf
cp configure $DISK_TOP/ASP
cd -
rm -fr temp
cd $CLOVIS_ROOT/ASP
echo "Done"

###########################
## Copying Header Files  ##
###########################

echo -n "Copying Header Files: "

cd $CLOVIS_ROOT/ASP/
for d in `find components -type d -name include`; do
	echo "In $d:" >> $PKG_LOG
	for f in `find $d -name "*.h"`; do
        echo "..........$f" >> $PKG_LOG
		cp -p $f $INCLUDE_DIR
	done
done
cp $CLOVIS_ROOT/ASP/3rdparty/ezxml/stable/ezxml.h $INCLUDE_DIR

# FIXME: Check if this is OK
cp -p $CLOVIS_ROOT/ASP/components/include/ipi/clAlarmIpi.h $INCLUDE_DIR/ipi/
cp -p $CLOVIS_ROOT/ASP/components/include/ipi/clSAClientSelect.h $INCLUDE_DIR/ipi/
cp -p $CLOVIS_ROOT/ASP/components/cor/common/clCorTxnJobStream.h $INCLUDE_DIR
cp -p $CLOVIS_ROOT/ASP/components/cor/common/clCorTxnClientIpi.h $INCLUDE_DIR
cp -p $CLOVIS_ROOT/ASP/components/amf/common/ams/parser/clAmsParser.h $INCLUDE_DIR
cp -p $CLOVIS_ROOT/ASP/components/amf/common/ams/clAmsSAClientApi.h $INCLUDE_DIR
cp -p $CLOVIS_ROOT/ASP/components/amf/common/ams/clAmsSAClientApi.h $INCLUDE_DIR
cp -p $CLOVIS_ROOT/ASP/components/amf/common/ams/clAmsMgmtCommon.h $INCLUDE_DIR

echo "Done"

#####################
## Copying scripts ##
#####################

echo -n "Copying Scripts: "
cd $CLOVIS_ROOT/ASP/
for d in `find components -type d -name script`; do
	echo "In $d:" >> $PKG_LOG
	for f in `find $d -name "*" -type f`; do
        echo "..........$f" >> $PKG_LOG
		cp -p $f $SCRIPT_DIR 
	done
done
echo "Done"

cp -p $CLOVIS_ROOT/ASP/components/snmp/mib/* $DISK_TOP/ASP/mib/

#######################
## Copying Libraries ##
#######################

echo -n "Copying Libraries: "

cp -pr $CLOVIS_ROOT/ASP/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib/*  $LIB_DIR
cd $CLOVIS_ROOT/ASP/3rdparty
for f in `find . -name "*.a"`; do
    echo "..........$f" >> $PKG_LOG
	cp -p $f $LIB_DIR
done

echo "Done"

echo -n "Copying Binaries: "

cp -pr $ASP_BIN/* $BIN_DIR

echo "Done"

cp $CLOVIS_ROOT/ASP/clasp.env $DISK_TOP/ASP

############################
## Copying Kernel Modules ##
############################

echo -n "Copying kmod files: " 

cd $CLOVIS_ROOT/ASP/
cp -p $CLOVIS_ROOT/ASP/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/kmod/* $KMOD_DIR

echo "Done"

##########################
## Copying Sample Tests ##
##########################

echo -n "Copying sample tests: "

cd $CLOVIS_ROOT/ASP/sample-tests
cp -pr test $DISK_TOP/ASP/sample-tests
rm -rf `find $DISK_TOP/ASP/sample-tests -type d | grep "\<obj\|dep\>"`

echo "Done"

###########################
## Copying Sample Models ##
###########################

echo -n "Copying model: "
cp -r $CLOVIS_ROOT/ASP/models/$ASP_MODEL_NAME $DISK_TOP/ASP/models
cp -r $CLOVIS_ROOT/ASP/models/static $DISK_TOP/ASP/models/
rm -rf `find $DISK_TOP/ASP/models/$ASP_MODEL_NAME -type d | grep "\<obj\|dep\>"`

echo "Done"

#######################
## Copying Templates ##
#######################

echo -n "Copying templates: "
cp -pr $CLOVIS_ROOT/ASP/templates/hal $DISK_TOP/ASP/templates
cp -pr $CLOVIS_ROOT/ASP/templates/snmp $DISK_TOP/ASP/templates
cp -pr $CLOVIS_ROOT/ASP/templates/*.c $DISK_TOP/ASP/templates
cp -pr $CLOVIS_ROOT/ASP/templates/*.h $DISK_TOP/ASP/templates
echo "Done"

#############################
## Creating Binary package ##
#############################

echo -n "Creating binary package: "
cd $TARGET_LOC
cp $CLOVIS_ROOT/ASP/build/binary/README $DISK_TOP/ASP
tar cvzf ASP-$ASP_VERSION-$LINUX_DIST-$CL_TARGET_PLATFORM-$CL_TARGET_OS-bin.tgz ASP >> $PKG_LOG
rm -rf ASP
echo "Done"

