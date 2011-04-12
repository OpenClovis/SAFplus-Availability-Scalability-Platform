#!/bin/bash
################################################################################
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
################################################################################
#
# Build: 5.0.0
#
################################################################################
# ModuleName  : packaging
# File        : install.sh
################################################################################
# Description :
# Script to install required 3rd party packages, ASP source code and IDE
# plugins
################################################################################

# values instantiated by the package script
THIRDPARTYPKG=3rdparty-base-1.15.tar
THIRDPARTYMD5=3rdparty-base-1.15.md5

coloring=0

# cleanup and exit if any of the operations fail
myexit () {

	row=$1
	shift
	echo -en "$red" >&2
	tput cup $row 44
	printf "%-17s" 'Failed' >&2
	tput el
	ROWS=`tput lines`
	ROWS=`expr $ROWS - 2`
	tar zcf $WORKING_DIR/log.tar.gz $WORKING_DIR/log >/dev/null 2>&1
	tput cup $ROWS 0
	echo "$*, send log.tar.gz to support@openclovis.com for help" >&2
	tput cnorm
	echo -en "$blue" >&2
	kill `cat $INSTALLER`
	exit
}

# cleanup and exit if any of the operations fail
myexit_notput () {

	tput setf 4
	printf "\b%-17s" 'Failed' >&2
	tput setf 1
	tput el
	ROWS=`tput lines`
	ROWS=`expr $ROWS - 2`
	tar zcf $WORKING_DIR/log.tar.gz $WORKING_DIR/log >/dev/null 2>&1
	tput cup $ROWS 0
	tput setf 4
	echo "$*, send log.tar.gz to support@openclovis.com for help" >&2
	tput setf 1
	tput cnorm
	kill `cat $INSTALLER`
	exit
}

# roll cursor to indicate busy
roll () {
	p1=$1
	echo -n " "
	while [ true ];
	do
        kill -0 $p1 > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			return
		fi
		printf "\b/"
		sleep 0.1
		printf "\b-"
		sleep 0.1
		printf "\b\\"
		sleep 0.1
		printf "\b|"
		sleep 0.1
	done
}

# move the cursor back and forth to indicate the scan
scan () {
	tput cnorm
	p1=$1
	echo -n "   "
	while [ true ];
	do
		kill -0 $p1 >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			return
		fi
		printf "\b"
		sleep 0.1
		printf "\b"
		sleep 0.1
		printf "\b"
		sleep 0.1
		printf "."
		sleep 0.1
		printf "."
		sleep 0.1
		printf "."
		sleep 0.1
	done
	tput civis
}

# cleanup
# remove the lock file and save the log directory
cleanup () {
	echo ""
	echo -n "Cleaning up ... "
	if [ -f $INSTALLER ]; then
		rm -f $INSTALLER
	fi
	cd $WORKING_DIR
	if [ -d log ]; then
		mv log log.`date "+%F-%H-%M"`
		echo -n "log directory saved as log.`date "+%F-%H-%M"`"
	else
		echo "done"
	fi
	echo
	tput sgr0
}

trapped () {
	rm -f $INSTALLER;
	tput cup `tput lines` 0;
	echo -en $red >&2
	echo "[WARNING] aborting installation..."
	echo "[WARNING] The installation is incomplete, rerun $(basename $0) to restart and complete installation"
	tput cnorm;
	tput sgr0;
}

if [ ! $coloring == 0 ]; then
    black='\E[30;49m'
    red='\E[31;49m'
    green='\E[32;49m'
    yellow='\E[33;49m'
    blue='\E[0;34;49m'
    magenta='\E[35;49m'
    cyan='\E[36;49m'
    white='\E[37;49m'
    blue_bold='\E[1;34;49m'
    red_bold='\E[1;31;49m'
else
    black=''
    red=''
    green=''
    yellow=''
    blue=''
    magenta=''
    cyan=''
    white=''
    blue_bold=''
    red_bold=''
fi

# Check for SunOS
hostos=$(uname -s)
if [ "$hostos" == "SunOS" ]; then
    echo ""
    echo "This installer is not intended for Solaris.  Please run "
    echo "install-solaris.sh instead."
    echo ""
    exit 1
fi

# Don't allow duplicate instance of the installer run
INSTALLER=/var/tmp/.openclovis_installer
PID=$$
if [ -f $INSTALLER ]; then
	if [ ! -d /proc/$(cat /var/tmp/.openclovis_installer) ]; then
		rm -f /var/tmp/.openclovis_installer
	else
		echo "Installation is in progress" >&2
		echo -en "Process ID:$red_bold " >&2
		cat $INSTALLER
		echo -e "$blue" >&2
		tput sgr0
		exit 1
	fi
fi
echo $$ > $INSTALLER
chmod 444 $INSTALLER
																																		
# Trap signals: cleanup & exit
trap "trapped; exit 1" 1 2 3 15 20

# change the default umask to enable world write. This is not ok, but what to do ;)
# ASP build is very tightly coupled to the Model code and IDE launch will generate
# some temporary files in the same directory where IDE is installed
# umask 0000

# parse option to specify openhpi tarball
usage() {
    echo ""
    echo "install.sh - Installation tool for OpenClovis SDK 5.0"
    echo ""
    echo "Installs OpenClovis SDK 5.0 to a system intended for use"
    echo "for OpenClovis ASP development."
    echo ""
    echo "Usage:"
    echo "  $(basename $0) [ -p <openhpi-package-tarball> ]"
    echo ""
}

CUSTOM_OPENHPI=0
while getopts :p:h options; do
    case $options in
        p)
            CUSTOM_OPENHPI=1
            CUSTOM_OPENHPI_PKG=$OPTARG
            if [ ! -f $CUSTOM_OPENHPI_PKG ]; then
                echo "$CUSTOM_OPENHPI_PKG not found.  Please specify a valid"
                echo "OpenHPI package tarball or run this installer without"
                echo "the -p option to use the default version of OpenHPI."
                echo ""
                exit 1
            fi
            ;;
        h)
            usage
            exit 0
            ;;
        ?)
            echo "Invalid option."
            usage
            exit 1
    esac
done

# we want the target directory to be named as processor-os-release
# example i686-gnu-linux-2.6.9-5.el
#install=`uname -m;uname -o;uname -r`
#INSTALL_DIR=`echo $install | sed -e 's/ /-/g' -e 's/\//-/' | tr '[A-Z]' '[a-z]'`
INSTALL_DIR=local
WORKING_DIR=$(pwd)

if [ -f "BUILD" ]; then
    ASP_VERSION=$(cat BUILD | head -1 | cut -d "=" -f2 | cut -d "-" -f3)
    ASP_REVISION=$(cat BUILD | head -1 | cut -d "=" -f2 | cut -d "-" -f4)
else
    echo "[ERROR] No $WORKING_DIR/BUILD file found, cannot continue installation." >&2
    cleanup
    exit 1
fi


PACKAGE_NAME=sdk-$ASP_VERSION
WORKING_ROOT=$(dirname $WORKING_DIR)
KERNEL_VERSION=$(uname -r)
BUILD_DIR=$WORKING_DIR/build

MD5CHECKSUM="NO"

PROCESSOR=`uname -m`

INSTALLIDE="YES"

PATCH_ERROR="[ERROR] Patch failed"
CONFIGURE_ERROR="[ERROR] Configure failed"
MAKE_ERROR="[ERROR] Make failed"
MAKE_INSTALL_ERROR="[ERROR] Make install failed"
EXTRACT_ERROR="[ERROR] Could not extract"

# . requires write permissions
# cannot continue without that.
if [ ! -w $WORKING_DIR ]; then
	echo -en "$red" >&2
	echo "$WORKING_DIR is read-only" >&2
	echo -en "$blue" >&2
	echo "Copy the contents of this folder to a directory where you have write permission and try again" >&2
	echo >&2
	cleanup
	exit 1
fi

# Let's know if we are running as a non-root user
ID=$(id -u)

# No point in continuing with out basic requirements
# viz.,
# pkg-config: this is being used by most of the pre-requisites
# gcc: obviously
# perl: some configure scripts rely on perl to sed
# md5sum: to calculate the md5check sum of the downloaded packages
# search for these executables in /usr/bin:/usr/local/bin
OLDPATH=${PATH}
export PATH=/usr/bin/:/usr/local/bin:$PATH
for essentials in 'gcc' 'pkg-config' 'perl' 'md5sum'
do
	which $essentials >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo -en "$red"  >&2
		echo "$essentials(1) is required to build the ASP/IDE prerequisites"  >&2
		echo "Cannot find $essentials in your PATH environment variable"  >&2
		echo "Please set your PATH environment variable to the directory where $essentials is installed"  >&2
		echo -e "$blue"  >&2
		cleanup
		exit 1;
	fi
done

tput clear

echo -e "${blue_bold}OpenClovis SDK${blue} Installer"
echo -e ""
echo -e "Welcome to the ${blue_bold}OpenClovis SDK${blue} $ASP_VERSION $ASP_REVISION Installer"
echo -e ""
echo -e "This program helps you to install:"
echo -e "    - Required 3rd-party Packages"
echo -e "    - The ${blue_bold}OpenClovis${blue} SDK"
echo -e ""
echo -e "Installation Directory Prerequisites"
echo -e "    - At least ${blue_bold}500MB${blue} free disk space"
echo -e "    - Write permission to the installation directory"
echo -e ""
echo -e "Note: You may experience slow installation if the target installation"
echo -e "      directory is mounted from a remote file system (e.g., NFS)."
echo -e ""

CACHE_DIR="${HOME}/.clovis/$PACKAGE_NAME"
CACHE_FILE="install.cache"
defsand=""
if [ -f $CACHE_DIR/$CACHE_FILE ]; then
	defsand=$(cat $CACHE_DIR/$CACHE_FILE)
else
	if [ $ID -ne 0 ]; then
		defsand=$HOME/clovis
	else
		defsand=/opt/clovis
	fi
fi

echo -e "For all distributions of Linux supported by the ${blue_bold}OpenClovis${blue} SDK, the"
echo -e "corresponding preinstall script must be run prior to this installer."
echo -e "This ensures that your system has all the prerequisite software packages"
echo -e "required to install the ${blue_bold}OpenClovis${blue} SDK.  For a list of supported Linux"
echo -e "distributions and further information, please refer to the Installation"
echo -e "Guide."
echo -e ""
echo -e "Please press <enter> to continue or <ctrl-c> to quit this installer and"
echo -en "run the preinstall script for your distribution..."

read preinstall_done dummy

echo -e ""
echo -en "Enter the installation root directory [default: ${blue_bold}$defsand${blue}]: "
read sand dummy #don't allow any blank spaces in the directory name
if [ "x${sand}" == "x" ]; then
	sand=$defsand
else
	sand=`echo $sand | perl -n -e 's/(\/*)$//; print "$_"'`
fi

# handle ~ in directory name
sand=$(echo $sand | sed -e "s/^~/$(echo $HOME | sed -e 's/\//\\\//g')/")

if [ ! -d $sand ]; then
	echo -n "Directory $sand does not exist, should it be created? <y|n> [y]: "
	read dir_create dummy
	if [ -z $dir_create ]; then
		dir_create='y';
	fi
	case $dir_create in
		Y|y)
			rm -rf $WORKING_DIR/log
			mkdir -p $sand
			if [ $? -ne 0 ]; then
				echo -en "$red" >&2
				echo "[ERROR] Failed to create directory" >&2
				cleanup
				exit 1
			fi
			;;
		N|n)
			cleanup
			exit 1
			;;
		*)
			echo -en "$red" >&2
			echo "[ERROR] Invalid option, considering as no" >&2
			cleanup
			exit 1
	esac
fi

# no point in continuing if the write permission is turned off
if [ ! -w $sand ]; then
	echo -en "$red" >&2
	echo "You do not have write permission on $sand directory" >&2
	echo -en "$blue" >&2
	cleanup
	exit
fi

# convert the sandbox directory into absolute path
# make install will fail otherwise
echo $sand | grep '^/' >/dev/null 2>&1
if [ $? -eq 0 ]; then
	# do nothing if the sandbox is mentioned as an absolute path
	INSTALL_ROOT="$sand"
else
	# remove './' prefix in the sand box if any and prepend the
	# current working directory
	sand=`echo $sand | perl -n -e 's/^\.(\/\+)//; print "$_"'`
	if [ "$sand" == "." ]; then
		INSTALL_ROOT=$WORKING_DIR
	else
		INSTALL_ROOT=$WORKING_DIR/$sand
	fi
fi

# save settings, always overwrite to save the current settings
if [ ! -d $CACHE_DIR ]; then
	mkdir -p $CACHE_DIR
fi
echo "$INSTALL_ROOT" > $CACHE_DIR/$CACHE_FILE

BUILDTOOLS="$INSTALL_ROOT/buildtools"
PACKAGE_ROOT="$INSTALL_ROOT/$PACKAGE_NAME"

PREFIX="$BUILDTOOLS/$INSTALL_DIR"
PREFIX_BIN="$PREFIX/bin"
PREFIX_LIB="$PREFIX/lib"
export PATH=${PREFIX_BIN}:${PATH}

IDE_ROOT="$PACKAGE_ROOT/IDE"	# IDE is installed here
ASP_ROOT="$PACKAGE_ROOT/src/ASP"	# ASP sources are copied here
DOC_ROOT="$PACKAGE_ROOT/doc"	# DOCS are copied here
BIN_ROOT="$PACKAGE_ROOT/bin"	# scripts are copied here

LIB_ROOT="$PACKAGE_ROOT/lib"	# copy rc scripts in this directory

# let us create the directories
for DIR in $PREFIX #$LIB_ROOT $IDE_ROOT $ASP_ROOT $DOC_ROOT $BIN_ROOT
do
	if [ ! -d $DIR ]; then
		mkdir -p $DIR
	elif [ ! -w $DIR ]; then
		echo -en "$red" >&2
		echo "[ERROR] You do not have write permission on $DIR" >&2
		echo -en "$blue" >&2
		cleanup
		exit
	fi		
done

ASP="ASP"
IDE="IDE"
GPL="0"
if [ $INSTALLIDE == "YES" ]; then
	if [ -d src/$IDE ]; then
		GPL='1'
	fi
fi

INSTALL_DOCS=1

export PKG_CONFIG_PATH=$PREFIX_LIB/pkgconfig:$PKG_CONFIG_PATH

# ASP Dependencies
INSTALL_SUBAGENT=0
INSTALL_OPENHPI=0
INSTALL_NETSNMP=0
INSTALL_BEECRYPT=0
INSTALL_GCC=0
INSTALL_GLIB=0
INSTALL_GDBM=0
INSTALL_GLIBC=0
INSTALL_TIPC=0
INSTALL_TIPC_CONFIG=0

BEECRYPT_WARNING=0
OPENHPI_WARNING=0

# IDE Dependencies
INSTALL_ECLIPSE=0
INSTALL_JRE=0
INSTALL_PYTHON=0
INSTALL_EMF=0
INSTALL_GEF=0
INSTALL_CDT=0

# Raise this flag only if there are any pending dependencies
DEPENDENCY=0

CC=gcc
EPREFIX=/opt/tmp/clovis
GCC_VERSION=gcc-3.2.3
NET_SNMP_VERSION=net-snmp-5.4.2
BEECRYPT_VERSION=beecrypt-4.1.2
GDBM_VERSION=gdbm-1.8.0
GLIB_VERSION=glib-2.12.6
GLIBC_VERSION=glibc-2.3.5
OPENHPI_VERSION=openhpi-2.14.0
OPENHPI_PKG=openhpi-2.14.0.tar.gz
if [ "$CUSTOM_OPENHPI" == "1" ]; then
    OPENHPI_PKG=$CUSTOM_OPENHPI_PKG
fi
OPENHPI_PKG_VER=$(basename $OPENHPI_PKG | cut -d- -f 2 | sed -e 's/\.*tar\.gz//g')
OPENHPI_SUBAGENT_VERSION=openhpi-subagent-2.3.4
PYTHON_VERSION=2.4.1
TIPC_VERSION=tipc-1.5.12
TIPCUTILS_VERSION=tipcutils-1.0.4

# Changed the JRE bin distribution format to tar.gz to make the installation
# non interactive
JRE_VERSION=1.5.0.03

ECLIPSE_SDK_VERSION=3.3.2
export ECLIPSE=$PREFIX/eclipse

if [ -d $PREFIX ] && [ -f $PREFIX_BIN/net-snmp-config ]; then
	NET_SNMP_CONFIG=$PREFIX_BIN/net-snmp-config
else
	NET_SNMP_CONFIG=net-snmp-config
fi

export PATH=$PREFIX_BIN:$PREFIX/jre1.6.0_21/bin:$PATH
export JAVA_HOME=$PREFIX/jre1.6.0_21
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PREFIX_LIB

# this works on mounted file systems as well, yes I tried it.
echo >&2
echo -n "Checking the disk space in $INSTALL_ROOT ... " >&2

available=0
lines=`df $INSTALL_ROOT | wc -l`
local=0
if [ $lines -eq 3 ]; then
	# this is from a volume manager
	available=`df $INSTALL_ROOT | tail -n 1 | awk '{print $3}'`
	df $INSTALL_ROOT | tail -n 2 | head -1 | grep -e '^\/' >/dev/null 2>&1
	local=$?
elif [ $lines -eq 2 ]; then
	available=`df $INSTALL_ROOT | tail -n 1 | awk '{print $4}'`
	df $INSTALL_ROOT | tail -n 1 | grep -e '^\/' >/dev/null 2>&1
	local=$?
fi

if [ $available -ge 512000 ]; then
	echo "ok" >&2
	host=`df $INSTALL_ROOT | tail -n 1 | awk '{print $1}'`
	if [ $local -ne 0 ]; then
		# not on the local file system, as well warn the user about the
		# possible delays
		echo "$INSTALL_ROOT is mounted from $host" >&2
		echo "you might experience unexpected delays while using" >&2
		echo "the libraries and binaries  installed in this path" >&2
		echo "" >&2
	fi
	echo
else
	echo -en "$red" >&2
	echo "[ERROR] Disk space not sufficient, aborting the installation" >&2
	echo -en "$blue" >&2
	echo >&2
	cleanup
	exit 1
fi

#tput civis #make the cursor invisible
echo -n "Checking downloaded packages "
cd $WORKING_ROOT
if [ ! -f $THIRDPARTYPKG ]; then
	echo -en "$red" >&2
	echo "Cannot find $THIRDPARTYPKG in the $WORKING_ROOT" >&2
	cleanup
	exit 1
fi

if  [ -f $THIRDPARTYMD5 ]; then
	md5sum -c $THIRDPARTYMD5 >/dev/null 2>&1 \
	|| myexit_notput "[ERROR] Checksum error: It is not safe to install these packages, please download them again" &
	PS=$!
	scan $PS
	MD5CHECKSUM="OK"
	printf " ok\n"
else
	echo "" >&2
	echo "Optional $THIRDPARTYMD5 not found." >&2
	echo "Proceeding without integrity check.  If you would prefer to verify the" >&2
	echo "the integrity of the $THIRDPARTYPKG, please" >&2
	echo "exit and place $THIRDPARTYMD5 in the same location as" >&2
	echo "$THIRDPARTYPKG and run this script again." >&2
fi
tput cnorm

echo
echo "Checking ASP dependencies ..."

#echo -n "Checking $CC ... " >&2
GCC_VERSION=`$CC -dumpversion 2>/dev/null`
GCC_VERSIONOK=`$CC -dumpversion 2>/dev/null | sed 's/\./ /g' | awk '{\
        if ( $1 > 3 ) {\
                print "OK";\
        }\
        if ( $1 == 3 ) {\
                if ( $2 > 2 ) {\
                        print "OK";\
                }\
                if ( $2 == 2 ) {\
                        if ( $3 >= 3 ) {\
                                print "OK";
                        }\
                }\
        }\
     }'`\

if test "$GCC_VERSIONOK" == "OK"; then
	echo "ok" >/dev/null
	GCC_COMMENT="${GCC_VERSION} -> OK"
else
#	echo "not present or version mismatch"
	WHICH_GCC=`which gcc 2>/dev/null | wc -l `
	if [ $WHICH_GCC -lt 1 ]; then
		GCC_COMMENT="not installed"
	else
		GCC_COMMENT="${GCC_VERSION} -> version mismatch"
	fi
	INSTALL_GCC=1
	DEPENDENCY=1
fi

#echo -n "Checking glib-2.0 ... "
GLIB_VERSION=`pkg-config --modversion glib-2.0 2>/dev/null`
GLIB_VERSIONOK=`pkg-config --modversion glib-2.0 2>/dev/null | sed 's/\./ /g' | awk '{\
        if ( $1 > 2 ) {\
                print "OK";\
        }\
        if ( $1 == 2 ) {\
                if ( $2 > 6 ) {\
                        print "OK";\
                }\
                if ( $2 == 6 ) {\
                        if ( $3 >= 0 ) {\
                                print "OK";
                        }\
                }\
        }\
     }'`\

if test "$GLIB_VERSIONOK" == "OK"; then
	echo "ok" >/dev/null
	GLIB_COMMENT="${GLIB_VERSION} -> OK"
else
#	echo "not present or version mismatch"
	if [ -z $GLIB_VERSION ]; then
		GLIB_COMMENT="not installed"
	else 
		GLIB_COMMENT="${GLIB_VERSION} -> version mismatch"
	fi
	INSTALL_GLIB=1
	DEPENDENCY=1
fi

# compile a test program which will print the
# gdbm version string
#echo -n "Checking gdbm ... "
cat << __EOM__ > /var/tmp/gdbm_version${PID}.c
#include <gdbm.h>
#include <stdio.h>
int main()
{
	extern char *gdbm_version;
	printf("%s\n", gdbm_version);
	return 0;
}
__EOM__
$CC -lgdbm -o /var/tmp/gdbm_version${PID} /var/tmp/gdbm_version${PID}.c
GDBM_VERSION=`/var/tmp/gdbm_version${PID}  2>/dev/null \
				| awk '{print substr($0,match($0,/GDBM/))}' \
				| awk '{ print $3 }' \
				| sed -e 's/,//g'`
GDBM_VERSIONOK=`echo $GDBM_VERSION | awk '{\
	if ( $1 > 1 ) {\
		print "OK"\
	}\
	if ( $1 == 1 ) {\
			if ( $2 > 8 ) {\
                        print "OK";\
                }\
                if ( $2 == 8 ) {\
                        if ( $3 >= 0 ) {\
                                print "OK";
                        }\
                }\
        }\
     }'`\

rm -f /var/tmp/gdbm_version*
if test "$GDBM_VERSIONOK" == "OK"; then
	echo "ok" >/dev/null
	GDBM_COMMENT="${GDBM_VERSION} -> OK"
else
#	echo "not present or version mismatch"
	GDBM_COMMENT="${GDBM_VERSION} -> version mismatch"
	INSTALL_GDBM=1
	DEPENDENCY=1
fi

# compile a test program and extract the version information
#echo -n "Checking glibc ... "
cat >/var/tmp/glibc-test${PID}.c<<_ACEOF
#include <stdio.h>
#include <gnu/libc-version.h>
int main(void)
{
	puts(gnu_get_libc_version());
	return 0;
}
_ACEOF
$CC -o /var/tmp/glibc-test${PID} /var/tmp/glibc-test${PID}.c
GLIBC_VERSION=`/var/tmp/glibc-test${PID}`
CURRENT_GLIBC_VERSION=`/var/tmp/glibc-test${PID}`
rm -rf /var/tmp/glibc-test${PID} /var/tmp/glibc-test${PID}.c
GLIBC_VERSIONOK=`echo $CURRENT_GLIBC_VERSION 2>/dev/null | sed 's/\./ /g' | awk '{\
        if ( $1 > 2 ) {\
                print "OK"\
        }\
        if ( $1 == 2 ) {\
                        if ( $2 > 3 ) {\
                        print "OK";\
                }\
                if ( $2 == 3 ) {\
                        if ( $3 >= 2 ) {\
                                print "OK";
                        }\
                }\
        }\
     }'`\

