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
# ModuleName  : message 
# File        : Makefile
################################################################################
# Description :
# 
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#   - CLOVIS_ROOT environment variable is specified properly
#   - Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
################################################################################

# Component name (using all lowercase):

MSG_PATH := $(CLOVIS_ROOT)/ASP/components/msg

COMPNAME	:= msg

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard *.c)\
			   $(wildcard $(MSG_PATH)/common/*.c)\
			   $(wildcard $(MSG_PATH)/idl/msgClt/client/*.c)\
			   $(wildcard $(MSG_PATH)/idl/msgIdl/client/*.c)\
			   $(wildcard $(MSG_PATH)/idl/msgIdl/server/*.c)\
		       $(wildcard $(MSG_PATH)/idl/msgIdl/xdr/*.c) \


# List other ASP components of which client libraries your component
# depends on:
CPPFLAGS += -I ../common -I ../idl/msgIdl/xdr -I ../idl/msgIdl/client -I ../idl/msgIdl/server -I ../idl/msgIdl/common
CPPFLAGS += -I ../idl/msgClt/client -I ../idl/msgClt/server -I ../idl/msgClt/common

DEP_COMP_LIST	:= utils osal timer buffer cnt ioc rmd eo amf ckpt dbal debug log idl name
DEP_COMP_LIST	+= cor event

# List ASP libraries needed to get this server built (or None):

ASP_LIBS	:= libClTimer.a \
               libClBuffer.a \
               libClOsal.a \
               libClCnt.a \
               libClEo.a \
               libClIoc.a \
               libClRmd.a \
               libClUtils.a \
               libClAmfClient.a \
               libClDbal.a \
               libClCkpt.a\
               libClEventClient.a \
               libClDebugClient.a \
               libClLogClient.a \
               libClCorClient.a \
               libClIdl.a \
               libClNameClient.a \
			   libClMain.a \
			   

                   
# List any third party libs needed to get this server built (or None):


# List any system libraries needed to get this server built (or None):


# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
