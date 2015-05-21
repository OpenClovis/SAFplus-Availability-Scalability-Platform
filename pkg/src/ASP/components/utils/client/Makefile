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
# ModuleName  : utils
# File        : Makefile
################################################################################
# Description :
#
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= utils

# List local source files needed for the component server:

SRC_FILES	:= clPool.c \
			clHeap.c \
			clMemStats.c \
			clBinSearch.c \
		   bit.c \
		   rule.c \
		   cksm.c \
		   queue.c \
		   clist.c \
		   parser.c \
			clIocParseConfig.c		\
	           clOampRtParser.c \
                   clHandleApi.c \
                   clVersionApi.c \
	           xdrGeneric.c \
	           xdrClHandleT.c \
			   xdrClWordT.c \
	           xdrClInt8T.c \
	           xdrClUint8T.c \
	           xdrClCharT.c \
	           xdrClInt16T.c \
	           xdrClUint16T.c \
	           xdrClInt32T.c \
	           xdrClUint32T.c \
	           xdrClInt64T.c \
	           xdrClUint64T.c \
		   xdrClStringT.c \
               xdrClNameTMarshall.c \
               xdrClNameTUnMarshall.c \
               xdrClVersionTMarshall.c \
               xdrClVersionTUnMarshall.c \
               clBitmap.c			\
               clDbg.c                         \
               clTest.c                        \
               clDispatchApi.c \
               clStat.c\
               clLogUtils.c\
               clLogDbg.c\
               clCommon.c \
               clBackingStorage.c\
               clTaskPool.c \
               clJobQueue.c \
               clMgd.c \
               clMetric.c \
               clThreadPool.c \
               clSafUtils.c\
               rbtree.c\
               clRadixTree.c\
               clTimeServer.c\
               clLeakyBucket.c \
               clMemGroup.c \
               stackTrace.c \
				clErrorTable.c \

ifeq ($(TARGET_VXWORKS), 1)
SRC_FILES += clMemTrackerVxWorks.c clMemPart.c
else
SRC_FILES += clMemTracker.c
endif
# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= osal timer debug buffer eo rmd ioc cnt log amf cor event

# Library name:

LIB_NAMES	= libClUtils
EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
CFLAGS  += -I$(CLOVIS_ROOT)/ASP/3rdparty/libunwind/libunwind-trunk/include/

ifeq ("$(WIND_VER)","1.2")
    EXTRA_CPPFLAGS += -DWIND_PTHREAD
endif

ifeq ($(EO_QUEUE_STATS),1)
	EXTRA_CPPFLAGS += -DEO_QUEUE_STATS
endif

ifeq ($(CL_HEAP_CHECKER), 1)
	EXTRA_CFLAGS += -DCL_HEAP_CHECKER
endif

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
