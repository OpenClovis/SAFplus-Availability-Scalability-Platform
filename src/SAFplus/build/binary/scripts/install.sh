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
# ModuleName  : binary
# File        : install.sh
################################################################################
# Description :
## Binary SDK Install script for SAFplus
################################################################################


######################################
## Initialize Environment variables ##
## and check for root permissions   ##
######################################

ASP_VERSION=2.3
CONF=/etc/asp-sdk-$ASP_VERSION.conf

USER=`whoami`
if [ $USER != "root" ]
then
    echo "Please run the install script with root privileges ... Exiting"
    exit
fi


## directory for generated files
SDK_PREFIX=$PREFIX/clovis/sdk/$ASP_VERSION
SDK_ASP=$SDK_PREFIX/SAFplus
SDK_CW=$SDK_PREFIX/cw

SDK_INCLUDE=$SDK_ASP/include
SDK_LIB=$SDK_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
SDK_BIN=$SDK_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/bin
SDK_MOD=$SDK_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/kmod
SDK_TEMPLATES=$SDK_ASP/templates
SDK_RT=$SDK_ASP/rt
SDK_SAMPLES=$SDK_ASP/samples
SDK_TEST=$SDK_ASP/test
SDK_MAKE=$SDK_ASP/mk
SDK_DOC=$SDK_PREFIX/doc
SDK_CW_TEMPLATES=$SDK_CW/templates
SDK_CW_SAMPLES=$SDK_CW/samples

########################################
## Create Directories for SDK package ##
########################################

if [ ! -f $CONF ]; then
    
    ## create installation directories
    echo "Creating $SDK_PREFIX :`mkdir -p $SDK_PREFIX` Done"
    echo "Creating $SDK_ASP :`mkdir -p $SDK_ASP` Done"
    echo "Creating $SDK_CW :`mkdir -p $SDK_CW` Done"
    echo "Creating $SDK_RT: `mkdir -p $SDK_RT` Done"
    echo "Creating $SDK_INCLUDE: `mkdir -p $SDK_INCLUDE` Done"
    echo "Creating $SDK_LIB: `mkdir -p $SDK_LIB` Done"
    echo "Creating $SDK_BIN: `mkdir -p $SDK_BIN` Done"
    echo "Creating $SDK_MOD: `mkdir -p $SDK_MOD` Done"
    echo "Creating $SDK_TEMPLATES: `mkdir -p $SDK_TEMPLATES` Done"
    echo "Creating $SDK_SAMPLES: `mkdir -p $SDK_SAMPLES` Done"
    echo "Creating $SDK_TEST: `mkdir -p $SDK_TEST` Done"
    echo "Creating $SDK_MAKE: `mkdir -p $SDK_MAKE` Done"
    echo "Creating $SDK_DOC: `mkdir -p $SDK_DOC` Done"
    echo "Creating $SDK_CW_TEMPLATES: `mkdir -p $SDK_CW_TEMPLATES` Done"
    echo "Creating $SDK_CW_SAMPLES: `mkdir -p $SDK_CW_SAMPLES` Done"

    ##########################
    ## Copying Header Files ##
    ##########################

    echo -n "Copying header files: "
    cp -fr $CLOVIS_ROOT/SAFplus/include/* $SDK_INCLUDE
    echo "Done"

    #######################
    ## Copying Makefiles ##
    #######################

    echo -n "Copying Make file includes : "
    cp -pr $CLOVIS_ROOT/SAFplus/mk/* $SDK_MAKE
    cp -f $CLOVIS_ROOT/SAFplus/build/common/mk/make-cross.mk $SDK_MAKE
    sed -e s/"SAFplus\/build\/rt\/scripts\/package.sh"/"SAFplus\/rt\/scripts\/package.sh"/g $SDK_MAKE/make-common.mk >$SDK_MAKE/make-common.mk.bak
    mv $SDK_MAKE/make-common.mk.bak $SDK_MAKE/make-common.mk

    echo "Done"

    ###########################
    ## Copying Documentation ##
    ###########################

    echo -n "Copying docs : "
    cp -pr $CLOVIS_ROOT/SAFplus/doc/* $SDK_DOC
    echo "Done"

    #####################################
    ## Copying packaging related files ##
    #####################################
    
 
    echo -n "Copying RT packaging files : "
    mkdir -p $SDK_RT/scripts
    mkdir -p $SDK_RT/conf
    cp -f $CLOVIS_ROOT/SAFplus/build/common/scripts/package.sh $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/scripts/clIocLoadModule.sh $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/scripts/clIocUnloadModule.sh $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/build/rt/scripts/install.sh $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/build/rt/scripts/uninstall.sh $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/build/rt/scripts/asp $SDK_RT/scripts/
    cp -f $CLOVIS_ROOT/SAFplus/build/rt/README $SDK_RT
    cp -f $CLOVIS_ROOT/SAFplus/build/rt/conf/* $SDK_RT/conf/
    echo "Done"

    #######################################
    ## Copying Scripts - asp-sdk-config ##
    #######################################

    echo -n "Copying scripts : "
    cp -rf $CLOVIS_ROOT/SAFplus/build/common/scripts/asp-sdk-config $PREFIX/clovis
    chmod +x $PREFIX/clovis/asp-sdk-config
    ln -sf $PREFIX/clovis/asp-sdk-config /usr/bin/asp-sdk-config

    cp -rf $CLOVIS_ROOT/SAFplus/build/common/scripts/uninstall.sh $PREFIX/clovis
    chmod +x $PREFIX/clovis/uninstall.sh
    echo "Done"

else
# If this is not first time SAFplus SDK installation , only copy 
# architecture specific binaries    

    OLDPREFIX=$PREFIX
    source $CONF

    #Check if PREFIX is the same as the old prefix
    if [ $OLDPREFIX != $PREFIX ] ;then
        echo "ERROR : Another sdk has been installed in a different directory"
        echo "Please uninstall the previous sdk by running uninstall.sh under $PREFIX/clovis driectory"
        echo "If the error still persists after sdk has been removed , manually remove $CONF"
        exit
    fi
    PREFIX=$OLDPREFIX
    echo "Creating $SDK_LIB: `mkdir -p $SDK_LIB` Done"
    echo "Creating $SDK_BIN: `mkdir -p $SDK_BIN` Done"
    echo "Creating $SDK_MOD: `mkdir -p $SDK_MOD` Done"
fi

#######################
## Copying libraries ##
#######################
    
echo -n "Copying libraries : "
cp -pr $ASP_LIB/* $SDK_LIB
echo "Done"

######################
## Copying Binaries ##
######################

echo -n "Copying binaries : "
cp -pr $MODEL_BIN/* $SDK_BIN
echo "Done"

############################
## Copying kernel Modules ##
############################

echo -n "Copying kernel modules : "
cp -pr $ASP_KMOD/* $SDK_MOD
echo "Done"

##################################
## Generating asp-sdk-2.3.conf ##
##################################

echo -n "Generating /etc/asp-sdk-$ASP_VERSION.conf : "
echo "export PREFIX=$PREFIX" > $CONF
echo "Done"


