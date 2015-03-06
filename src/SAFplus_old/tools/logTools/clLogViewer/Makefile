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
# ModuleName  : logViewer
# File        : Makefile
################################################################################
# Description :
#
# Makefile for log component log viewer 
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
#
################################################################################

# Component name (using all lowercase):
COMPNAME	:=safplus_binlogviewerexec
# Local source files needed for the component log viewer:
LOG_PATH   :=$(CLOVIS_ROOT)/SAFplus/components/log/

UTILS_PATH = $(CLOVIS_ROOT)/SAFplus/components/utils

SRC_FILES	:=   $(wildcard *.c)

# Other SAFplus components on which asp_logViewer depends on 

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo amf idl name ckpt dbal debug log cnt
# Needed to run with P=1, could have added to make-common but then all other exes' size would have
#increased
# Other SAFplus libraries needed to get asp_logViewer

1_FLAGS	:= libClGDBM.a
ASP_LIBS   := $($(P)_FLAGS) libClCnt.a libClUtils.a libClBuffer.a libClOsal.a libClRmd.a libClIoc.a libClEo.a libClLogClient.a libClAmsXdr.a libClAmfClient.a libClTimer.a libClEventClient.a libClIdl.a libClIdlPtr.a libClNameClient.a libClCorClient.a libClDebugClient.a



CPPFLAGS += -I$(LOG_PATH)server
CPPFLAGS += -I$(LOG_PATH)common
CPPFLAGS += -I$(LOG_PATH)idl/Log/xdr/ -I$(LOG_PATH)idl/logEo/xdr/

# System libraries needed to get asp_logd built:

ifdef SOLARIS_BUILD
   SYS_LIBS	:= -lpthread -ldl -L/usr/local/BerkeleyDB.4.2/lib -ldb
else 
 ifeq ($(TARGET_QNX),1)
   SYS_LIBS	:=
 else
   SYS_LIBS	:= -lpthread  -ldl -ldb
 endif
endif

# Executable name:
# Notice the '=' in the assignment.

EXE_NAME	= $(COMPNAME)

include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk
