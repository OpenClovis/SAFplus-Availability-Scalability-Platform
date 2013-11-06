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

export SAFPLUS_MODEL_DIR=/root/workspace333/rmdExternalExampleModel
export SAFPLUS_SRC_DIR=/opt/clovis/6.1/sdk/src
export TOOLCHAIN_DIR=/local
include $(SAFPLUS_MODEL_DIR)/build/$(TOOLCHAIN_DIR)/mk/define.mk


#ifdef ModelOnly # Once you've compiled everything once, you can use "make ModelOnly=1" to rebuild just your model
#SUBDIRS          = $(MODEL_PATH)/app
#else
#SUBDIRS          = $(MODEL_PATH)/config $(MODEL_PATH)/app $(CLOVIS_ROOT)/SAFplus/3rdparty $(CLOVIS_ROOT)/SAFplus/components $(CLOVIS_ROOT)/SAFplus/tools
#endif

# Make in the plugin directory only if it exists
pluginDir = $(wildcard $(MODEL_PATH)/plugins)

ifeq ($(ASP_BUILD),1)
	ifeq ($(MODEL_BUILD),0)
		SUBDIRS          = $(CLOVIS_ROOT)/SAFplus/3rdparty $(CLOVIS_ROOT)/SAFplus/components $(CLOVIS_ROOT)/SAFplus/tools  
	else
		SUBDIRS		 = $(MODEL_PATH)/config $(MODEL_PATH)/app $(pluginDir) $(CLOVIS_ROOT)/SAFplus/3rdparty $(CLOVIS_ROOT)/SAFplus/components $(CLOVIS_ROOT)/SAFplus/tools 
	endif
else
	ifdef ModelOnly
		SUBDIRS		 = $(MODEL_PATH)/app $(pluginDir) $(CLOVIS_ROOT)/SAFplus/tools
    else
		SUBDIRS		 = $(MODEL_PATH)/config $(MODEL_PATH)/app $(pluginDir) $(CLOVIS_ROOT)/SAFplus/tools
	endif
endif

ifeq ($(TARGET_SOLARIS),1)
	export SOLARIS_BUILD=1
endif

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs.
# 

include $(CLOVIS_ROOT)/SAFplus/mk/make-subdir.mk

ifeq ($(ASP_BUILD),1)
ifeq ($(MODEL_BUILD),0)        
        $(shell rm -f $(PROJECT_ROOT)/configure.conf)
        $(shell echo -e "export CM_BUILD=$(CM_BUILD)\nexport HCL_CM=$(HCL_CM)\nexport HPI_EMERSON=$(HPI_EMERSON)\nexport HPI_OPENHPI=$(HPI_OPENHPI)\nexport SAFPLUS_BUILD_PYTHON=$(SAFPLUS_BUILD_PYTHON)\nexport SAFPLUS_BUILD_JAVA=$(SAFPLUS_BUILD_JAVA)\nexport SAFPLUS_BUILD_CPP=$(SAFPLUS_BUILD_CPP)" > $(PROJECT_ROOT)/configure.conf)
endif
endif

.PHONY: label_build erase_label

all: label_build 

clean: erase_label 

label_build:
	$(CLOVIS_ROOT)/SAFplus/build/prerequisites/scripts/label_build.sh

erase_label:
	$(CLOVIS_ROOT)/SAFplus/build/prerequisites/scripts/label_build.sh CLEAN