if test "$GLIBC_VERSIONOK" == "OK"; then
	echo "ok" >/dev/null
	GLIBC_COMMENT="${GLIBC_VERSION} -> OK"
else
	GLIBC_COMMENT="${GLIBC_VERSION} -> version mismatch"
	INSTALL_GLIBC=1
	DEPENDENCY=1
fi

#echo -n "Checking net-snmp ... "
NETSNMP_VERSION=`$NET_SNMP_CONFIG --version 2>/dev/null `
NETSNMP_VERSIONOK=`$NET_SNMP_CONFIG --version 2>/dev/null | sed 's/\./ /g' \
| awk '{\
        if ( $1 > 5 ) {\
                print "OK";\
        }\
        if ( $1 == 5 ) {\
                if ( $2 >= 4 ) {\
                        print "OK";\
                }\
        }\
}'`\

if test "$NETSNMP_VERSIONOK" == "OK"; then

# Check if SNMP development is installed
	cat >conftest.c <<_ACEOF
/* end confdefs.h.  */

        #include <net-snmp/net-snmp-config.h>
        #include <net-snmp/net-snmp-includes.h>

int
main ()
{

        struct snmp_session session

  ;
  return 0;
}
_ACEOF

	gcc conftest.c 2> /dev/null
	if test $? != "0" ; then
        NETSNMP_COMMENT="development version not installed"
	INSTALL_NETSNMP=1
	DEPENDENCY=1
else
	echo "ok" >/dev/null
	NETSNMP_COMMENT="${NETSNMP_VERSION} -> OK"
fi
else
#	echo "version mismatch"
	WHICH_NETSNMP=`which net-snmp-config 2>/dev/null | wc -l `
	if [ $WHICH_NETSNMP -lt 1 ]; then
		NETSNMP_COMMENT="not installed"
	else
		NETSNMP_COMMENT="${NETSNMP_VERSION} -> version mismatch"
	fi
	INSTALL_NETSNMP=1
	DEPENDENCY=1
fi

# If we've already built it in a prior run, don't do it again
if [ "$INSTALL_NETSNMP" -eq 1 -a -e $PREFIX_BIN/net-snmp-config ]; then
    current=$($PREFIX_BIN/net-snmp-config --version)
    newest=`echo -e "$current\n5.4" | sort -r | head -n 1`
    if [ $current == $newest ]; then
        INSTALL_NETSNMP=0
        NETSNMP_COMMENT="Installed during a prior run of this installer"
    fi
fi

#echo -n "Checking openhpi ... "
IF_OPENHPI=`pkg-config --modversion openhpi 2>/dev/null | wc -l`
OPENHPI_VERSION=`pkg-config --modversion openhpi 2>/dev/null`
OPENHPI_VERSIONOK=`pkg-config --modversion openhpi 2>/dev/null| sed 's/\./ /g' | awk '{\
        if ( $1 > 2 ) {\
                print "OK";\
        }\
        if ( $1 == 2 ) {\
                if ( $2 > 8 ) {\
                        print "OK";\
                }\
                if ( $2 == 8 ) {\
                        if ( $3 >= 1 ) {\
                                print "OK";
                        }\
                }\
        }\
     }'`\

if test "$OPENHPI_VERSIONOK" == "OK"; then
	if [ "${INSTALL_NETSNMP}" = "1" ]; then
		OPENHPI_COMMENT="${OPENHPI_VERSION} -> needs (re)install"
		INSTALL_OPENHPI=1
		DEPENDENCY=1
	else
		OPENHPI_COMMENT="${OPENHPI_VERSION} -> OK"
	fi
else
#	echo "not present or version mismatch"
	if test $IF_OPENHPI -gt 0; then
		OPENHPI_COMMENT="${OPENHPI_VERSION} -> version mismatch"
	else
		OPENHPI_COMMENT="not installed"
	fi
	INSTALL_OPENHPI=1
	DEPENDENCY=1
fi

if test "$CUSTOM_OPENHPI" == "1"; then
    OPENHPI_COMMENT="${OPENHPI_VERSION} -> custom (re)install"
    INSTALL_OPENHPI=1
fi


# check the existance of the hpiSubagent in the sandbox
#echo -n "Checking openhpi-subagent ... "
if [ "${INSTALL_OPENHPI}" = 1 -o "${INSTALL_NETSNMP}" = "1" ]
then
    SUBAGENT_COMMENT="needs (re)install"
    INSTALL_SUBAGENT=1
    DEPENDENCY=1
elif  [ -x /usr/local/bin/hpiSubagent ]  ||  [ -x /usr/bin/hpiSubagent ]  ||  [ -x $PREFIX_BIN/hpiSubagent ];
then
	echo "ok" >/dev/null
	SUBAGENT_COMMENT="OK"
else
#	echo "not present or version mismatch"
	SUBAGENT_COMMENT="not installed"
	INSTALL_SUBAGENT=1
	DEPENDENCY=1
fi

# check for tipc module
/sbin/modinfo tipc > /dev/null 2>&1
if [ $? -eq 1 ]; then
    if [ "$(uname -r | cut -d. -f 3 | cut -d- -f 1)" -lt "15" ]; then
        INSTALL_TIPC=1
        TIPC_COMMENT="not installed"
        TIPC_MODULE_VERSION=1.5.12
    else
        echo "[ERROR] No TIPC module was found on this system, nor can one be installed"
        echo "        by this installer.  Please recompile your kernel with TIPC support"
        echo "        enabled as a kernel module and run this program again."
        cleanup
        exit 1
    fi
else
    INSTALL_TIPC=0
    TIPC_COMMENT="installed"

    # check for tipc-config
    WHICH_TIPC_CONFIG=$(which tipc-config 2>/dev/null | wc -l)
    if [ $WHICH_TIPC_CONFIG -lt 1 ]; then
        INSTALL_TIPC_CONFIG=1
        TIPC_CONFIG_COMMENT="not present"

        TIPC_MODULE_VERSION=$(/sbin/modinfo tipc | grep '^version' | tr -s " " | cut -d\  -f 2)
        TIPC_MAJOR_VERSION=$(echo $TIPC_MODULE_VERSION | cut -d. -f 1)
        TIPC_MINOR_VERSION=$(echo $TIPC_MODULE_VERSION | cut -d. -f 2)
        if [ ! $TIPC_MAJOR_VERSION -eq 1 ]; then
            INSTALL_TIPC_CONFIG=0
            TIPC_CONFIG_COMMENT="incompatible version, won't install"
        fi
        if [ $TIPC_MINOR_VERSION -eq 5 ]; then
            INSTALL_TIPC=1
            TIPC_COMMENT="installed"
            INSTALL_TIPC_CONFIG=0
            TIPC_CONFIG_COMMENT="not installed"
        elif [ $TIPC_MINOR_VERSION -eq 6 ]; then
            TIPCUTILS_VERSION=tipcutils-1.0.4
        elif [ $TIPC_MINOR_VERSION -eq 7 ]; then
            TIPCUTILS_VERSION=tipcutils-1.1.5
        fi
    else
        INSTALL_TIPC_CONFIG=0
        TIPC_CONFIG_COMMENT="installed"
    fi
fi

