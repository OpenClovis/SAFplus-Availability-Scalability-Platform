#!/bin/bash
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

##############################################################################
# Install ASP libraries to SDK directory or custom location specified by
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
echo -n "Installing ASP headers to $PREFIX..."
cd $INCLUDE_PATH
find $CLOVIS_ROOT/ASP/components -name '*.h' | \
    $AWK '{ str=sprintf("cp %s .", $0); system(str) }'
find $CLOVIS_ROOT/ASP/components -name '*.h' | \
    grep ipi | \
    $AWK '{ str=sprintf("cp %s ipi/.", $0); system(str) }'
cd - > /dev/null
echo "done."

#
# Copy built libs to install path
echo -n "Installing ASP libraries to $PREFIX..."
cp -r $ASP_LIB/* $LIBS_INSTALL_PATH
cp -r $PROJECT_ROOT/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib/* $LIBS_INSTALL_PATH
echo "done."

#
# TODO: update tag in sdk_dir/lib/clconfig.rc
