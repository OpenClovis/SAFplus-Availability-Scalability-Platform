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
# ModuleName  : rt
# File        : install.sh
################################################################################
# Description :
## Install script for ASP
################################################################################
######################################
## Initialize environment variables ##
######################################

ASP_VER=2.3
CLOVIS_ROOT=`pwd`
CONF=/etc/asp-rt-$ASP_VER.conf

############################################
## These will be modified appropriately   ##
## by package.sh script or while invoking ##
## 'make package'                         ##
############################################

CL_TARGET_PLATFORM=ia32
CL_TARGET_OS=linux-2.4


################################
## Check for Root permissions ##
################################
USER=`whoami`
if [ $USER != "root" ]
then
    echo "Please run the install script with root privileges ... Exiting"
    exit
fi

########################
## Installation Begin ##
########################

cat << EOF

	You are about to be prompted with a series of questions.  Answer
them carefully, as they determine how ASP servers are to function.

	After the script finishes, you can browse the newly
created /etc/asp-rt-$ASP_VER.conf file to edit and modify further parrameters .

-Press Return to continue
EOF

read temp

###########################
## Installation Location ##
###########################

echo -n "Enter Install Location (Default:- /usr/local)  : "
read PREFIX
if [ -z "$PREFIX" ]
then
    PREFIX=/usr/local
fi

if [ ! -d  $PREFIX ]
then
    echo "$PREFIX does not exist . Creating $PREFIX "
    mkdir -p $PREFIX
fi    

cat << EOF



*** ASP Bootup paramters

The following questions will prompt the user regarding the parameters
to start ASP. To start ASP , the parameters can be obtained either 
using IPMI (if installed) , or by taking input from the user.

If IMPI is installed on the system , Enter "y" .If IPMI is not
installed on the systest or you are not sure , Enter "n". 


EOF

#######################
## IPMI availability ##
#######################

echo -n "Is IPMI avaliable (default n): "
read IPMI_AVAILABLE

if [ -z "$IPMI_AVAILABLE" ]; then
    IPMI_AVAILABLE="n"
else
    if [ $IPMI_AVAILABLE != "y" ] && [ $IPMI_AVAILABLE != "Y" ] && 
        [ $IPMI_AVAILABLE != "N" ] && [ $IPMI_AVAILABLE != "n" ]; then
        echo "Invalid Entry , setting reply to No "
        IPMI_AVAILABLE="n"
    fi
fi

echo
echo

###########################
## Amf Bootup parameters ##
###########################

if [ $IPMI_AVAILABLE = "n" ] || [ $IPMI_AVAILABLE = "N" ];then 
    
    echo -n "Enter Chassis ID (default 0): "
    read CHASSIS_ID
    if [ -z "$CHASSIS_ID" ]
    then
        CHASSIS_ID=0
    fi

    echo -n "Enter Local Slot ID (default 1): "
    read LOCALSLOT_ID
    if [ -z "$LOCALSLOT_ID" ]
    then
        LOCALSLOT_ID=1
    fi

fi

echo -n "Enter Node Name (default SysController_0): "
read ASP_NODENAME
if [ -z "$ASP_NODENAME" ]
then
    ASP_NODENAME=SysController_0
fi

echo -n "Enter Boot Profile (default debug): "
read BOOT_PROFILE
if [ -z "$BOOT_PROFILE" ]
then
    BOOT_PROFILE=debug
fi

cat << EOF

*** ASP server configuration

The following questions are specific to certain ASP servers. 
If the server is enabled for this RT installation  , certain
questions will be asked with regards to that server , so that 
it will start succesfully on bootup.

EOF

################################################
## CM Server paramters - Location of rt-en.sh ##
################################################

echo -n "Is CM Server enabled in this package (y/n) (default y) : "
read CM_ENABLED
if [ -z "$CM_ENABLED" ]; then
    CM_ENABLED="y"
else
    if [ $CM_ENABLED != "y" ] && [ $CM_ENABLED != "Y" ] && [ $CM_ENABLED != "N" ] &&
        [ $CM_ENABLED != "n" ]; then
        echo "Invalid Entry , setting reply to Yes "
        CM_ENABLED="y"
    fi
fi

if [ $CM_ENABLED = "y" ]; then
    echo "Certain environment variables need to be set before starting CM Server." 
    echo "These environment variables are generally present in the openhpi source package"
    echo "in a file called rt-env.sh"
    
    echo
    echo
    
    echo -n "Enter Location of rt-env.sh (Reqd To start CM Server): "
    read OPENHPI_PATH
    if [ -z "$OPENHPI_PATH" ]
    then
        echo " Open HPI Path not set. Modify Env variable OPENHPI_PATH in $CONF to set it "
    fi
fi

echo
echo

###########################
## SNMP Server paramters ## 
###########################

