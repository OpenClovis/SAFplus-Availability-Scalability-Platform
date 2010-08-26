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
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################

# Component name (using all lowercase):
COMPNAME	:= amf

SERVER_LIB_NAME := amf

AMF_PATH = $(CLOVIS_ROOT)/ASP/components/amf

SRC_FILES :=        $(wildcard $(AMF_PATH)/common/cpm/*.c)                                        \
		    $(wildcard *.c)                                 \

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo amf event debug dbal \
                    ckpt amf log gms cor om cm idl

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt
ifeq ($(HCL_CM),1)
CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/radisys
else
CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/openhpi
endif


CPPFLAGS += -I../../../include
CPPFLAGS += -I../../common/ams
CPPFLAGS += -I ../../common/ams/parser
CPPFLAGS += -I../../common/cpm 
CPPFLAGS += -I../../common/cpm/idl/xdr
CPPFLAGS += -I../../server/ams 
CPPFLAGS += -I../../server/cpm
CPPFLAGS += -DCL_CPM_GMS
CPPFLAGS += -DCL_CPM_AMS
#CPPFLAGS += -DCL_CPM_COR

# List ASP libraries needed to get this server built (or None):

ASP_LIBS	:=                              \
                libClTimer.a                \
                libClBuffer.a               \
                libClOsal.a                 \
                libClCnt.a                  \
                libClEo.a                   \
                libClIoc.a                  \
                libClRmd.a                  \
                libClDbal.a                 \
                libClUtils.a                \
                libClEventClient.a          \
                libClDebugClient.a          \
                libClNameClient.a           \
                libClIdl.a                  \
                libClIdlPtr.a               \
                libClCkpt.a                 \
                libClLogClient.a            \
                libClFaultClient.a          \
                libClAmfClient.a            \
                libClCorClient.a            \
                libClAmsXdr.a               \
                libClAmsParser.a            \
                libClAmsMgmt.a        \
                libClAmsServer.a            \
                libClGms.a                  \
                libClCm.a                   \
                   
# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils/

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml \
			-L$(THIRD_PARTY_UTILS_PATH) -l3rdpartyutils

EXTRA_CPPFLAGS  := -I$(THIRD_PARTY_EZXML_PATH) -I$(THIRD_PARTY_UTILS_PATH)

# List any system libraries needed to get this server built (or None)


# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
