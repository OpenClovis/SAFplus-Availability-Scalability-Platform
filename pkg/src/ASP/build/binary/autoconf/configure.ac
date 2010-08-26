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
# ModuleName  : binary
# File        : configure.ac
################################################################################
# Description :
## Autoconf script for ASP binary SDK					 
################################################################################

##################################################
# Initialize Autoconf and environment variables  #
##################################################

#Initializing Autoconf Script 
AC_INIT(asp, 3.0)
AC_PREFIX_DEFAULT(/usr/local)

#Initialize Environment variables
cd ..;
export CLOVIS_ROOT=`pwd`
export OPENHPILIBS=`pkg-config --libs openhpi openhpiutils`
cd -

export NET_SNMP_CONFIG=net-snmp-config
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig/

export CL_TARGET_PLATFORM=`uname -m`
export BUILD_TOOLS_DIR=/opt/clovis/buildtools
export CROSS_FLAG=0
export CROSS_BUILD=0
export KERNEL_FLAG=0
export KERNEL=linux
export KERNEL_SOURCE=/lib/modules/`uname -r`/build
export SNMP_BUILD=1
export ASP_MODEL_NAME=SampleModel

if [[ $prefix != "NONE" ]] ; then
    export PREFIX=$prefix
else
    export PREFIX=/usr/local
fi


if [[ ! -d servers ]] ; then
    AC_MSG_ERROR([Please run the script from ASP directory])
fi;

#####################################################
## Process command line options to configure script #
#####################################################

AC_ARG_WITH(model-name,
[  --with-model-name=Model Name
                                  Default directory for Model.],
                                  [export ASP_MODEL_NAME=$with_model_name],
                                  [export ASP_MODEL_NAME=SampleModel] 
                                  )

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

#Checks if specified Model exists or not
if [[ ! -d models/$ASP_MODEL_NAME ]] ;then
    AC_MSG_ERROR([
***    Model $ASP_MODEL_NAME Does not exist 
***    Aborting configure])
fi;

###################################################
## If Cross compilation is enabled , build tools ##
## must be picked from the appropriate build     ##
## directory                                     ##
###################################################

if [[ $CROSS_FLAG == 1 ]] ;then

    # Check if build toolchain specified exists or not
    if [[ ! -d $BUILD_TOOLS_DIR/$CROSS_BUILD ]] ;then
        AC_MSG_ERROR([
    ***   Build Tools directory $BUILD_TOOLS_DIR/$CROSS_BUILD
    ***   does not exist])
    fi;

    #Checks if specified kernel exists or not 
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
#TODO Check for glibc version

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h termios.h unistd.h], ,[AC_MSG_ERROR([Sorry, can't do anything for you])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm bzero dup2 gethostbyname gettimeofday inet_ntoa localtime_r memmove memset mkdir munmap select setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol strtoul strtoull sysinfo],,[AC_MSG_ERROR([Sorry, can't do anything for you])])


#Setting Kernel versions
KERNEL_VERSION_FILE=$KERNEL_SOURCE/include/linux/version.h
KERNEL_MAJOR_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print rshift($3,16)}' $KERNEL_VERSION_FILE`
KERNEL_MINOR_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print and(rshift($3,8),0xff)}' $KERNEL_VERSION_FILE`
KERNEL_PATCH_VERSION=`awk '$2=="LINUX_VERSION_CODE"{print and($3,0xff)}' $KERNEL_VERSION_FILE`
export CL_TARGET_OS=linux-$KERNEL_MAJOR_VERSION.$KERNEL_MINOR_VERSION.$KERNEL_PATCH_VERSION


#Checking if the correct version of ASP binary package is available
AC_MSG_CHECKING( for Correct Binary ASP package )
if [[ ! -d $CLOVIS_ROOT/ASP/target/$CL_TARGET_PLATFORM/$CL_TARGET_OS ]] ; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([
*** This Binary package was not built for "$CL_TARGET_PLATFORM" platform
*** and "$CL_TARGET_OS" OS . Please run configure again with (or without)
*** the correct parameter for --with-cross-build
])
AC
fi    
AC_MSG_RESULT(yes)



#Checking for presence of gdbm
AC_CHECK_LIB([gdbm], [gdbm_open])

#Checking for version of Glib-2.0
GLIB_REQUIRED_VERSION=2.3.0
GLIB=glib-2.0

AC_MSG_CHECKING(for $GLIB $GLIB_REQUIRED_VERSION)
if pkg-config --atleast-version $GLIB_REQUIRED_VERSION $GLIB; then
   :
else
   AC_MSG_ERROR([
*** GLIB >= $GLIB_REQUIRED_VERSION is required. The latest version of
*** GLIB is always available from ftp://ftp.gtk.org/.])
fi
AC_MSG_RESULT(yes)

###############################################
## Checking for openhpi 2.3.0 . If not found ##
## CM Server compilation is disabled         ##
###############################################

#Checking for openhpi 2.3.0
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

export MACH=`uname -m`

if [[ $MACH == "i686" ]]; then
	export LDEMULATION=elf_i386
fi
if [[ $MACH == "ppc" ]]; then
	export LDEMULATION=elf32ppclinux
fi

#####################################################
## Substituting environment files in template files #
#####################################################

AC_SUBST(ASP_MODEL_NAME)
AC_SUBST(CLOVIS_ROOT)
AC_SUBST(OPENHPILIBS)
AC_SUBST(LDEMULATION)
AC_SUBST(SNMP_BUILD)
AC_SUBST(CROSS_FLAG)
AC_SUBST(CROSS_BUILD)
AC_SUBST(PREFIX)
AC_SUBST(CM_BUILD)
AC_SUBST(CL_TARGET_PLATFORM)
AC_SUBST(CL_TARGET_OS)
AC_SUBST(KERNEL_FLAG)
AC_SUBST(KERNEL)
AC_CONFIG_FILES(clasp.env:build/common/templates/clasp.env.in Makefile:build/binary/templates/Makefile.in mk/make-cross.mk:build/common/templates/make-cross.mk.in)
AC_OUTPUT
chmod +w clasp.env
source clasp.env
