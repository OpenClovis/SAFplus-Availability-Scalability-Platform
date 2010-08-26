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
# ModuleName  : cor
# File        : Makefile
################################################################################
# Description :
#
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= cor

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard idl/*.c) \
                   clCorClassExport.c  clCorDmHooks.c   clCorMArrayUtils.c        \
                   clCorMOTreeManage.c   clCorObjHandle.c    clCorStats.c         \
                   clCorVector.c clCorCompInit.c     clCorDmMain.c                \
                   clCorMoClassPathUtil.c  clCorObjTreeLib.c     \
                   clCorSync.c  clCorDbgCmds.c      clCorDmObject.c               \
                   clCorMoIdToNodeNameParser.c clCorNiTable.c clCorDbgCmdUtils.c  	  \
				   clCorEO.c  clCorMoIdToNodeNameTable.c clCorNotify.c   	      \
				   clCorTxnInterface.c clCorDebug.c                 \
                   clCorMoIdUtils.c    clCorPreProvDemo.c                         \
                   clCorDmAttr.c    clCorList.c                                   \
                   clCorMOTreeByPath.c      clCorObjCont.c      clCorRmExport.c   \
                   clCorTxnProcess.c   clCorDmClass.c      clCorMArray.c          \
                   clCorMOTree.c       clCorObjExport.c      clCorRmMain.c        \
                   clCorUtilityServer.c clCorDeltaSave.c clCorModule.c
		
# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal dbal timer buffer cnt ioc rmd eo amf  event om cor debug cm txn log idl name
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:

# List ASP libraries needed to get this server built (or None):

ASP_LIBS	:= libClTimer.a         \
                   libClBuffer.a        \
                   libClOsal.a          \
                   libClCnt.a           \
                   libClEo.a            \
                   libClIoc.a           \
                   libClRmd.a           \
                   libClUtils.a         \
                   libClAmfClient.a     \
                   libClDbal.a          \
                   libClCorClient.a     \
                   libClDebugClient.a   \
                   libClEventClient.a   \
                   libClTxnClient.a     \
                   libClTxnAgent.a      \
                   libClLogClient.a     \
		   libClNameClient.a    \
		   libClIdl.a \
		   libClIdlPtr.a\
		   libClMain.a

# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils

THIRDPARTY_LIBS := -l3rdpartyutils
ifeq ($(TARGET_POSIX), 1)
	DB_LIBS := -db
else
	DB_LIBS := -lgdbm -ldb -lsqlite3
endif
SYS_LIBS        += $(DB_LIBS)
CPPFLAGS        = -I ../common/idl/xdr
EXTRA_CPPFLAGS  := -I$(THIRD_PARTY_EZXML_PATH) -I$(THIRD_PARTY_UTILS_PATH) 
CFLAGS          = -DCL_COR_MEASURE_DELAYS

ifeq ($(BUILD_AMF_OI), 1)
	CFLAGS += -DBUILD_AMF_OI
endif

# List any system libraries needed to get this server built (or None):



# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
