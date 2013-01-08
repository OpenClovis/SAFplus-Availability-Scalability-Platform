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
# File        : Makefile.in
################################################################################
# Description :
################################################################################

# Subdirectories having Makefiles and need recursive escalation:
# common should always be before client and server


export CLOVIS_ROOT=/opt/clovis/sdk-6.0/src
export PROJECT_ROOT=/root/SAFplus/SAFplus-Availability-Scalability-Platform/src/examples/mgr/ide_workspace
export ASP_MODEL_NAME=amfMgr
export CL_TARGET_PLATFORM=x86_64
export CL_TARGET_OS=linux-3.2.30
export BUILD_ROOT=/root/SAFplus/SAFplus-Availability-Scalability-Platform/src/examples/mgr/ide_workspace/amfMgr/build/local
ifndef MODEL_PATH
export MODEL_PATH=$(PROJECT_ROOT)/$(ASP_MODEL_NAME)/src
endif
export SNMP_BUILD=1
export CM_BUILD=0
export BUILD_TIPC=1
export OPENHPILIBS=
export OPENHPICFLAGS=
export CL_INST=0
export HCL_CM=0
export TOOLCHAIN_DIR=/opt/clovis/buildtools/local
export ASP_BUILD=1
export MODEL_BUILD=1
export PREFIX=/opt/clovis/sdk-6.0
export SDK_DIR=/opt/clovis/sdk-6.0
export ASP_INSTALLDIR=/opt/clovis/sdk-6.0
export TIPC_INCLUDE=/lib/modules/3.2.0-32-generic/build/include
export ASP_LIBDIR=$(ASP_INSTALLDIR)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/lib
export TARGET_QNX=0
export TARGET_SOLARIS=0
export OPENAIS_TIPC=1

#ifdef ModelOnly # Once you've compiled everything once, you can use "make ModelOnly=1" to rebuild just your model
#SUBDIRS          = $(MODEL_PATH)/app
#else
#SUBDIRS          = $(MODEL_PATH)/config $(MODEL_PATH)/app $(CLOVIS_ROOT)/ASP/3rdparty $(CLOVIS_ROOT)/ASP/components $(CLOVIS_ROOT)/ASP/tools
#endif

# Make in the plugin directory only if it exists
pluginDir = $(wildcard $(MODEL_PATH)/plugins)

ifeq ($(ASP_BUILD),1)
	ifeq ($(MODEL_BUILD),0)
		SUBDIRS          = $(CLOVIS_ROOT)/ASP/3rdparty $(CLOVIS_ROOT)/ASP/components $(CLOVIS_ROOT)/ASP/tools
	else
		SUBDIRS		 = $(MODEL_PATH)/config $(MODEL_PATH)/app $(pluginDir) $(CLOVIS_ROOT)/ASP/3rdparty $(CLOVIS_ROOT)/ASP/components $(CLOVIS_ROOT)/ASP/tools
	endif
else
	ifdef ModelOnly
		SUBDIRS		 = $(MODEL_PATH)/app $(pluginDir) servers $(CLOVIS_ROOT)/ASP/tools
    else
		SUBDIRS		 = $(MODEL_PATH)/config $(MODEL_PATH)/app $(pluginDir) servers $(CLOVIS_ROOT)/ASP/tools
	endif
endif

ifeq ($(TARGET_SOLARIS),1)
	export SOLARIS_BUILD=1
endif

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs.
# 

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk

.PHONY: label_build erase_label

all: label_build

clean: erase_label

label_build:
	$(CLOVIS_ROOT)/ASP/build/prerequisites/scripts/label_build.sh

erase_label:
	$(CLOVIS_ROOT)/ASP/build/prerequisites/scripts/label_build.sh CLEAN
