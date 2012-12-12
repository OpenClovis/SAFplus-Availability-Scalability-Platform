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
# ModuleName  : mw 
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

COMPNAME	:= mw 

LIB_NAMES := libmw

RANLIB := ranlib

DEP_COMP_CM :=

# List local source files needed for the component server:

BASE_OBJ_DIR := $(PROJECT_ROOT)/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS)/obj
ALL_OBJ_DIR = $(BASE_OBJ_DIR)
CM_CONFLICT_RESOLVE_OBJS := $(BASE_OBJ_DIR)/components/ground/client/clGroundCm.o

ifneq ($(strip $(CL_CM)),)
    DEP_COMP_CM = cm/client
    CM_CONFLICT_RESOLVE_OBJS :=
endif

ALL_CONFLICT_RESOLVE_OBJS := $(BASE_OBJ_DIR)/components/ckpt/client/clIdlOpen.o \
	$(BASE_OBJ_DIR)/components/msg/client/xdrClIocPhysicalAddressTMarshall.o \
	$(BASE_OBJ_DIR)/components/msg/client/xdrClIocPhysicalAddressTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOHandleTMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOHandleTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOIdTMarshall.o \
	$(BASE_OBJ_DIR)/components/cor/client/xdrClCorMOIdTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrClIocAddressIDLTMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrClIocAddressIDLTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrClIocLogicalAddressIDLTMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrClIocLogicalAddressIDLTUnMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrDWordMarshall.o \
	$(BASE_OBJ_DIR)/components/amf/client/cpm/xdrDWordUnMarshall.o \
	$(BASE_OBJ_DIR)/components/idl/common/xdrClGenericPtr.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundOMAlarmStruct.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundTrace.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundProvTxnStruct.o \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundCustom.o \
	$(CM_CONFLICT_RESOLVE_OBJS) \
	$(BASE_OBJ_DIR)/components/ground/client/clGroundEo.o

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:=  \
             cnt/client    \
             buffer/client \
             osal/client   \
             timer/client  \
             utils/client  \
             ioc/client    \
             rmd/client    \
             idl/client idl/common    \
             dbal/client   \
             eo/client    \
             debug/client  \
             amf/client    \
             alarm/client alarm/utils alarm/common/idl/xdr  \
             event/client  \
             ckpt/client ckpt/test/src   \
             name/client   \
             om/client     \
             cor/client    \
             fault/client  \
             sm/client     \
             txn/client    \
			 med/client	\
			 mso/client    \
			 prov/client 	\
			 hal/client 	\
			 gms/client gms/server	\
			 log/client    \
			 msg/client    \
			 pm/client     \
             ntf/client \
			$(CM_COMP_DEP)	\
			amf/client/cpm amf/client/ams amf/common/ams/idl/xdr amf/test/src \

THIRD_PARTY_LIST := ../3rdparty/utils ../3rdparty/ezxml/ezxml-0.8.6 ../3rdparty/openais/openais-0.80.3/exec

COMP_LIST := $(DEP_COMP_LIST) $(THIRD_PARTY_LIST)

ALL_ASP_LIBS    :=         -lClTimer -lClBuffer -lClOsal -lClIoc -lClUtils -lClEo -lClMsoClient -lClProv \
			   -lClLogClient -lClTxnClient -lClTxnAgent -lClSm -lClCnt -lClIdl -lClRmd \
			   -lClCorClient -lClAmfClient -lClAmsMgmt -lClAmsXdr -lClDebugClient -lClFaultClient \
				   -lClEventClient -lClDbal -lClGms $(CL_CM) -lClCkpt -lClMedClient -lClNameClient \
			   -lClUtils -lClIdlPtr -lClAlarmClient -lClAlarmUtils -lClAlarmXdr -lClOmClient -lClMsgClient \
			   -lClPMClient -lClNtf -l3rdpartyutils -lClGroundClient 

ALL_OBJS :=

ALL_FILTER_OBJS :=

define OBJ_LIST
	ALL_OBJS += $(wildcard $(1)/*.o)
	ALL_FILTER_OBJS += $(filter $(1)/asp_build.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clIdlOpen.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocPhysicalAddressTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocPhysicalAddressTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOHandleTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOHandleTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOIdTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClCorMOIdTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocAddressIDLTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocAddressIDLTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocLogicalAddressIDLTMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClIocLogicalAddressIDLTUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrDWordMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrDWordUnMarshall.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/xdrClGenericPtr.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clGmsCli.o, $(wildcard $(1)/*.o))
	ALL_FILTER_OBJS += $(filter $(1)/clGmsEo.o, $(wildcard $(1)/*.o))
endef

obj_files := $(foreach objdir, $(addprefix $(BASE_OBJ_DIR)/components/, $(COMP_LIST)), $(eval $(call OBJ_LIST, $(objdir))))

OBJ_FILES := $(filter-out $(ALL_FILTER_OBJS), $(sort $(ALL_OBJS)))

OBJ_FILES +=  $(ALL_CONFLICT_RESOLVE_OBJS)

ALL_OBJ_FILES := $(OBJ_FILES)

EXTRA_SHARED_LDFLAGS+=-L$(ASP_LIB) -lezxml

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