if [ $INSTALLIDE == "YES" ]; then
	echo
	echo "Checking IDE dependencies ..."
	if [ -f $PREFIX_BIN/python ]; then
		PYTHON=$PREFIX_BIN/python
	else
		PYTHON=python
	fi
	# Python writes the version information to STDERR
	#echo -n "Checking Python ... "
	PYTHON_VERSION=`$PYTHON -V 2>&1 | awk '{print $2}'`
	PYTHON_VERSIONOK=`$PYTHON -V 2>&1 | awk '{print $2}' | sed 's/\./ /g' \
	| awk '{\
		if ( $1 > 2 ) {\
			print "OK";\
		}\
		if ( $1 == 2 ) {\
			if ( $2 > 4 ) {\
				print "OK";\
			}\
                if ( $2 == 4 ) {\
				if ( $3 >= 1 ) {\
					print "OK";
				}\
			}\
		}\
	}'`\

	if test "$PYTHON_VERSIONOK" == "OK"; then
		echo "ok" >/dev/null
		PYTHON_COMMENT="${PYTHON_VERSION} -> OK"
	else
	#	echo "version mismatch"
		WHICH_PYTHON=`which python 2>/dev/null | wc -l`
		if [ $WHICH_PYTHON -lt 1 ]; then
			PYTHON_COMMENT="not installed"
		else
			PYTHON_COMMENT="${PYTHON_VERSION} -> version mismatch"
		fi
		INSTALL_PYTHON=1
		DEPENDENCY=1
	fi

	# java too writes the version information to STDERR
	if [ -f $PREFIX/jre1.6.0_21/bin/java ]; then
		JAVA=$PREFIX/jre1.6.0_21/bin/java
	else
		JAVA=java
	fi
	#echo -n "Checking JRE ... "
	JAVA_VERSION=`$JAVA -version 2>&1 | fgrep "java version" | awk '{print $NF}' | sed -e 's/"//g'`
	JAVA_VERSIONOK=`$JAVA -version 2>&1 | fgrep "java version" | awk '{print $NF}' | sed -e 's/"/ /g' -e 's/\./ /g'  -e 's/_/ /g' \
	| awk '{\
		if ( $1 > 1 ) {\
			print "OK";\
		}\
		if ( $1 == 1 ) {\
			if ( $2 > 5 ) {\
				print "OK";\
			}\
			if ( $2 == 5 ) {\
				if ( $3 >= 0 ) {\
					print "OK";
				}\
			}\
		}\
	}'`\

	JAVA_ARCH=`$JAVA -version 2>&1 | fgrep "64-Bit"`
	if test "$JAVA_ARCH" == ""; then
		JAVA_ARCH="32-Bit"
	else
		JAVA_ARCH="64-Bit"
		JAVA_VERSIONOK=""
	fi
	
        $JAVA -version > /dev/null 2>&1
	if [ $? -eq 0 ]; then
	    JAVA_GCJ=$($JAVA -version 2>&1 | fgrep "gcj")
	    if [ "$JAVA_GCJ" == "" ]; then
                if [ "$JAVA_VERSIONOK" == "OK" ]; then
	    	    JAVA_VERSIONOK="OK"
                else
                    JAVA_VERSIONOK=""
                fi
	    else
		    JAVA_VERSIONOK=""
	    fi
        else
            JAVA_VERSIONOK=""
        fi

	if test "$JAVA_VERSIONOK" == "OK"; then
		echo "ok" >/dev/null
		JAVA_COMMENT="${JAVA_VERSION} -> OK"
	else
	#	echo "version mismatch"
		WHICH_JAVA=`which java 2>/dev/null | wc -l`
		if [ $WHICH_JAVA -lt 1 ]; then
			JAVA_COMMENT="not installed"
		else
			JAVA_COMMENT="${JAVA_VERSION} ${JAVA_ARCH} -> version mismatch"
		fi
		INSTALL_JRE=1
		DEPENDENCY=1
	fi

	if [ -d $PREFIX/eclipse ]; then
	#	echo -n "Checking Eclipse ... "
		# This will work in readme directory
		ECLIPSE_VERSION=`grep "Release Notes" $PREFIX/eclipse/readme/readme_eclipse.html | grep "title" | awk '{print $NF}' | sed -e 's/<\/title>/ /'`
		ECLIPSE_VERSIONOK=`grep "Release Notes" $PREFIX/eclipse/readme/readme_eclipse.html | grep "title" | awk '{print $NF}' | sed -e 's/<\/title>/ /' -e 's/\./ /' \
	| awk '{\
		if ( $1 > 3 ) {\
			print "OK";\
		}\
		if ( $1 == 3 ) {\
			if ( $2 > 3 ) {\
				print "OK";\
			}\
			if ( $2 == 3 ) {\
				if ( $3 >= 2 ) {\
					print "OK";
				}\
			}\
		}\
	}'`\

	# the only source to get the version information of 
	# eclipse SDK is from the readme_eclipse.html
	# so parse it and get the information.
	# For the GEF and EMF runtime plugins the only way
	# to find the version information is ls -l ;)
	if test "$ECLIPSE_VERSIONOK" == "OK"; then
		echo "ok" >/dev/null
		ECLIPSE_COMMENT="${ECLIPSE_VERSION} -> OK"
	else
	#	echo "not present"
		ECLIPSE_COMMENT="${ECLIPSE_VERSION} -> version mismatch"
		INSTALL_ECLIPSE=1
		DEPENDENCY=1
	fi

	#	echo -n "Checking GEF Runtime ... " 
		IF_GEF=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.gef_" | awk -F_ '{print $NF}' | sort | uniq | wc -l | awk '{print $0}'`
		if test $IF_GEF -ge 1; then
			GEF_VERSION=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.gef_" | awk -F_ '{print $NF}' | sort | uniq | tail -n 1`
			GEF_VERSIONOK=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.gef_" | awk -F_ '{print $NF}' | sort | uniq | tail -n 1 | sed 's/\./ /g' \
	| awk '{\
		if ( $1 > 3 ) {\
			print "OK";\
		}\
		if ( $1 == 3 ) {\
			if ( $2 > 3 ) {\
				print "OK";\
			}\
			if ( $2 == 3 ) {\
				if ( $3 >= 1 ) {\
					print "OK";
				}\
			}\
		}\
	}'`\

			if test "$GEF_VERSIONOK" == "OK"; then
				echo "ok" >/dev/null
				GEF_COMMENT="${GEF_VERSION} -> OK"
			else
	#			echo "version mismatch"
				GEF_COMMENT="${GEF_VERSION} -> version mismatch"
				INSTALL_GEF=1
				DEPENDENCY=1
			fi
		else
		#		echo "not present"
			GEF_COMMENT="not installed"
			INSTALL_GEF=1
			DEPENDENCY=1
		fi

		# This will work in features directory
	#	echo -n "Checking EMF Runtime ... "
		IF_EMF=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.emf_" | awk -F_ '{print $NF}' | sort | uniq | wc -l | awk '{print $0}'`
		if test $IF_EMF -ge 1; then
			EMF_VERSION=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.emf_" | awk -F_ '{print $NF}' | sort | uniq | tail -n 1`
			EMF_VERSIONOK=`ls -1 $PREFIX/eclipse/features | egrep "org.eclipse.emf_" | awk -F_ '{print $NF}' | sort | uniq | tail -n 1 | sed 's/\./ /g' \
	| awk '{\
		if ( $1 > 2 ) {\
			print "OK";\
		}\
		if ( $1 == 2 ) {\
			if ( $2 > 3 ) {\
				print "OK";\
			}\
			if ( $2 == 3 ) {\
				if ( $3 >= 2 ) {\
					print "OK";
				}\
			}\
		}\
	}'`\

			if test "$EMF_VERSIONOK" == "OK"; then
				echo "ok" >/dev/null
				EMF_COMMENT="${EMF_VERSION} -> OK"
			else
			#	echo "version mismatch"
				EMF_COMMENT="${EMF_VERSION} -> version mismatch"
				INSTALL_EMF=1
				DEPENDENCY=1
			fi
		else
			#echo "not present"
			EMF_COMMENT="not installed"
			INSTALL_EMF=1
			DEPENDENCY=1
		fi

                # Checking for CDT
                CDT_VERSION=$(grep 'cdt.*version' $PREFIX/eclipse/cdt/site.xml 2>/dev/null | head -n 1 | awk -F= '{print $NF}' | sed -e 's/"//g' | sed -e 's/>//g')
                if [ "$CDT_VERSION" == "" ]; then
                    CDT_COMMENT="not installed"
                    INSTALL_CDT=1
                else
                    CDT_VERSIONOK=$(echo CDT_VERSION | sed -e 's/\./ /g' | awk '{ \
                        if ( $1 > 4 ) { \
                            print "OK"; \
                        } \
                        if ( $1 == 4 ) { \
                            if ( $2 > 0 ) { \
                                print "OK"; \
                            }
                            if ( $2 == 0 ) { \
                                if ( $3 >= 3 ) { \
                                    print "OK"; \
                                } \
                            } \
                        } \
                    }')
                    
                    if [ "$CDT_VERSIONOK" == "OK" ]; then
                        CDT_COMMENT="$CDT_VERSION -> OK"
                    else
                        CDT_COMMENT="$CDT_VERSION -> version mismatch"
                        INSTALL_CDT=1
                    fi
                fi

	else
	#	echo "Eclipse ... not present"
		INSTALL_ECLIPSE=1
		INSTALL_GEF=1
		INSTALL_EMF=1
                INSTALL_CDT=1
		DEPENDENCY=1
		EMF_COMMENT="not installed"
		GEF_COMMENT="not installed"
		ECLIPSE_COMMENT="not installed"
	fi
fi

if [ -d $WORKING_DIR/log ]; then
	echo
	echo -n "Installer log directory ($WORKING_DIR/log) exists. Overwrite? <y|n> [y]: "
	read log_remove dummy
	if [ -z $log_remove ]; then
		log_remove='y';
	fi
	case $log_remove in
		Y|y)
			rm -rf $WORKING_DIR/log
			mkdir $WORKING_DIR/log
			;;
		N|n)
			mv $WORKING_DIR/log $WORKING_DIR/log.`date "+%F-%H-%M"`
			mkdir $WORKING_DIR/log
			;;
		*)
			echo "Invalid option, considering as y" >&2
			rm -rf $WORKING_DIR/log
			mkdir $WORKING_DIR/log
	esac
else
	mkdir $WORKING_DIR/log
fi

if [ ! -d $BUILD_DIR ]; then
	mkdir -p $BUILD_DIR
fi

# we are now done with checking the dependencies
# install only required packages
count=0
if test $DEPENDENCY -eq 1; then

	tput clear
	echo "The following packages are available on your system:"
	echo
	printf "%17s  %-9s  %-16s\n" "-----------------" "---------" "----------------"
	printf "%17s  %-9s  %-16s\n" "Package" "Required" "Status"
	printf "%17s  %-9s  %-16s\n" "" "Version" ""
	printf "%17s  %-9s  %-16s\n" "-----------------" "---------" "----------------"
	if test $INSTALL_GCC -eq 0; then
		printf "%17s  %-9s  %-16s\n" "gcc" "3.2.3" "$GCC_COMMENT"
	fi
	if test $INSTALL_GLIB -eq 0; then
		printf "%17s  %-9s  %-16s\n" "glib-2.0" "2.12.6" "$GLIB_COMMENT"
	fi
	if test $INSTALL_GDBM -eq 0; then
		printf "%17s  %-9s  %-16s\n" "gdbm" "1.8.0" "$GDBM_COMMENT"
	fi
	if test $INSTALL_GLIBC -eq 0; then
		printf "%17s  %-9s  %-16s\n" "glibc" "2.3.2" "$GLIBC_COMMENT"
	fi
	if test $INSTALL_NETSNMP -eq 0; then
		printf "%17s  %-9s  %-16s\n" "net-snmp" "5.4.2" "$NETSNMP_COMMENT"
	fi
	if test $INSTALL_OPENHPI -eq 0; then
		printf "%17s  %-9s  %-16s\n" "openhpi" "2.8.1" "$OPENHPI_COMMENT"
	fi
	if test $INSTALL_SUBAGENT -eq 0; then
		printf "%17s  %-9s  %-16s\n" "openhpi-subagent" "2.3.4" "$SUBAGENT_COMMENT"
	fi
    if test $INSTALL_TIPC -eq 0; then
		printf "%17s  %-9s  %-16s\n" "tipc" "$TIPC_MODULE_VERSION" "$TIPC_COMMENT"
    fi
    if test $INSTALL_TIPC_CONFIG -eq 0; then
		printf "%17s  %-9s  %-16s\n" "tipc-config" "n/a" "$TIPC_CONFIG_COMMENT"
    fi
	if test $INSTALLIDE == "YES"; then
		if test $INSTALL_PYTHON -eq 0; then
			printf "%17s  %-9s  %-16s\n" "Python" "2.4.1" "$PYTHON_COMMENT"
		fi
		if test $INSTALL_JRE -eq 0; then
			printf "%17s  %-9s  %-16s\n" "JRE" "1.6.0_21" "$JAVA_COMMENT"
		fi
		if test $INSTALL_ECLIPSE -eq 0; then
			printf "%17s  %-9s  %-16s\n" "Eclipse SDK" "3.3.2" "$ECLIPSE_COMMENT"
		fi
		if test $INSTALL_EMF -eq 0; then
			printf "%17s  %-9s  %-16s\n" "EMF Runtime" "2.3.2" "$EMF_COMMENT"
		fi
		if test $INSTALL_GEF -eq 0; then
			printf "%17s  %-9s  %-16s\n" "GEF Runtime" "3.3.1" "$GEF_COMMENT"
		fi
                if test $INSTALL_CDT -eq 0; then
			printf "%17s  %-9s  %-16s\n" "CDT" "4.0.3" "$CDT_COMMENT"
                fi
	fi
	printf "%17s  %-9s  %-16s\n" "-----------------" "---------" "----------------"
	printf "\n\n"

	echo "The following packages will be installed at"
	echo "	$PREFIX"
	printf "%17s  %-9s  %-16s\n" "-----------------" "---------" "----------------"
	printf "%17s  %-9s  %-16s\n" "Package" "Required" "Status"
	printf "%17s  %-9s  %-16s\n" "" "Version" ""
	printf "%17s  %-9s  %-16s\n" "-----------------" "---------" "----------------"
	if test $INSTALL_GCC -eq 1; then
		printf "%17s  %-9s  %-16s\n" "gcc" "3.2.3" "$GCC_COMMENT"
	fi
	if test $INSTALL_GLIB -eq 1; then
		printf "%17s  %-9s  %-16s\n" "glib-2.0" "2.12.6" "$GLIB_COMMENT"
	fi
	if test $INSTALL_GDBM -eq 1; then
		printf "%17s  %-9s  %-16s\n" "gdbm" "1.8.0" "$GDBM_COMMENT"
	fi
	if test $INSTALL_GLIBC -eq 1; then
		printf "%17s  %-9s  %-16s\n" "glibc" "2.3.5" "$GLIBC_COMMENT"
	fi
	if test $INSTALL_NETSNMP -eq 1; then
		printf "%17s  %-9s  %-16s\n" "net-snmp" "5.4.2" "$NETSNMP_COMMENT"
	fi
	if test $INSTALL_OPENHPI -eq 1; then
		printf "%17s  %-9s  %-16s\n" "openhpi" "$OPENHPI_PKG_VER" "$OPENHPI_COMMENT"
	fi
	if test $INSTALL_SUBAGENT -eq 1; then
		printf "%17s  %-9s  %-16s\n" "openhpi-subagent" "2.3.4" "$SUBAGENT_COMMENT"
	fi
    if test $INSTALL_TIPC -eq 1; then
		printf "%17s  %-9s  %-16s\n" "tipc" "$TIPC_MODULE_VERSION" "$TIPC_COMMENT"
    fi
    if test $INSTALL_TIPC_CONFIG -eq 1; then
		printf "%17s  %-9s  %-16s\n" "tipc-config" "n/a" "$TIPC_CONFIG_COMMENT"
    fi
	if test $INSTALL_PYTHON -eq 1; then
		printf "%17s  %-9s  %-16s\n" "Python" "2.4.1" "$PYTHON_COMMENT"
	fi
	if test $INSTALL_JRE -eq 1; then
		printf "%17s  %-9s  %-16s\n" "JRE" "1.6.0_21" "$JAVA_COMMENT"
	fi
	if test $INSTALL_ECLIPSE -eq 1; then
		printf "%17s  %-9s  %-16s\n" "Eclipse SDK" "3.3.2" "$ECLIPSE_COMMENT"
	fi
	if test $INSTALL_EMF -eq 1; then
		printf "%17s  %-9s  %-16s\n" "EMF Runtime" "2.3.2" "$EMF_COMMENT"
	fi
	if test $INSTALL_GEF -eq 1; then
		printf "%17s  %-9s  %-16s\n" "GEF Runtime" "3.3.1" "$GEF_COMMENT"
	fi
	if test $INSTALL_CDT -eq 1; then
		printf "%17s  %-9s  %-16s\n" "CDT Runtime" "4.0.3" "$CDT_COMMENT"
	fi
	printf "%17s  %-9s  %-16s\n\n" "-----------------" "---------" "----------------"

	printf "\nProceed with the installation <y|n> [y]: "
	read install dummy

	if [ -z $install ]; then
		install='y'
	fi

	case $install in
		N|n)
			tput cnorm
			cleanup
			exit
			;;
		Y|y)
			;;
		*)
			tput cnorm
			printf "Invalid option ... exiting\n"
			cleanup
			exit
			;;
	esac

	tput clear
	printf "Installing ${blue_bold}OpenClovis ${blue}Prerequisites\n\n"
	printf "%23s  %-17s  %-17s\n" "-----------------------" "-----------------" "-----------------"
	printf "%23s  %-17s  %-17s\n" "Package" "Installation Time" "Installation"
	printf "%23s  %-17s  %-17s\n" "" "(approx in min)" "Status"
	printf "%23s  %-17s  %-17s\n" "-----------------------" "-----------------" "-----------------"
	if test $INSTALL_GCC -eq 1; then
		printf "%23s  %-17s  %-17s\n" "gcc V3.2.3" " 4" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_GLIB -eq 1; then
		printf "%23s  %-17s  %-17s\n" "glib-2.0 V2.12.6" " 1.5" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_GDBM -eq 1; then
		printf "%23s  %-17s  %-17s\n" "gdbm V1.8.0" " < 1" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_GLIBC -eq 1; then
		printf "%23s  %-17s  %-17s\n" "glibc V2.3.5" " 13" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_NETSNMP -eq 1; then
		printf "%23s  %-17s  %-17s\n" "net-snmp V5.4.2" " 4.5" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_OPENHPI -eq 1; then
		printf "%23s  %-17s  %-17s\n" "openhpi V$OPENHPI_PKG_VER" " 4.5" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_SUBAGENT -eq 1; then
		printf "%23s  %-17s  %-17s\n" "openhpi-subagent V2.3.4" " < 1" "Pending"
		count=`expr $count + 1`
	fi
    if test $INSTALL_TIPC -eq 1; then
		printf "%23s  %-17s  %-17s\n" "tipc V1.5.12" " < 1" "Pending"
		count=`expr $count + 1`
    fi
    if test $INSTALL_TIPC_CONFIG -eq 1; then
		printf "%23s  %-17s  %-17s\n" "$TIPCUTILS_VERSION" " < 1" "Pending"
		count=`expr $count + 1`
    fi
	if test $INSTALL_PYTHON -eq 1; then
		printf "%23s  %-17s  %-17s\n" "Python V2.4.1" " 2.5" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_JRE -eq 1; then
		printf "%23s  %-17s  %-17s\n" "JRE V1.6.0_21" " < 1" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_ECLIPSE -eq 1; then
		printf "%23s  %-17s  %-17s\n" "Eclipse SDK V3.3.2" " < 1" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_EMF -eq 1; then
		printf "%23s  %-17s  %-17s\n" "EMF Runtime V2.3.2" " < 1" "Pending"
		count=`expr $count + 1`
	fi
	if test $INSTALL_GEF -eq 1; then
		printf "%23s  %-17s  %-17s\n" "GEF Runtime V3.3.1" " < 1" "Pending"
		count=`expr $count + 1`
	fi
        if test $INSTALL_CDT -eq 1; then
		printf "%23s  %-17s  %-17s\n" "CDT V4.0.3" " < 1" "Pending"
		count=`expr $count + 1`
        fi
	printf "%23s  %-17s  %-17s\n" "-----------------------" "-----------------" "-----------------"

fi

row=6
tput civis
ROWS=`tput lines`
if test $INSTALL_GCC -eq 1; then
	#echo -n "Installing gcc 3.2.3 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG gcc-3.2.3.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf gcc-3.2.3.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f gcc-3.2.3.tar.gz
	#echo -n "."
	cd gcc-3.2.3
	mkdir build
	cd build
	# got the following GCC configure options from the
	# existing gcc installation.
	tput cup $row 44
	printf "%-17s" 'Configuring'
	../configure \
		--prefix=$PREFIX \
		--enable-shared \
		--enable-threads=posix \
		--disable-checking \
		--with-system-zlib \
		--enable-__cxa_atexit \
		--disable-libunwind-exceptions \
		--enable-java-awt=gtk \
		--enable-languages=C,C++,Java \
		>> $WORKING_DIR/log/gcc.3.2.3.log 2>&1   || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/gcc.3.2.3.log 2>&1   || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/gcc.3.2.3.log 2>&1   || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	cd ../..
	rm -rf gcc-3.2.3
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	#echo "done"
	row=`expr $row + 1`
fi

if test $INSTALL_GLIB -eq 1; then
	#echo -n "Installing glib 2.6.0 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG glib-2.12.6.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf glib-*.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f glib-*.tar.gz
	cd glib-*
	#echo -n "."
	tput cup $row 44
	printf "%-17s" 'Configuring'
	./configure --prefix=$PREFIX >> $WORKING_DIR/log/glib.log 2>&1   || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/glib.log 2>&1   || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/glib.log 2>&1   || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf glib-*
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if test $INSTALL_GDBM -eq 1; then
	#echo -n "Installing gdbm 1.8.0 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG gdbm-1.8.0.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf gdbm-1.8.0.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f gdbm-1.8.0.tar.gz	
	cd gdbm-1.8.0
	#echo -n "."
	# prefix is hard coded in gdbm
	sed -i  -e s#usr\/local#$PREFIX# Makefile.in 
	tput cup $row 44
	printf "%-17s" 'Configuring'
	if [ $PROCESSOR != "i686" ]; then
		# ia64-linux-gnu is not recognized as a valid host
		./configure --host=i686-linux-gnu --prefix=$PREFIX >> $WORKING_DIR/log/gdbm-1.8.0.log 2>&1 || myexit $row $CONFIGURE_ERROR &
	else
		./configure --prefix=$PREFIX >> $WORKING_DIR/log/gdbm-1.8.0.log 2>&1   || myexit $row $CONFIGURE_ERROR &
	fi
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/gdbm-1.8.0.log 2>&1   || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	# Need to overwrite default 'bin' owner and group assumed by the makefile
	# so that it works if this is a non-root install
	if [ $ID -ne 0 ]; then
		make BINOWN=$USER BINGRP=$GROUPS install >> $WORKING_DIR/log/gdbm-1.8.0.log 2>&1   || myexit $row $MAKE_INSTALL_ERROR &
	else
		make install >> $WORKING_DIR/log/gdbm-1.8.0.log 2>&1   || myexit $row $MAKE_INSTALL_ERROR &
	fi
	PS=$!
	roll $PS
	printf "\b"

	rm -rf gdbm-1.8.0
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
	cd ..
fi


if test $INSTALL_GLIBC -eq 1; then
	#echo -n "Installing glibc 2.3.5 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG glibc-2.3.5.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf glibc-2.3.5.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f glibc-2.3.5.tar.gz
	#echo -n "."
	cd glibc-2.3.5
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG glibc-linuxthreads-2.3.5.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf glibc-linuxthreads-2.3.5.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f glibc-linuxthreads-2.3.5.tar.gz
	#echo -n "."
	mkdir build
	cd build
	# the following configure options are from one of the
	# best practices published in the internet ;)
	tput cup $row 44
	printf "%-17s" 'Configuring'
	../configure \
		--prefix=$PREFIX \
		--build=i686-pc-linux-gnu \
		--enable-add-ons=linuxthreads \
		--disable-profile \
		--without-cvs \
		--without-__thread \
		--without-sanity-check \
		>>$WORKING_DIR/log/glibc-2.3.5.log 2>&1 || myexit $row $CONFIGURE_ERROR & 
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >>$WORKING_DIR/log/glibc-2.3.5.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >>$WORKING_DIR/log/glibc-2.3.5.log 2>&1 || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	cd ../..
	rm -rf glibc-2.3.5
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if test $INSTALL_NETSNMP -eq 1; then

	if [ ! -f /usr/lib/libelf.so ]; then # link is missing, let us try to link it in .
		ln -fs /usr/lib/libelf.so.1 libelf.so
	fi

	export LD_LIBRARY_PATH=$PREFIX_LIB:$LD_LIBRARY_PATH
	#echo -n "Installing net-snmp 5.4.2 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG net-snmp-5.4.2.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf net-snmp-*.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f net-snmp-*.tar.gz
	#echo -n "."
	cd net-snmp-*
	tput cup $row 44
	printf "%-17s" 'Configuring'
	./configure \
		--prefix=$PREFIX \
		--with-default-snmp-version="2" \
		--without-rpm \
		--with-defaults \
		--with-perl-modules=PREFIX=$PREFIX \
		>> $WORKING_DIR/log/net-snmp.log 2>&1 || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/net-snmp.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/net-snmp.log 2>&1 || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf net-snmp-*
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if test $INSTALL_OPENHPI -eq 1; then
	#echo -n "Installing openhpi $OPENHPI_PKG_VER"
	tput cup $row 44
	printf "%-17s" 'Extracting'
	echo cd $PREFIX >> $WORKING_DIR/log/openhpi.log
	cd $PREFIX # keep the openhpi sources as they are required to bring up CM
        rm -fr openhpi-* # need to remove old tree so that patch will work
        if [ "$CUSTOM_OPENHPI" == "1" ]; then
	    tar zxf $OPENHPI_PKG &
	    PS=$!
	    roll $PS
	    printf "\b"
        else
	    echo tar xfm $WORKING_ROOT/$THIRDPARTYPKG $OPENHPI_PKG >> $WORKING_DIR/log/openhpi.log
	    tar xfm $WORKING_ROOT/$THIRDPARTYPKG $OPENHPI_PKG & 
	    PS=$!
	    roll $PS
	    printf "\b"
	    tar zxf $OPENHPI_PKG &
	    PS=$!
	    roll $PS
	    printf "\b"
        fi
	rm -f openhpi-*.tar.gz
	#echo -n "."
	cd openhpi-[0-9]*
	chown -R $USER . 
#   zcat ../openhpi-2.8.1-pps-x86.patch.gz | patch -p1 \
#       >> $WORKING_DIR/log/openhpi.log 2>&1 || myexit $row $PATCH_ERROR
	# compilation on ia64 2.6.9-5 will fail with these flags
	# OpenHPI build on MontaVista will fail without these flags
	GCC_OK=`gcc -dumpversion | awk -F. '{print $1}'`
	grep MontaVista /etc/issue >/dev/null 2>&1
        # not needed for 2.8.0
#	if [ $? -ne 0 ]; then
#		sed -i.bak -e '/no-strict-aliasing/d' -e '/cast-align/d' configure
#	fi
	# GCC 4.1.1 requires all flags
