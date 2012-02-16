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
# ModuleName  : fault                                                           
# File        : Makefile
################################################################################
# Description :
# 
# Makefile for alarm component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= fault

FAULT_PATH        = $(CLOVIS_ROOT)/ASP/components/fault

# List local source files needed for the component server:

SRC_FILES	:= clFaultMain.c \
			$(FAULT_PATH)/common/clFaultClientServerCommons.c \
			clFaultHistory.c \
			clFaultRecovery.c \
			clFaultDebug.c \
			clFaultServerCmds.c 	\
			$(FAULT_PATH)/idl/xdr/xdrClFaultVersionInfoTUnMarshall.c


# List other ASP components of which client libraries your component
# depends on:
DEP_COMP_LIST	:= utils osal timer debug event buffer ioc rmd eo   cor\
alarm fault om amf name log txn idl

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt  

EXTRA_CFLAGS	:= -I$(FAULT_PATH)/idl/xdr -I$(FAULT_PATH)/client
 	
CPPFLAGS += -I./ 

# List ASP libraries needed to get this server built (or None):
ASP_LIBS	:=	libClTimer.a \
				libClBuffer.a \
				libClOsal.a \
				libClIoc.a \
				libClUtils.a \
				libClCnt.a \
				libClEo.a \
				libClCorClient.a \
				libClRmd.a \
				libClAmfClient.a \
				libClDebugClient.a \
				libClLogClient.a \
				libClFaultClient.a \
				libClNameClient.a \
				libClIdl.a \
				libClEventClient.a \
				libClNameClient.a \
                libClAlarmClient.a \
				libClAlarmXdr.a \
                libClOmClient.a\
				libClMain.a



# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
     

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
		   
# List any system libraries needed to get this server built (or None):


# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
