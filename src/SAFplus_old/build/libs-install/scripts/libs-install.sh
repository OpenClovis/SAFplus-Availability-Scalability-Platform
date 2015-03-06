#!/bin/bash
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

##############################################################################
# Install SAFplus libraries to SDK directory or custom location specified by
# PREFIX
##############################################################################

LIBS_INSTALL_PATH=$PREFIX/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
INCLUDE_PATH=$PREFIX/include

#Use different commands for Solaris
HOST_OS=`uname -s`
if [ $HOST_OS == "SunOS" ]; then
    AWK=gawk
else
    AWK=awk
fi

if [ ! -d $LIBS_INSTALL_PATH ]; then
    mkdir -p $LIBS_INSTALL_PATH
fi

if [ ! -d $INCLUDE_PATH ]; then
    mkdir -p $INCLUDE_PATH
    mkdir -p $INCLUDE_PATH/ipi
fi

#
# Populate include path
echo -n "Installing SAFplus headers to $PREFIX..."
cd $INCLUDE_PATH
find $CLOVIS_ROOT/SAFplus/components -name '*.h' -o -name '*.hxx' | \
    $AWK '{ str=sprintf("cp %s .", $0); system(str) }'
find $CLOVIS_ROOT/SAFplus/components -name '*.h' | \
    grep ipi | \
    $AWK '{ str=sprintf("cp %s ipi/.", $0); system(str) }'
cd - > /dev/null
echo "done."

#
# Copy built libs to install path
echo -n "Installing SAFplus libraries to $PREFIX..."
cp -r $ASP_LIB/* $LIBS_INSTALL_PATH
cp -r $PROJECT_ROOT/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib/* $LIBS_INSTALL_PATH
# As libCmServer locate at asp/target, need to copy from there
# when making asp-install
if [ "$ASP_MODEL_NAME" == "asp" ]; then
cp -r $MODEL_LIB/* $LIBS_INSTALL_PATH
fi
echo "done."

#
# TODO: update tag in sdk_dir/lib/clconfig.rc