#	if [ $GCC_OK -ge 4 ]; then # revert back changes in configure script
#		if [ -f configure.bak ]; then
#			mv -f configure.bak configure
#		fi
#	fi
	chmod +x configure
	tput cup $row 44
	printf "%-17s" 'Configuring'
	# build openhpi with out daemon if the user doesn't have write permission on /etc
	pwd >> $WORKING_DIR/log/openhpi.log
	./configure \
		--prefix=$PREFIX \
		--with-varpath=$PREFIX/var/lib/openhpi \
		>> $WORKING_DIR/log/openhpi.log 2>&1 || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

    #
    # and now a grotesque bit of kludgery.  In openhpi's snmp plugins we #include
    # <config.h> and <net-snmp/net-snmp-config.h>.  Both of those files define
    # several preprocessor symbols with conflicting values.  That is, there are
    # several preprocessor symbols which are defined in both header files.
    # This causes the build of the openhpi snmp plugin to fail.
    # Our kludge is to remove those definitions from our config.h file.
#	sed -i -e '/#define.*PACKAGEBUGREPORT/d' \
#            -e '/#define.*PACKAGE_NAME/d' \
#            -e '/#define.*PACKAGE_STRING/d' \
#            -e '/#define.*PACKAGE_TARNAME/d' \
#            -e '/#define.*PACKAGE_BUGREPORT/d'  \
#            -e '/#define.*PACKAGE_VERSION/d' config.h

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make  >> $WORKING_DIR/log/openhpi.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"
    
	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/openhpi.log 2>&1 || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"
    # This is a workaround to a openhpi pacth bug of Pigeon Point. They added
    # an additional file needed by oh_utils.h, but missed to add it to the list
    # of headers to be installed. Here we install it manually.
    #cp utils/wrappers.h $PREFIX/include/openhpi

	cd ..
	tput cup $row 44
	cd $WORKING_DIR
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if [ $INSTALL_SUBAGENT -eq 1 ]; then
	#echo -n "Installing openhpi-subagent 2.3.4 "
	cd $BUILD_DIR
	tput cup $row 44
        # echo export PKG_CONFIG_PATH=$BUILDTOOLS/local/lib/pkgconfig:$PKG_CONFIG_PATH
        export PKG_CONFIG_PATH=$BUILDTOOLS/local/lib/pkgconfig:$PKG_CONFIG_PATH
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG openhpi-subagent-2.3.4.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf openhpi-subagent-2.3.4.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f openhpi-subagent-2.3.4.tar.gz
	#echo -n "."
	cd openhpi-subagent-2.3.4
	tput cup $row 44
	printf "%-17s" 'Configuring'
