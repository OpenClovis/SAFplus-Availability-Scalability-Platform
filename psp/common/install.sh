#!/bin/bash
#
# install.sh
#
# Installation script to install OpenClovis PSP packages into an existing SDK
# installation.
#

PACKAGE_NAME=sdk-4.0

# Trap signals: cleanup & exit
trap "trapped; exit 1" 1 2 3 15 20

# Always jump to the dir where this install.sh is located
cd $(dirname $0)

PSP_NAME=$(basename $PWD)

CACHE_DIR="${HOME}/.clovis/$PACKAGE_NAME"
CACHE_FILE="install.cache"
defsand=""

# Let's know if we are running as a non-root user
ID=$(id -u)

if [ -f $CACHE_DIR/$CACHE_FILE ]; then
	defsand=$(cat $CACHE_DIR/$CACHE_FILE)
else
	if [ $ID -ne 0 ]; then
		defsand=$HOME/clovis
	else
		defsand=/opt/clovis
	fi
fi

if [ -n "$1" ]; then
    PKG_DIR=$1
else
    echo
    echo "Installing OpenClovis Platform Support Package '$PSP_NAME'"
    echo
    echo "You will need to specify the directory in which OpenClovis SDK has been"
    echo "installed previously."
    echo
    echo "Please note that you must have write permission to the SDK installation"
    echo "directory to proceed."
    echo
    echo -en "Enter the installation root directory [default:$defsand]: "
    read sand dummy #don't allow any blank spaces in the directory name
    if [ "x${sand}" == "x" ]; then
	    sand=$defsand
    else
	    sand=`echo $sand | perl -n -e 's/(\/*)$//; print "$_"'`
    fi
    # handle ~ in directory name
    sand=$(echo $sand | sed -e "s/^~/$(echo $HOME | sed -e 's/\//\\\//g')/") #"
    if [ ! -d $sand ]; then
        echo "[ERROR] Directory $sand does not exist"
        exit 1
    fi
    PKG_DIR=$sand/$PACKAGE_NAME
    if [ ! -d $PKG_DIR ]; then
        echo "[ERROR] Directory $PKG_DIR does not exist. $sand does not look"
        echo "        like a correct ASP installation directory"
        exit 1
    fi
    # TODO We shall add some version checking later here
fi

# Some sanity checking: verify that $PKG_DIR has the usual src and src/ASP
# subdir
if [ ! -d $PKG_DIR/src/ASP ]; then
    echo "[ERROR] $PKG_DIR does not seem to be a complete/correct ASP"
    echo "        installation. Could not fine the"
    echo "        $PKG_DIR/src/ASP subdirectory!"
    exit 1
fi

echo "Installing PSP '$PSP_NAME' into SDK package directory '$PKG_DIR'..."
echo

# First, we will copy the overlay files over the SDK installation
rsync -avDH --exclude='.svn' pkg-overlay/ $PKG_DIR/
if [ $? != 0 ]; then
    echo
    echo "[ERROR] Installing the overlay files failed. Permission problem?"
    exit 1
fi

# Then we will apply all patches listed in the patches directory
patch_files=$(find patches -name '*.patch' | sort)
if [ "$patch_files" ]; then
    echo
    echo "[ERROR] Patch file installation not yet implemented"
    exit 1
fi

# Finally mark the PKD directory with a signature that indicates that the
# PSP was installed
touch $PKG_DIR/psp-$PSP_NAME.installed
if [ $? != 0 ]; then
    echo
    echo "[ERROR] Could not register installation"
    exit 1
fi

echo
echo "PSP '$PSP_NAME' installation completed"
echo

