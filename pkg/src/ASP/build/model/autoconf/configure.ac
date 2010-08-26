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
# ModuleName  : model
# File        : configure.ac
################################################################################
# Description :
## Autoconf script for ASP model ( SDK compilation)					 
################################################################################

##################################################
# Initialize Autoconf and environment variables  #
##################################################

#Initializing Autoconf Script 
AC_INIT(asp, 3.0)
export ASP_VERSION=2.3

###########################################
## Checks if ASP SDK has been installed ##
###########################################


AC_MSG_CHECKING(for ASP-sdk 2.3)
if [[ ! -f /etc/asp-sdk-$ASP_VERSION.conf ]] ;then
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([
*** ASP $ASP_VERSION SDK needs to be installed ])
fi
AC_MSG_RESULT(yes)

source /etc/asp-sdk-$ASP_VERSION.conf
export CLOVIS_ROOT=$PREFIX/clovis/sdk/$ASP_VERSION
export CL_TARGET_PLATFORM=`uname -m`
export KERNEL_SOURCE=/lib/modules/`uname -r`/build
export SNMP_BUILD=1
export CROSS_FLAG=0
export CROSS_BUILD=0
export KERNEL_FLAG=0
export KERNEL=linux
export BUILD_TOOLS_DIR=/opt/clovis/buildtools
export NET_SNMP_CONFIG=net-snmp-config
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/

#####################################################
## Process command line options to configure script #
#####################################################

AC_ARG_WITH(cm-build,
[  --without-cm-build
                                  Will not build Chassis Manager Server .],
                                  [export CM_BUILD=0],                                 
                                  [export CM_BUILD=1] 
                                  )
AC_ARG_WITH(cross-build,
  --with-cross-build=toolchain-name
                                 - Triggers a host-independent build using the specified
                                    toolchain.  The following toolchains are available
                                    at your host:
                                     ,
                                  [export CROSS_FLAG=1 ;
                                   export CROSS_BUILD=$with_cross_build],                                 
                                  [export CROSS_FLAG=0;] 
                                  )
AC_ARG_WITH(kernel-build,
[  --with-kernel-build=kernel-dir
                                 -Name of an alternative kernel under the given
                                    toolchain. ]
                                     ,
                                  [ 
                                    export KERNEL_FLAG=1;
                                    export KERNEL=$with_kernel_build],                                 
                                  [export KERNEL_FLAG=0;] 
                                  )
###################################################
## If Cross compilation is enabled , build tools ##
## must be picked from the appropriate build     ##
## directory                                     ##
###################################################

if [[ $CROSS_FLAG == 1 ]] ;then
    if [[ ! -d $BUILD_TOOLS_DIR/$CROSS_BUILD ]] ;then
        AC_MSG_ERROR([
    ***   Build Tools directory $BUILD_TOOLS_DIR/$CROSS_BUILD
    ***   does not exist])
    fi;
    if [[ $KERNEL_FLAG == 1 ]] ; then
        if [[ ! -d $BUILD_TOOLS_DIR/$CROSS_BUILD/src/$KERNEL ]] ;then
            AC_MSG_ERROR([
        ***   Build Tools directory $BUILD_TOOLS_DIR/$CROSS_BUILD/src/$KERNEL
        ***   does not exist])
        fi;
    fi;
    cross_compiling=yes;
    source $BUILD_TOOLS_DIR/$CROSS_BUILD/config.mk
    export CC=$BUILD_TOOLS_DIR/$CROSS_BUILD/bin/$TARGET-gcc
    GCC=$CC
    LD=$BUILD_TOOLS_DIR/$CROSS_BUILD/bin/$TARGET-ld
    AR=$BUILD_TOOLS_DIR/$CROSS_BUILD/bin/$TARGET-ar
    RANLIB=$BUILD_TOOLS_DIR/$CROSS_BUILD/bin/$TARGET-ranlib
    export PKG_CONFIG_PATH=$BUILD_TOOLS_DIR/$CROSS_BUILD/lib/pkgconfig
    export NET_SNMP_CONFIG=$BUILD_TOOLS_DIR/$CROSS_BUILD/bin/net-snmp-config
    export CL_TARGET_PLATFORM=$ARCH
    export KERNEL_SOURCE=$BUILD_TOOLS_DIR/$CROSS_BUILD/src/$KERNEL
  
fi;

#Checking For Version of GCC compiler
echo "Testing for Build TOOLS"
AC_PROG_CC
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

echo -n "Testing For Correct version of gcc... "
OH_CHECK_GCC(3,2,0)

KERNEL_VERSION_FILE=$KERNEL_SOURCE/include/linux/version.h
KERNEL_MAJOR_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print rshift($3,16)}' $KERNEL_VERSION_FILE`
KERNEL_MINOR_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print and(rshift($3,8),0xff)}' $KERNEL_VERSION_FILE`
KERNEL_PATCH_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print and($3,0xff)}' $KERNEL_VERSION_FILE`
export CL_TARGET_OS=linux-$KERNEL_MAJOR_VERSION.$KERNEL_MINOR_VERSION.$KERNEL_PATCH_VERSION

###############################################
## Checking for openhpi 2.3.0 . If not found ##
## CM Server compilation is disabled         ##
###############################################

if [[ $CM_BUILD -eq 1 ]]; then
    AC_MSG_CHECKING(for openhpi 2.3.0)
    if pkg-config --atleast-version 2.3.0 openhpi; 
    then
        export CM_BUILD=1 ;
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
		AC_MSG_WARN([
********************************************
*** CM Server will not be built .        ***
*** Need openhpi version 2.3.0 or higher.***
*********************************************
])
    export CM_BUILD=0 ;
    fi
fi

###############################################
## Checking for net-snmp 5.3.0. If not found ##
## SNMP server compilation is disabled       ##
###############################################

AC_ARG_WITH(snmp-build,
[  --without-snmp-build
                                  Disable snmp agent client and snmp agent server compilation.],
                                  [export SNMP_BUILD=0
                                  ]
                                  ,
                                  [export SNMP_BUILD=1] 
                                  )


if [[ $SNMP_BUILD -eq 1 ]]; then
    OH_CHECK_NETSNMP
fi

export MODEL_LIB=`pwd`/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
export ASP_LIB=$CLOVIS_ROOT/ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
export MODEL_BIN=`pwd`/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS/bin
export MODEL_CONFIG=`pwd`/config
export ASP_CONFIG=`pwd`/config
export ASP_BIN=$MODEL_BIN

mkdir -p $MODEL_BIN
mkdir -p $ASP_BIN

#####################################################
## Substituting environment files in template files #
#####################################################

AC_SUBST(CLOVIS_ROOT)
AC_SUBST(CROSS_FLAG)
AC_SUBST(CROSS_BUILD)
AC_SUBST(KERNEL_FLAG)
AC_SUBST(KERNEL)
AC_SUBST(CL_TARGET_OS)
AC_SUBST(CL_TARGET_PLATFORM)
AC_SUBST(ASP_VERSION)
AC_SUBST(ASP_LIB)
AC_SUBST(MODEL_LIB)
AC_SUBST(MODEL_BIN)
AC_SUBST(MODEL_CONFIG)
AC_SUBST(ASP_CONFIG)
AC_SUBST(ASP_BIN)
AC_SUBST(SNMP_BUILD)
AC_SUBST(CM_BUILD)

AC_CONFIG_FILES(Makefile:Makefile.in clModelAspEnv.sh:clModelAspEnv.sh.in)
AC_OUTPUT
