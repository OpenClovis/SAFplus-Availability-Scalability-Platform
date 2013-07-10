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
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= log

# List local source files needed for the component server:
LOG_PATH := $(CLOVIS_ROOT)/SAFplus/components/log



SRC_FILES := clLogClntWrap.c clLogClient.c clLogClientStream.c \
             clLogClientEo.c \
             clLogClientHandle.c \
             clLogClientCommon.c \
             clLogClientHandler.c \
             clLogClntFileHdlr.c \
             clLogClientDump.c\
             clLogStreamMutex.c\
             $(wildcard $(LOG_PATH)/idl/Log/client/*.c)\
             $(wildcard $(LOG_PATH)/idl/Log/xdr/*.c)\
             $(wildcard $(LOG_PATH)/idl/App/server/*.c)\
             $(wildcard $(LOG_PATH)/idl/App/client/*.c)\
             $(wildcard $(LOG_PATH)/idl/App/xdr/*.c)\
             $(wildcard $(LOG_PATH)/common/*.c)\


# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd  cnt eo dbal debug amf \
                   name idl cor ckpt event
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
#DEP_COMP_LIST	+= dbal 

# Library name:

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable
CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH) -I ../idl/Log/client/
CPPFLAGS += -I ../idl/Log/xdr/ -I ../idl/Log/common
CPPFLAGS += -I ../idl/App/xdr/ -I ../idl/App/common
CPPFLAGS += -I ../idl/Log/client
CPPFLAGS += -I ../idl/App/server/ -I ../idl/App/client
CPPFLAGS += -I ../server/
CPPFLAGS += -I ../
EXTRA_CPPFLAGS  := -DCL_DEBUG_START

#CFLAGS   += -E	
LIB_NAMES	= libClLogClient

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

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