echo -n "Is SNMP Server enabled in this package (y/n) (default y) : "
read SNMP_ENABLED
if [ -z "$SNMP_ENABLED" ]; then
    SNMP_ENABLED="y"
else
    if [ $SNMP_ENABLED != "y" ] && [ $SNMP_ENABLED != "Y" ] && [ $SNMP_ENABLED != "N" ] && [ $SNMP_ENABLED != "n" ]; then
        echo "Invalid Entry , setting reply to Yes "
        SNMP_ENABLED="y"
    fi
fi

echo
echo
echo

echo "*** Beginning Installation "
echo
echo


####################################################
## Initializing Env variables for RT installation ##
## and Creating directories for the same          ##
####################################################

RT_PREFIX=$PREFIX/clovis/rt/$ASP_VER
RT_ASP=$RT_PREFIX/asp

RT_LIB=$RT_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/lib
RT_BIN=$RT_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/bin
RT_MOD=$RT_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/modules
RT_CONFIG=$RT_ASP/$CL_TARGET_PLATFORM/$CL_TARGET_OS/config
RT_DOC=$RT_PREFIX/doc

BOOT_DIR=/etc/rc.d/init.d
INIT_LINK=/etc/rc.d/rc5.d/S96asp
STOP_LINK=/etc/rc.d/rc3.d/K02asp

echo "Creating $RT_LIB :`mkdir -p $RT_LIB` Done"
echo "Creating $RT_BIN :`mkdir -p $RT_BIN` Done"
echo "Creating $RT_MOD :`mkdir -p $RT_MOD` Done"
echo "Creating $RT_CONFIG :`mkdir -p $RT_CONFIG` Done"


##############################
## Copying shared libraries ##
##############################

echo -n "Copying libraries : "
cp -pr $CLOVIS_ROOT/lib/* $RT_LIB
echo "Done"

######################
## Copying binaries ##
######################

echo -n "Copying binaries : "
cp -pr $CLOVIS_ROOT/bin/* $RT_BIN
echo "Done"

############################################
## Copying kernel modules and IOC scripts ##
############################################

echo -n "Copying module files/scripts : "
cp -pr $CLOVIS_ROOT/modules/* $RT_MOD
cp -pr $CLOVIS_ROOT/scripts/*.sh $RT_MOD
echo "Done"

##########################
## Copying config files ##
##########################

echo -n "Copying config/templates : "
cp -pr $CLOVIS_ROOT/config/* $RT_CONFIG
echo "Done"

############################
## Copying init.d scripts ##
############################

echo -n "Copying boot-up script : "
cp -rf $CLOVIS_ROOT/scripts/asp $BOOT_DIR
chmod 555 $BOOT_DIR/asp
cp -rf $CLOVIS_ROOT/scripts/uninstall.sh $PREFIX/clovis
chmod +x $PREFIX/clovis/uninstall.sh
echo "Done"

echo -n "Creating soft-links : "
ln -sf $BOOT_DIR/asp $INIT_LINK
ln -sf $BOOT_DIR/asp $STOP_LINK
echo "Done"


#FIXME :- Is this needed
## Copy debug script
#echo -n "Copying debug Server script : "
#cp -rf $CLOVIS_ROOT/scripts/asp-debug /usr/bin
#chmod +x /usr/bin/asp-debug

#################################
## Generating asp-rt-2.2.conf ##
#################################

echo -n "Generating $CONF : "

echo "export CLOVIS_ROOT=$PREFIX" > $CONF
echo "export ASP_KMOD=$RT_MOD" >> $CONF
echo "export ASP_CONFIG=$RT_CONFIG" >> $CONF
echo "export MODEL_CONFIG=$RT_CONFIG" >> $CONF
echo "export ASP_BIN=$RT_BIN" >> $CONF

echo "export IPMI_AVAILABLE=$IPMI_AVAILABLE" >> $CONF
echo "export CHASSIS_ID=$CHASSIS_ID" >> $CONF
echo "export LOCALSLOT_ID=$LOCALSLOT_ID" >> $CONF
echo "export ASP_NODENAME=$ASP_NODENAME" >> $CONF
echo "export BOOT_PROFILE=$BOOT_PROFILE" >> $CONF

echo "export CM_ENABLED=$CM_ENABLED" >> $CONF
echo "export OPENHPI_PATH=$OPENHPI_PATH" >> $CONF

echo "export SNMP_ENABLED=$SNMP_ENABLED" >> $CONF
echo "export SNMPCONFPATH=$RT_CONFIG:$SNMPCONFPATH" >> $CONF
if [ $SNMP_ENABLED = "y" ] || [ $SNMP_ENABLED == "Y" ]; then
    echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$RT_LIB:`net-snmp-config --prefix`/lib" >> $CONF
else
    echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$RT_LIB" >> $CONF
fi
echo "Done"

echo
echo
echo
echo "*** Installation Succesful"