#		cat configure | perl -n -e \
#			"if ( $. == 4621 ) \
#			{ \
#				print $_; \
#				print '	ac_link=\'\$CC -o conftest\$ac_exeext \`net-snmp-config --cflags\` \`net-snmp-config --libs\` \$CFLAGS \$CPPFLAGS \$LDFLAGS conftest.\$ac_ext \$LIBS >&5\'
#				'; \
#			} else { \
#				print $_;\
#			}" > configure.new
#		mv -f configure.new configure
#		chmod +x configure
	# let's configure 
        pwd >> $WORKING_DIR/log/openhpi-subagent.log 2>&1
        echo "./configure --prefix=$PREFIX CFLAGS=-I$BUILDTOOLS/local/include >> $WORKING_DIR/log/openhpi-subagent.log 2>&1  || myexit $row $CONFIGURE_ERROR &" >> $WORKING_DIR/log/openhpi-subagent.log 2>&1
	./configure --prefix=$PREFIX CFLAGS=-I$BUILDTOOLS/local/include >> $WORKING_DIR/log/openhpi-subagent.log 2>&1  || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

        sed --in-place -e "s;\$(shell net-snmp-config --prefix);$PREFIX;g" Makefile

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/openhpi-subagent.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/openhpi-subagent.log 2>&1 || myexit $row $MAKE_INSTALL_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf openhpi-subagent-2.3.4
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if [ $INSTALL_TIPC -eq 1 ]; then
    export KERNELDIR=/lib/modules/$(uname -r)/build
	
	#echo -n "Installing tipc-1.5.12 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG tipc-1.5.12.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf tipc-1.5.12.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f tipc-1.5.12.tar.gz
	#echo -n "."
	cd tipc-*
	tput cup $row 44

	tput cup $row 44
	printf "%-17s" 'Compiling'
    export KERNELDIR=/lib/modules/$(uname -r)/build
	make >> $WORKING_DIR/log/tipc.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
    # copying tipc module
    if [ ! -d $PREFIX/modules ]; then
        mkdir -p $PREFIX/modules
    fi
    # copying module
    if [ -f net/tipc/tipc.ko ]; then
        cp net/tipc/tipc.ko $PREFIX/modules
    else
        TIPC_ERROR="tipc kernel module not built."
        myexit $row $TIPC_ERROR
    fi
    # copying tipc-config
    if [ ! -d $PREFIX/bin ]; then
        mkdir -p $PREFIX/bin
    fi

    if [ -f tools/tipc-config ]; then
        cp tools/tipc-config $PREFIX/bin
    else
        TIPC_ERROR="tipc-config not built."
        myexit $row $TIPC_ERROR
    fi
    # copying tipc headers
    KERNEL_PATCH_VERSION=$(uname -r | cut -d. -f 3 | cut -d- -f 1)
    if [ $KERNEL_PATCH_VERSION -lt 16 ]; then
        cp include/net/tipc/*.h $PREFIX/include
    else
        mkdir -p $PREFIX/include/linux >/dev/null 2>&1
        cp include/net/tipc/*.h $PREFIX/include
        cp include/net/tipc/*.h $PREFIX/include/linux
    fi
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf tipc-*
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if [ $INSTALL_TIPC_CONFIG -eq 1 ]; then
    export KERNELDIR=/lib/modules/$(uname -r)/build

	#echo -n "Installing tipc-config"
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG ${TIPCUTILS_VERSION}.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf ${TIPCUTILS_VERSION}.tar.gz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f ${TIPCUTILS_VERSION}.tar.gz
	#echo -n "."
	cd tipcutils-*
	tput cup $row 44

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/tipc-config.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
    # copying tipc-config
    if [ ! -d $PREFIX/bin ]; then
        mkdir -p $PREFIX/bin
    fi

    if [ -f tipc-config ]; then
        cp tipc-config $PREFIX/bin
    else
        TIPC_ERROR="tipc-config not built."
        myexit $row $TIPC_ERROR
    fi
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf tipcutils-*
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if [ $INSTALL_PYTHON -eq 1 ]; then
	#echo -n "Installing Python 2.4.1 "
	cd $BUILD_DIR
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar xfm $WORKING_ROOT/$THIRDPARTYPKG Python-2.4.1.tgz &
	PS=$!
	roll $PS
	printf "\b"
	tar zxf Python-2.4.1.tgz &
	PS=$!
	roll $PS
	printf "\b"
	rm -f Python-2.4.1.tgz
	#echo -n "."
	cd Python-2.4.1
	tput cup $row 44
	printf "%-17s" 'Configuring'
	./configure --enable-shared --prefix=$PREFIX >> $WORKING_DIR/log/Python.log || myexit $row $CONFIGURE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Compiling'
	make >> $WORKING_DIR/log/Python.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"

	tput cup $row 44
	printf "%-17s" 'Installing'
	make install >> $WORKING_DIR/log/Python.log 2>&1 || myexit $row $MAKE_INSTALL_ERROR &
        cp libpython2.4.a $PREFIX/lib
	PS=$!
	roll $PS
	printf "\b"

	cd ..
	rm -rf Python-2.4.1
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	#echo "done"
fi

if [ $INSTALL_JRE -eq 1 ]; then
	#echo -n "Installing JRE 1.6.0_21 ... "
	cd $PREFIX
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar vxf $WORKING_ROOT/$THIRDPARTYPKG jre1.6.0_21.tar.gz \
		> $WORKING_DIR/log/jre1.6.0_21.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Copying'
	tar zvxf jre1.6.0_21.tar.gz \
		>> $WORKING_DIR/log/jre1.6.0_21.log 2>&1 || myexit $row $MAKE_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	rm -f jre1.6.0_21.tar.gz
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
fi
cd $WORKING_DIR

if [ $INSTALL_ECLIPSE -eq 1 ]; then
	#echo -n "Installing Eclipse SDK 3.3.2 ... "
	cd $PREFIX
	# remove older version of eclipse if any; this will remove the EMF and GED sdk also
	[ -d eclipse ] && rm -rf eclipse >/dev/null 2>&1
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar vxf $WORKING_ROOT/$THIRDPARTYPKG eclipse-SDK-3.3.2-linux-gtk.tar.gz \
		>$WORKING_DIR/log/eclipse.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Copying'
	tar zvxf eclipse-SDK-3.3.2-linux-gtk.tar.gz \
		>>$WORKING_DIR/log/eclipse.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	rm -f eclipse-SDK-3.3.2-linux-gtk.tar.gz
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
fi

if [ $INSTALL_EMF -eq 1 ]; then
	#echo -n "Installing EMF Runtime 2.3.2 ... "
	cd $PREFIX
	tput cup $row 44
	printf "%-17s" 'Extracting'
	tar vxf $WORKING_ROOT/$THIRDPARTYPKG emf-sdo-runtime-2.3.2.zip \
		>>$WORKING_DIR/log/emf-sdo-runtime.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Copying'
#	tar zvxf emf-sdo-runtime-2.3.2.tar.gz 
        
	yes | unzip emf-sdo-runtime-2.3.2.zip >>$WORKING_DIR/log/emf-sdo-runtime.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	rm -f emf-sdo-runtime-2.3.2.zip
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
fi

if [ $INSTALL_GEF -eq 1 ]; then
	tput cup $row 44
	printf "%-17s" 'Extracting'
	#echo -n "Installing GEF Runtime 3.3.1 ... "
	cd $PREFIX
	tar vxf $WORKING_ROOT/$THIRDPARTYPKG GEF-runtime-3.3.1.zip \
		>>$WORKING_DIR/log/GEF-runtime.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Copying'
	yes | unzip GEF-runtime-3.3.1.zip >> $WORKING_DIR/log/GEF-runtime.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
	rm -f GEF-runtime-3.3.1.zip
fi

if [ $INSTALL_CDT -eq 1 ]; then
	tput cup $row 44
	printf "%-17s" 'Extracting'
	cd $PREFIX
	tar vxf $WORKING_ROOT/$THIRDPARTYPKG cdt-master-4.0.3.zip \
		>>$WORKING_DIR/log/CDT.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Installing'
        mkdir -p eclipse/cdt/eclipse || myexit $row "Failed to create directory $PREFIX/eclipse/cdt/eclipse"
        mkdir -p eclipse/links || myexit $row "Failed to create directory $PREFIX/eclipse/links"
        cd eclipse/cdt/eclipse
	yes | unzip $PREFIX/cdt-master-4.0.3.zip >> $WORKING_DIR/log/CDT.log 2>&1 || myexit $row $EXTRACT_ERROR &
	PS=$!
	roll $PS
        echo "path=$PREFIX/eclipse/cdt" > $PREFIX/eclipse/links/cdt.link || myexit $row "Failed to create $PREFIX/eclipse/links/cdt.link"
	printf "\b"
	tput cup $row 44
	printf "%-17s" 'Installed'
	tput el
	row=`expr $row + 1`
        cd $PREFIX
	rm -f cdt-master-4.0.3.zip
fi

row=`expr $row + 2`
echo
echo
echo "All SDK dependencies are now in place."
echo
tput cnorm
sleep 2; tput clear
# see if the user downloaded any crossbuild tools
# checking for crossbuild-*.tar.gz; there might be many
# ASSUMPTION:
# for every crossbuild-*.tar.gz there exists a crossbuild-*.md5
# + which contains the md5 checksum
echo "Now we check if there are any Clovis-provided crossbuild toolchains in"
echo "$WORKING_ROOT to install."
echo "These allow you to build OpenClovis ASP for platforms other than this"
echo "development host."
ls -1 $WORKING_ROOT/crossbuild-*.tar.gz >/dev/null 2>&1
if [ $? -eq 0 ]; then # crossbuild tools are downloaded

	declare -a toolchain
	declare -a initial

	i=0
	cd $WORKING_ROOT; #ls -1 crossbuild-*.tar.gz | awk '{print "      ", $1}'
	for tool in $(ls -1 crossbuild-*.tar.gz)
	do
		initial[$i]=$tool
		i=$(expr $i + 1)
	done

	echo
	echo "The following crossbuild tool packages are found:"
	echo ""

	for ini in "${initial[@]}"
	do
		
		# derive dirname from the package file name, by dropping
		# the crosstool prefix, and a suffix that conforms to the
		# following sed pattern:
		# '-[0-9]\+\.[0-9]\+\(-patch[0-9]\+\)\?\.tar\.gz'
		# This allows:
		#   crossbuild-ia32-linux-rhel4-2.6.9-1.0.tar.gz
		#   crossbuild-ia32-linux-rhel4-2.6.9-11.23.tar.gz
		#   crossbuild-ia32-linux-rhel4-2.6.9-11.23-patch1.tar.gz
		#   crossbuild-ia32-linux-rhel4-2.6.9-11.23-patch12.tar.gz
		# all mapping to the ia32-linux-rhel4-2.6.9 buildtool
		# base. This allows incremental adding of things to a
		# base crossbuild.
		dirname=$(echo $ini | sed -e 's/^crossbuild-//g' \
			-e 's/-[0-9]\+\.[0-9]\+\(-patch[0-9]\+\)\?\.tar\.gz//g')
		if [ -d $BUILDTOOLS/$dirname ]; then
			echo "      $ini (*)"
		else
			echo "      $ini"
		fi
	done
	echo
	echo "(*): indicates that the crossbuild tool appears to be installed already on"
	echo "this host.  You can however choose to over-install by selecting it here."
	echo
	echo "Select the crossbuild tool(s) to install from the above list, specified as a"
	echo "white-space separated list of the above package names."
	echo "Remember, you can always re-run install.sh to install these or new toolchains."
	echo -n "Specify list of toolchain packages to install [default: none]: "
	read -a toolchain

	if [ ${#toolchain[@]} -gt 0 ]; then
		for cross_tools in "${toolchain[@]}"
		do
			cross_tool_checksum=$(echo $cross_tools | sed -e 's/tar.gz/md5/')
			install_cross_tool='n'
			if [ -f $cross_tools ]; then
				if [ -f $cross_tool_checksum ]; then
					echo -n "Checking integrity of $cross_tools ... "
					md5sum -c $cross_tool_checksum >/dev/null 2>&1
					if [ $? -eq 0 ]; then
						echo "ok"
						install_cross_tool='y'
					else
						echo -en $red >&2
						echo "[ERROR] $cross_tools: checksum error. Skipping installation" >&2
						echo "        of this crosstool." >&2
						echo -en $blue >&2
					fi
				else
					echo "Optional $cross_tool_checksum file not found, skipping the" >&2
					echo "integrity check of $cross_tools" >&2
					install_cross_tool='y'
				fi

				if [ $install_cross_tool == 'y' ]; then
					echo -n "Installing $cross_tools ... "
					if [ ! -d $BUILDTOOLS ]; then
						mkdir -p $BUILDTOOLS
					fi
					cd $BUILDTOOLS
					tar zxf $WORKING_ROOT/$cross_tools \
					|| myexit_notput "[ERROR] error extracting $cross_tools" &
					PS=$!
					roll $PS
					printf "\b"
					cd - >/dev/null 2>&1
					echo "done"
				fi
			else
				echo -en $red >&2
				echo "[ERROR] $cross_tools: file not found. Skipping installation" >&2
				echo "        of this crosstool." >&2
				echo -en $blue >&2
			fi
		done
	#else
	#	echo "Remember, you can always re-run install.sh to install these or new toolchains."
	fi
else
	echo
	echo "Did not find any crossbuild package under $WORKING_ROOT."
    echo "No crossbuild tools are installed this time."
    echo "You can download these later and install them manually,"
    echo "or rerun this install.sh script which will assist you in the installation."
	echo
fi

echo -e ""
echo -e "All prerequisites are installed. Proceeding with ${blue_bold}OpenClovis ${blue}SDK."

tput cnorm

cd $WORKING_DIR

overwrite_openclovis_sdk='y'

if [ -d $PACKAGE_ROOT ]; then
	echo
	echo -e  "${blue_bold}OpenClovis ${blue}$PACKAGE_NAME is already installed. Overwrite?"
	echo -e  "Responding with 'no' will leave the existing SDK intact and proceed to the"
	echo -en "installation of other utilities.  Overwrite existing SDK? <y|n> [n]: "
	read overwrite_openclovis_sdk dummy
	if [ -z $overwrite_openclovis_sdk ]; then
		overwrite_openclovis_sdk='n'
	fi
	case $overwrite_openclovis_sdk in
		Y|y)
			echo
			echo -n "Cleaning up ... "
			cd $INSTALL_ROOT
			rm -rf $PACKAGE_NAME >/dev/null 2>&1
			if [ $? -ne 0 ]; then
				echo -en $red >&2
				echo "[ERROR] cannot remove $PACKAGE_NAME" >&2
				cleanup
				exit 1
			fi

			mkdir $PACKAGE_NAME >/dev/null 2>&1
			if [ $? -ne 0 ]; then
				echo -en $red >&2
				echo "[ERROR] failed to created $PACKAGE_NAME" >&2
				cleanup
				exit 1
			fi

			cd $ECLIPSE/plugins
			rm -rf com.clovis.* >/dev/null 2>&1
			if [ $? -ne 0 ]; then
				echo -en $red >&2
				echo "[ERROR] failed to remove clovis plugins from $ECLIPSE/plugins" >&2
				cleanup
				exit 1
			fi

			echo "done"
			;;
		N|n)
			;;
		*)
			echo "Invalid option, considering as 'no'" >&2
			;;
	esac
fi
if [ ! -d $PACKAGE_ROOT ]; then 
	mkdir -p $PACKAGE_ROOT >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo -en $red >&2
		echo "[ERROR] failed to create $PACKAGE_ROOT directory" >&2
		cleanup
		exit 1
	fi
fi

if [ $overwrite_openclovis_sdk == 'y' ] || [ $overwrite_openclovis_sdk == 'Y' ]; then
	echo
	echo -n "Installing ASP ... "
	#cd $ASP_ROOT
	cd $WORKING_DIR
	tar cf - src/$ASP src/examples |( cd $PACKAGE_ROOT; tar xfm -) &
	PS=$!
	roll $PS
	printf "\b"
	#cd $ASP_ROOT; cd ASP
	# configure script provided with the ASP package has hard coded reference to PKG_CONFIG_PATH
	# prepending the sand-box location to pickup the new installations
	#sed -i -e 's/\/usr\/local\/lib\/pkgconfig/$PKG_CONFIG_PATH:\/usr\/local\/lib\/pkgconfig/' configure

	# modifying mk-cross.mk.in to specify buildtools location
	sed -e "s;buildtools_dir:=/opt/clovis/buildtools;buildtools_dir:=$BUILDTOOLS;g" \
	    -e "s;NET_SNMP_CONFIG = net-snmp-config;NET_SNMP_CONFIG = $NET_SNMP_CONFIG;g" \
		$PACKAGE_ROOT/src/ASP/build/common/templates/make-cross.mk.in > \
		$PACKAGE_ROOT/src/ASP/build/common/templates/make-cross.mk.in.modified

	mv  -f $PACKAGE_ROOT/src/ASP/build/common/templates/make-cross.mk.in.modified \
	       $PACKAGE_ROOT/src/ASP/build/common/templates/make-cross.mk.in
	
	echo "done"

	if [ $INSTALLIDE == "YES" ]; then
		echo
		echo -n "Installing IDE ... "
		cd $WORKING_DIR
		tar cf - $IDE | ( cd $PACKAGE_ROOT; tar xfm -) &
		PS=$!
		roll $PS
		if [ $GPL -eq '1' ]; then 
			tar cf - src/$IDE | ( cd $PACKAGE_ROOT; tar xfm -) &
			PS=$!
			roll $PS
		fi
		printf "\b"
		echo "done"
		echo -n "Linking Eclipse in $PACKAGE_ROOT ... "
		cd $PACKAGE_ROOT
		cp -rl $ECLIPSE .
		# remove redundant clovis plugins if any
		rm -rf eclipse/plugins/*clovis* >/dev/null 2>&1

		# remove splash screen lines from eclipse.ini
		sed -e '/-showsplash\|org.eclipse.platform/d' eclipse/eclipse.ini > eclipse/eclipse_ini.tmp
		rm eclipse/eclipse.ini
		mv eclipse/eclipse_ini.tmp eclipse/eclipse.ini 
		cd $IDE_ROOT/plugins
		mv -f * $PACKAGE_ROOT/eclipse/plugins >/dev/null 2>&1
		cd $IDE_ROOT

		# remove plugins directory from IDE installation as it no more required
		rm -rf plugins

		# update config.ini		
		cd $IDE_ROOT/scripts
		cp -rf config.ini $ECLIPSE/configuration
		echo "done"

		# Delete help cache if the build we are installing is newer than
		# the build we last installed
		if [ -f $CACHE_DIR/eclipse/BUILD ]; then
    			source $CACHE_DIR/eclipse/BUILD
    			export LASTBUILD=$BUILD_NUMBER
    			source $PACKAGE_ROOT/src/ASP/BUILD
    			export CURBUILD=$BUILD_NUMBER
    			if [ "$CURBUILD" == "$LASTBUILD" ]; then
        			echo "do nothing" > /dev/null
    			else
        			cp $PACKAGE_ROOT/src/ASP/BUILD $CACHE_DIR/eclipse/.
        			rm -rf $CACHE_DIR/eclipse/org.eclipse.help.base
    			fi
		else
			if [ ! -f $CACHE_DIR/eclipse ]; then
    				mkdir -p $CACHE_DIR/eclipse
    				if [ $? -ne 0 ]; then
        			echo "[ERROR] Could not create $CACHE_DIR/eclipse. Check permissions."
    				fi
    				echo "Created $CACHE_DIR/eclipse directory" >&2
			fi

    			cp $PACKAGE_ROOT/src/ASP/BUILD $CACHE_DIR/eclipse/.
    			rm -rf $CACHE_DIR/eclipse/org.eclipse.help.base
		fi
		echo "done"
	fi

	echo
	if [ -d $WORKING_DIR/logviewer ]; then
		echo -n "Installing LogViewer ... "
		cd $WORKING_DIR
		tar cf - logviewer |(cd $PACKAGE_ROOT; tar xfm -) &
		PS=$!
		roll $PS
		if [ $GPL -eq 1 ]; then
			tar cf - src/logviewer |(cd $PACKAGE_ROOT; tar xfm -) &
			PS=$!
			roll $PS
		fi
		# setting LOGVIEWER PATH in logViewer.sh
		cd $PACKAGE_ROOT
		sed -i "s;\$PWD;$PACKAGE_ROOT\/logviewer;" logviewer/logViewer.sh
		printf "\b"
		cd - >/dev/null 2>&1
		echo "done"
	fi
	
	echo
	echo -n "Copying documents ... "

	cd $WORKING_DIR
	tar cf - doc | (cd $PACKAGE_ROOT; tar xfm -) >/dev/null 2>&1
	
	echo "done"
	
	echo
	echo -n "Instantiating and copying scripts, rc files, etc ... "
	cd $WORKING_DIR/templates/bin
	
	if [ ! -d $BIN_ROOT ]; then
		mkdir -p $BIN_ROOT
	fi
	
	# escape PACKAGE_ROOT
	ESC_PKG_ROOT=`echo $PACKAGE_ROOT | sed -e 's;/;\\\/;g'`
	ESC_PKG_NAME=`echo $PACKAGE_NAME | sed -e 's/\./\\\./g'`
	
	# adding values to cl-create-project-area
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-create-project-area.in > $BIN_ROOT/cl-create-project-area
	
	# adding values to cl-ide
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-ide.in > $BIN_ROOT/cl-ide
	
	# adding values to cl-log-viewer
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-log-viewer.in > $BIN_ROOT/cl-log-viewer

	# adding values to cl-generate-source
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-generate-source.in > $BIN_ROOT/cl-generate-source
	
	# adding values to cl-validate-project
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-validate-project.in > $BIN_ROOT/cl-validate-project

	# adding values to cl-migrate-project
	sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g" \
		-e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g" \
		$WORKING_DIR/templates/bin/cl-migrate-project.in > $BIN_ROOT/cl-migrate-project

    # symlinking cl-create-wrs-toolchain in $BIN_ROOT
    ln -s $PACKAGE_ROOT/src/ASP/scripts/cl-create-wrs-toolchain $BIN_ROOT/.
    ln -s $PACKAGE_ROOT/src/ASP/scripts/cl-create-wrs2.0-toolchain $BIN_ROOT/.

	chmod +x $BIN_ROOT/*
	
	cd - >/dev/null 2>&1
	
	cd $BIN_ROOT
	ln -s $PACKAGE_ROOT/logviewer/icons icons
	ln -s $PACKAGE_ROOT/logviewer/config config

	cd - >/dev/null 2>&1

	# instantiate clconfig.rc and put it in $LIB_ROOT
	if [ ! -d $LIB_ROOT ]; then
		mkdir -p $LIB_ROOT
	fi

	# escape ECLIPSE
	ESC_ECLIPSE_DIR=`echo $PACKAGE_ROOT/eclipse | sed -e 's;/;\\\/;g'`

	sed -e "s;@SDKDIR@;$ESC_PKG_ROOT;g" \
		-e "s;@ECLIPSE@;$ESC_ECLIPSE_DIR;g" \
		$WORKING_DIR/templates/lib/clconfig.rc.in > $LIB_ROOT/clconfig.rc

	echo "done"
fi

if [ -f $LIB_ROOT/clutils.rc ]; then
	if [ ! -w $LIB_ROOT/clutils.rc ]; then
		echo -en "$red" >&2
		echo "[WARNING] Cannot save run-time utilities in $LIB_ROOT/clutils.rc: permission denied" >&2
		echo -en "$blue" >&2
		cleanup
		exit
	fi
fi

echo
echo -n "Installing utilities ... "
cat << __EOM__ > $LIB_ROOT/clutils.rc
###########################################################################
#  Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
#
#  The source code for this program is not published or otherwise divested
#  of its trade secrets, irrespective of what has been deposited with  the
#  U.S. Copyright office.
#
#  No part of the source code  for this  program may  be use,  reproduced,
#  modified, transmitted, transcribed, stored  in a retrieval  system,  or
#  translated, in any form or by  any  means,  without  the prior  written
#  permission of OpenClovis Inc
###########################################################################

myexit_notput () {

	printf "\\b%-17s" 'Failed'
	tput el
	ROWS=\`tput lines\`
	ROWS=\`expr \$ROWS - 3\`
	tar zcf \$WORKING_DIR/log.tar.gz \$WORKING_DIR/log >/dev/null 2>&1
	#tput cup \$ROWS 0
	echo -e \$red
	echo "\$*, send log.tar.gz to support@openclovis.com for help"
	echo -e \$black
	tput cnorm
	kill \`cat \$INSTALLER \`
	exit
}

# roll cursor to indicate busy
roll () {
	p1=\$1
	echo -n " "
	while [ true ];
	do
        kill -0 \$p1 > /dev/null 2>&1
		if [ \$? -ne 0 ]; then
			return
		fi
		printf "\\b/"
		sleep 0.1
		printf "\\b-"
		sleep 0.1
		printf "\\b\\\\"
		sleep 0.1
		printf "\\b|"
		sleep 0.1
	done
}

cleanup () {
	echo
	echo -n "Cleaning up ... "
	cd \$WORKING_DIR
	if [ -d log ]; then
		mv log log.\`date "+%F-%H-%M"\`
	fi
	echo "done"
}

CONFIGURE_ERROR="[ERROR] Configure failed"
MAKE_ERROR="[ERROR] Make failed"
MAKE_INSTALL_ERROR="[ERROR] Make install failed"
EXTRACT_ERROR="[ERROR] Could not extract tarball"

black='\E[30;49m'
red='\E[31;49m'
green='\E[32;49m'
yellow='\E[33;49m'
blue='\E[34;49m'
magenta='\E[35;49m'
cyan='\E[36;49m'
white='\E[37;49m'

trap 'rm -f \$INSTALLER; tput cup \`tput lines\` 0; tput cnorm; tput sgr0 ' 0
trap "exit 1; tput sgr0" 1 2 3 15 20

__EOM__

if [ ! -d $BIN_ROOT ]; then
	mkdir -p $BIN_ROOT
fi

# copying scripts to $BIN_ROOT

echo "done"

# Build ASP libraries for the local machine and/or installed crossbuild toolchains

DEFAULT_ASP_BUILD_DIR=$sand/$PACKAGE_NAME/prebuild
echo -n "Build ASP libraries for the local machine and/or installed crossbuild toolchains ? <y|n> [y] : "
read asp_build dummy
[ -z $asp_build ] && asp_build='y'

case $asp_build in
    Y|y)
        echo -ne "Where to build ? [default: $DEFAULT_ASP_BUILD_DIR]: "
        read asp_build_dir dummy
        [ -z $asp_build_dir ] && asp_build_dir=$DEFAULT_ASP_BUILD_DIR

        asp_build_dir_create='y'
        if [ ! -d $asp_build_dir ]; then
		    echo -n "Directory $asp_build_dir does not exist, create it? <y/n> [y]: "
		    read asp_build_dir_create dummy
		    [ -z $asp_build_dir_create ] && asp_build_dir_create='y'
        fi

        case $asp_build_dir_create in
            Y|y)
                mkdir -p $asp_build_dir
                if [ $? -ne 0 ]; then
                    echo -en $red >&2
					echo "[ERROR] failed to create $asp_build_dir directory" >&2
					cleanup
					exit 1
                fi
                ;;
            N|n)
                asp_build_dir=$DEFAULT_ASP_BUILD_DIR
                ;;
            *)
				echo "Invalid option, assuming no"
                asp_build_dir=$DEFAULT_ASP_BUILD_DIR
                ;;
        esac

        cd $asp_build_dir
	    
        echo
	    echo "The following installed build tool packages are found:"
	    echo ""

        cd $sand/buildtools

        for tool in `ls -1d *`
	    do
            echo "      $tool"
    	done

        cd -

        echo
	    echo "Select the crossbuild tool(s) to build from the above list, specified as a"
	    echo "white-space separated list of the above package names, [default: none]: "
        
        declare -a toolchain
    	read -a toolchain

        if [ ${#toolchain[@]} -gt 0 ]; then
		for tools in "${toolchain[@]}"
        do
                if [ $tools = "local" ] ; then
                    $sand/$PACKAGE_NAME/src/ASP/configure --with-asp-build > build.log &
                else
                    $sand/$PACKAGE_NAME/src/ASP/configure --with-asp-build --with-cross-build=$tools > build.log &
                fi
                echo -n "building asp $tools ..  "
	            PS=$!
	            roll $PS
                if [ $? -ne 0 ]; then
                    echo -en $red >&2
			        echo "[ERROR] failed to build $tools" >&2
                fi
                cd asp/build/$tools
                make asp-libs
                make asp-install
                cd -
	        done
        fi 
        ;;
    N|n)
        ;;
    *)
        echo "Invalid option, assuming no"
        ;;
esac

# create symbolic links for all scripts that were copied in $BIN_ROOT
DEFAULT_SYM_LINK=""
if [ $ID -eq 0 ]; then
	DEFAULT_SYM_LINK='/usr/local/bin'
else
	declare -a path
	path=( ${OLDPATH//:/" "} )
	for i in "${path[@]}"
	do
		echo $i | grep $USER >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			DEFAULT_SYM_LINK=${i}
			break
		fi
	done
fi

[ -z $DEFAULT_SYM_LINK ] && DEFAULT_SYM_LINK="${HOME}/bin" && MSG='is not found in your PATH environment variable'

echo ""
echo "A few binaries are installed in $BIN_ROOT."
echo "For convenience, you can add the above directory to your PATH definition."
echo "Alternatively, we can create symlinks for you (from a binary directory"
echo "that is already in your PATH)."
echo ""
echo -n "Create symbolic links for items in $BIN_ROOT? <y|n> [n] : "
read create_symbolic_links dummy
[ -z $create_symbolic_links ] && create_symbolic_links='n'

case $create_symbolic_links in
    Y|y)
	echo -ne "Where to create the symbolic links? [default: $blue_bold$DEFAULT_SYM_LINK$blue]: "
	read symlinkdir dummy
	if [ -z $symlinkdir ]; then
		symlinkdir=$DEFAULT_SYM_LINK
	fi
	if [ ! -d $symlinkdir ]; then
		echo -n "Directory $symlinkdir does not exist, create it? <y/n> [n]: "
		read symlinkdircreate dummy
		if [ -z $symlinkdircreate ]; then
			symlinkdircreate='n'
		fi
		case $symlinkdircreate in
			Y|y)
				mkdir -p $symlinkdir
				if [ $? -ne 0 ]; then
					echo -en $red >&2
					echo "[ERROR] failed to create $symlinkdir directory" >&2
					cleanup
					exit 1
				fi
				;;
			N|n)
				;;
			*)
				echo "Invalid option, assuming no"
				;;
		esac
	fi
	if [ -d $symlinkdir ]; then
		cd $symlinkdir
		for file in `ls $BIN_ROOT | egrep -v "cl-log-viewer|config|icons"`; do
			if [ -L $file ]; then
				rm -f $file
			fi
			ln -s $BIN_ROOT/$file $file
			if [ $? -ne 0 ]; then
				echo -e "[WARNING] An existing file $file is in the way. Symlink not"
				echo -e "          created."
			fi
		done
		if [ -f cl-log-viewer ]; then
			rm -f cl-log-viewer
		fi
cat << __EOM__ >cl-log-viewer
#!/bin/sh
cd $PACKAGE_ROOT/bin
./cl-log-viewer
__EOM__
		chmod 555 cl-log-viewer
        fi
	if [ "$MSG" ]; then
		echo
		echo -e "$blue_bold$DEFAULT_SYM_LINK $MSG$blue"
		echo
	fi
	;;
    N|n)
	;;
    *)
	echo "Invalid option, assuming as no"
	;;
esac	

# Finally, if things were installed as root, set ownership on all files
# installed under $PACKAGE to $USER:${GrOUPS[0]}.
# This is a very crude method, as can be a large amount of files, and files
# that have been installed before. But making it more selective is not worth
# the effort.
if [ $ID -eq 0 ]; then
    echo
    echo -n "Fixing file ownerships ... "
    chown -R 0:0 $INSTALL_ROOT > /dev/null 2>&1 &
	PS=$!
	roll $PS
	printf "\b"
    echo "done"
    echo
fi

echo ""
echo -e "======================================================================="
echo -e "Installation of ${blue_bold}OpenClovis SDK$blue is now complete"
echo -e ""
echo -e "Next steps:"
echo -e " -  Run 'cl-create-project-area' to create a new project area and start"
echo -e "    working with the SDK"
echo -e " -  Run 'cl-ide' to start the OpenClovis IDE"
echo -e " -  To read more information, please consult with the user documentation"
echo -e "    installed under $PACKAGE_ROOT/doc."
echo -e "======================================================================="
echo -e ""

cleanup

tput sgr0
exit 0 # return success0
