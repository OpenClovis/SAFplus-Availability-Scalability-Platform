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
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################
include ../../../../mk/preface.mk


# Component name (using all lowercase):
COMPNAME	:= amf

SERVER_LIB_NAME := amf

AMF_PATH = $(CLOVIS_ROOT)/safplus/components/amf

CPPFLAGS += $(CM_CFLAGS)

SRC_FILES := $(wildcard *.c)

# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo amf event debug dbal ckpt amf log gms cm idl

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt


CPPFLAGS += $(HPI_CFLAGS) -I../../../include
CPPFLAGS += -I../../common/ams
CPPFLAGS += -I ../../common/ams/parser
CPPFLAGS += -I../../common/cpm 
CPPFLAGS += -I../../common/cpm/idl/xdr
CPPFLAGS += -I../../server/ams 
CPPFLAGS += -I../../server/cpm
CPPFLAGS += -DCL_CPM_GMS
CPPFLAGS += -DCL_CPM_AMS
#CPPFLAGS += -DCL_CPM_COR

# List SAFplus libraries needed to get this server built (or None):

ASP_LIBS	:=                          \
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
                libClAmfClient.a            \
                libClAmsXdr.a               \
                libClAmsParser.a            \
                libClAmsMgmt.a              \
                libClAmsServer.a            \
                libClGms.a                  \
                libClHal.a                  \
                $(CL_CM)

# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml

EXTRA_CPPFLAGS  := -I$(THIRD_PARTY_EZXML_PATH)

# List any system libraries needed to get this server built (or None)


# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk
