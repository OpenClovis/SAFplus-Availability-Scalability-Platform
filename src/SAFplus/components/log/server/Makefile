################################################################################
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
# ModuleName  : log                                                             
# File        : Makefile
################################################################################
# Description :
# 
# Makefile for log component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################


# Component name (using all lowercase):
COMPNAME	:=logd
# Local source files needed for the component server:
LOG_PATH := $(CLOVIS_ROOT)/ASP/components/log

SRC_FILES := clLogStreamOwnerEo.c \
             clLogStreamOwnerCkpt.c\
             clLogStreamOwner.c \
             clLogStreamOwnerSvrClnt.c\
             clLogFileOwnerEo.c\
             clLogFileOwner.c \
             clLogFileOwnerUtils.c\
             clLogFileOwnerDeputy.c\
             clLogFileEvt.c\
             clLogSvrEo.c\
             clLogSvrCkpt.c  \
             clLogSvrEvt.c  \
             clLogGms.c \
             clLogSvrCommon.c \
             clLogFlusher.c\
             clLogMaster.c\
             clLogMasterCkpt.c\
             clLogMasterEo.c\
             clLogSvrMain.c \
             clLogServer.c \
             clLogServerDump.c \
             clLogSvrDebug.c \
             clLogFileHdlr.c \
             $(wildcard $(LOG_PATH)/idl/Log/server/*.c)\
             $(wildcard $(LOG_PATH)/idl/Log/client/*.c)\
             $(wildcard $(LOG_PATH)/idl/Log/xdr/*.c)\
             $(wildcard $(LOG_PATH)/idl/App/client/*.c)\
             $(wildcard $(LOG_PATH)/common/*.c)\
#           clLogEvt.c             

# Other ASP components on which asp_logd depends on 

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo idl name ckpt dbal debug log cnt event gms

EXTRA_CPPFLAGS	+= -DCL_DEBUG
# Needed to run with P=1, could have added to make-common but then all other exes' size would have
#increased
# Other ASP libraries needed to get asp_logd 

ASP_LIBS	:= libClTimer.a  		\
			   libClBuffer.a 		\
			   libClOsal.a 	 		\
			   libClCnt.a 	 		\
			   libClIoc.a 			\
			   libClEo.a 			\
			   libClRmd.a 			\
			   libClUtils.a 		\
			   libClCkpt.a 			\
			   libClDbal.a 			\
			   libClDebugClient.a 	\
			   libClEventClient.a 	\
			   libClIdl.a 			\
			   libClIdlPtr.a 			\
			   libClNameClient.a 	\
			   libClLogClient.a

# Third party libs needed to get asp_logd built:

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
CPPFLAGS +=  -I ../idl/Log/xdr -I ../idl/Log/client/
CPPFLAGS +=  -I ../idl/Log/server -I ../idl/Log/common
CPPFLAGS +=  -I ../idl/App/client/ -I ../idl/App/common
CPPFLAGS +=  -I ../common/
CPPFLAGS +=  -I ../include/
CPPFLAGS +=  -I ../
EXTRA_CPPFLAGS  := -DCL_DEBUG_START

# System libraries needed to get asp_logd built:

# Executable name:
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)

include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
