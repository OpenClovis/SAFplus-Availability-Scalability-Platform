###############################################################################
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
# ModuleName  : 
# File        : make-path.mk
################################################################################
# Description :
#
#  Make include for setting up build paths
#  Called from make-client.mk and make-server.mk. It is assumed that it is
#  never included directly from a Makefile, as this file is not self-sufficient.
#  It's sole purpose is to allow a common mechanism to determine where things
#  should be built.
#
################################################################################

COMP_ROOT		:= $(CLOVIS_ROOT)/ASP/components

ifdef SOLARIS_BUILD
        AWK=gawk
else
        AWK=awk
endif
export AWK

# Decide if this is an ASP component or not. We think it is an ASP component
# if ASP can be found "upstream" from the current dir and it contains a
# "components" and a "3rdparty" subdir.
ifdef SOLARIS_BUILD
    IS_ASP_COMP := $(shell pwd | grep -c "ASP/components")
else
    IS_ASP_COMP := $(shell \
        asp_dir=$$(pwd|awk '{if(match($$0,".*/ASP/")){print substr($$0,0,RLENGTH-1)}}'); \
        if [ -d "$$asp_dir" -a \
             -d "$$asp_dir"/components -a \
             -d "$$asp_dir"/3rdparty ]; then \
            echo 1; \
        else \
            echo 0; \
        fi)
endif

#$(warning IS_ASP_COMP:$(IS_ASP_COMP))
#$(warning ASP_MODEL_NAME:$(ASP_MODEL_NAME))

ifeq ("$(IS_ASP_COMP)","1")
    # $(warning Building ASP component)
    # In this case we create the BUILD_SUBPATH as the subdirectory under ASP
    BUILD_SUBPATH := $(shell \
        pwd | $(AWK) '{match($$0,".*/ASP/"); print substr($$0,RLENGTH+1)}')
else
    # $(warning Building user component)
	# in this case we create the BUILD_SUBPATH as the subdirectory under the
	# model directory
    BUILD_SUBPATH := $(shell \
        pwd | $(AWK) '{match($$0,regex); print substr($$0,RLENGTH+1)}' regex=".*/$(ASP_MODEL_NAME)/")
endif

#$(warning BUILD_SUBPATH:$(BUILD_SUBPATH))

ifeq ("$(IS_ASP_COMP)","1")
	OBJ_DIR		:= $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/obj/$(BUILD_SUBPATH)
	DEP_DIR		:= $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/dep/$(BUILD_SUBPATH)
	INC_DIR		:= $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/inc/$(BUILD_SUBPATH)
	LIB_DIR		:= $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/lib
	ETC_DIR		:= $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/etc
else
	OBJ_DIR 	:= $(MODEL_TARGET)/obj/$(BUILD_SUBPATH)
	DEP_DIR 	:= $(MODEL_TARGET)/dep/$(BUILD_SUBPATH)
	INC_DIR	    := $(MODEL_TARGET)/inc/$(BUILD_SUBPATH)
	LIB_DIR		:= $(MODEL_TARGET)/lib
	ETC_DIR		:= $(MODEL_TARGET)/etc

endif
BIN_DIR			:= $(MODEL_BIN)

#$(warning OBJ_DIR:$(OBJ_DIR))
#$(warning DEP_DIR:$(DEP_DIR))
#$(warning INC_DIR:$(INC_DIR))
#$(warning BIN_DIR:$(BIN_DIR))
