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
# ModuleName  : alarm
# File        : Makefile
################################################################################
# Description :
################################################################################

# Component name (using all lowercase):

COMPNAME	:= alarm

COR_PATH    := $(CLOVIS_ROOT)/SAFplus/components/cor

# List local source files needed for the component server:

SRC_FILES	:= clAlarmMain.c clAlarmServerEngine.c clAlarmServerAlarmUtil.c\
 clAlarmCorDbgCmdUtils.c \
 clAlarmServerCmds.c \
 clAlarmInfoCnt.c \
 clAlarmPayloadCont.c \
 clAlarmDebug.c \
 clAlarmClientTable.c \
 ../common/clAlarmCommons.c


# List other SAFplus components of which client libraries your component
# depends on:
DEP_COMP_LIST	:= utils osal timer buffer ioc rmd event eo fault cor om amf hal txn \
debug log idl name

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt

CPPFLAGS += -I./

# List SAFplus libraries needed to get this server built (or None):
ASP_LIBS	:= 	   libClTimer.a \
                   libClBuffer.a \
				   libClOsal.a \
				   libClIoc.a \
				   libClUtils.a \
                   libClCnt.a \
                   libClEo.a \
                   libClRmd.a \
				   libClCorClient.a \
                   libClAmfClient.a \
                   libClDebugClient.a \
                   libClLogClient.a \
                   libClNameClient.a \
                   libClIdl.a \
                   libClFaultClient.a \
				   libClEventClient.a \
				   libClMain.a \
				   libClAlarmXdr.a \
				   libClOmClient.a \
				   libClGround.a


# List any third party libs needed to get this server built (or None):

    
THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH) -I../common/idl/xdr
CPPFLAGS += -I$(COR_PATH)/common/idl/xdr
CPPFLAGS += -I../client/

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk
