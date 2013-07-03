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
# ModuleName  : externalLib 
# File        : Makefile
################################################################################
# Description :
#
# Makefile for ASP standard client library
# This library internally links to all client libraries of ASP
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################
include $(CLOVIS_ROOT)/SAFplus/mk/preface.mk

# Component name (using all lowercase):

COMPNAME	:= external 

LIB_NAMES := libexternal

# I do not want to autogenerate a target for this so do not set LIB_NAMES := libmw

RANLIB := ranlib


# List local source files needed for the component server:

BASE_OBJ_DIR := $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/obj

ALL_OBJ_DIR = $(BASE_OBJ_DIR)

ALL_CONFLICT_RESOLVE_OBJS := $(BASE_OBJ_DIR)/components/ckpt/client/clIdlOpen.o \
	$(BASE_OBJ_DIR)/components/msg/client/xdrClIocPhysicalAddressTMarshall.o \
	$(BASE_OBJ_DIR)/components/msg/client/xdrClIocPhysicalAddressTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOHandleTMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOHandleTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOIdTMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOIdTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundOMAlarmStruct.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundTrace.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundProvTxnStruct.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundCustom.o \
	$(CM_CONFLICT_RESOLVE_OBJS) 

# List other ASP components of which client libraries your component
# depends on: utils osal timer buffer cnt ioc eo rmd

DEP_COMP_LIST	:=  \
             cnt/client    \
             buffer/client \
             osal/client   \
             timer/client  \
             utils/client  \
             ioc/client    \
             eo/client    \
             rmd/client    \
             idl/client \
	     idl/common    \
             debug/client  \
             event/client  \
             name/client   \
	     log/client    \
	

THIRD_PARTY_LIST := ../3rdparty/utils ../3rdparty/ezxml/ezxml-0.8.6 ../3rdparty/openais/openais-0.80.3/exec

COMP_LIST := $(DEP_COMP_LIST) $(THIRD_PARTY_LIST)

ALL_ASP_LIBS    := \
			libClTimer.a \
                   	libClBuffer.a \
                   	libClOsal.a \
                   	libClCnt.a \
                   	libClUtils.a \
                   	libClIoc.a \
                        libClRmd.a \
                        libClLogClient.a \
                        libClIdl.a \
                        libClNameClient.a \
                        libClEventClient.a \
                        libClDebugClient.a \

ALL_OBJS :=
ALL_FILTER_OBJS :=

define OBJ_LIST
	ALL_OBJS += $(wildcard $(1)/*.o)
	ALL_FILTER_OBJS += $(filter $(1)/asp_build.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clIdlOpen.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOHandleTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOHandleTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOIdTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOIdTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clGmsCli.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clGmsEo.o, $(wildcard $(1)/*.o))
endef

obj_files := $(foreach objdir, $(addprefix $(BASE_OBJ_DIR)/components/, $(COMP_LIST)), $(realpath $(eval $(call OBJ_LIST, $(objdir)))))

BASE_OBJ_FILES_FILTER := $(BASE_OBJ_DIR)/asp_build.o $(BASE_OBJ_DIR)/ezxml.o 
CONFLICT_FILTER:= $(BASE_OBJ_DIR)/components/ground/client/clGroundIoc.o \
                  $(BASE_OBJ_DIR)/components/ground/client/clGroundLog.o \
                  $(BASE_OBJ_DIR)/components/ground/client/clGroundDebug.o \
		  $(BASE_OBJ_DIR)/components/ground/client/clGroundName.o \
                  $(BASE_OBJ_DIR)/components/amf/client/cpm/xdrClGenericPtr.o\
                  $(BASE_OBJ_DIR)/components/amf/client/cpm/clIdlOpen.o\
                  $(BASE_OBJ_DIR)/components/event/client/clIdlOpen.o\
                  $(BASE_OBJ_DIR)/components/event/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/log/client/clIdlOpen.o\
                  $(BASE_OBJ_DIR)/components/osal/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/cnt/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/buffer/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/eo/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/ground/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/rmd/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/ioc/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/utils/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/timer/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/ioc/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/log/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/name/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/idl/common/asp_build.o\
                  $(BASE_OBJ_DIR)/components/idl/client/asp_build.o\
                  $(BASE_OBJ_DIR)/components/../3rdparty/openais/openais-0.80.3/exec/clovis_gms_wrapper.o\


ALL_OBJS += $(filter-out $(BASE_OBJ_FILES_FILTER), $(EXTERNAL_OBJECT))
OBJ_FILES := $(filter-out $(ALL_FILTER_OBJS), $(sort $(ALL_OBJS)))
OBJ_FILES := $(filter-out $(CONFLICT_FILTER), $(sort $(ALL_OBJS)))
ALL_OBJ_FILES := $(OBJ_FILES)

#exec := $(shell mkdir -p $(BASE_OBJ_DIR)/tmp)
#exec := $(foreach file, $(OBJ_FILES), $(shell cp $(file) $(BASE_OBJ_DIR)/tmp))
#OBJ_FILES:= $(wildcard $(BASE_OBJ_DIR)/tmp/*)

#NOVAR := $(shell echo $(OBJ_FILES) > objfiles.txt)
NOVAR0 := $(shell echo 'in object file')
NOVAR0 := $(shell echo $(filter-out $(BASE_OBJ_FILES_FILTER), $(wildcard $(BASE_OBJ_DIR)/components/ioc/client/*.o)) > /root/eo.log)
NOVAR0 := $(shell echo $(filter-out $(BASE_OBJ_FILES_FILTER), $(wildcard $(BASE_OBJ_DIR)/components/debug/*.o)) > /root/debug.log)
NOVAR := $(shell echo $(ALL_FILTER_OBJS) > /root/filterobjfiles.log)
NOVAR := $(shell echo $(ALL_OBJ_FILES) > /root/objfiles.log)
#EXTRA_SHARED_LDFLAGS+= -lezxml

# Library name:

# If there are more than one client libraries to be built here, you
# need to list the object files that will make up each library:
#
# Example:
#     libClTmpcomp_basic: file1.o file2.o
#     libClTmpcomp_advanced: file1.o file2.o file3.o file4.o
#
# If your component builds one library only, than this list will automatically
# obtained from the SRC_FILES variable above, so you don't need to create it.
#

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk

