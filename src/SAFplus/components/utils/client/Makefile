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
# ModuleName  : utils
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

COMPNAME	:= utils

# List local source files needed for the component server:

SRC_FILES	:= clPool.c \
			clHeap.c \
			clMemStats.c \
			clBinSearch.c \
		   bit.c \
		   rule.c \
		   cksm.c \
           crcbsd.c \
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
               xdrClMD5TMarshall.c \
               xdrClMD5TUnMarshall.c \
               xdrClDataVectorTMarshall.c \
               xdrClDataVectorTUnMarshall.c \
               xdrClDifferenceVectorTMarshall.c \
               xdrClDifferenceVectorTUnMarshall.c \
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
               clPlugin.c \
	       clErrorTable.c \
	       md5.c\
	       clDifferenceVector.c

ifeq ($(TARGET_VXWORKS), 1)
SRC_FILES += clMemTrackerVxWorks.c clMemPart.c
else
SRC_FILES += clMemTracker.c
endif
# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= osal timer debug buffer eo rmd ioc cnt log amf cor event

# Library name:

LIB_NAMES	= libClUtils

EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

ifeq ("$(WIND_VER)","1.2")
    EXTRA_CPPFLAGS += -DWIND_PTHREAD
endif

ifeq ($(EO_QUEUE_STATS),1)
	EXTRA_CPPFLAGS += -DEO_QUEUE_STATS
endif

ifeq ($(CL_HEAP_CHECKER), 1)
	EXTRA_CFLAGS += -DCL_HEAP_CHECKER
endif


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
