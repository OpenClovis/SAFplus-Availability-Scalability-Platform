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
# ModuleName  : cor
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

COMPNAME	:= cor

COR_PATH 	:= $(CLOVIS_ROOT)/ASP/components/cor

# List local source files needed for the component server:

SRC_FILES	:=     $(wildcard $(COR_PATH)/common/idl/xdr/*.c) \
				   clCorAttrPath.c              \
                   clCorClientInit.c            \
                   clCorMoClassPath.c           \
                   clCorMoIdToSlotTableClient.c \
                   clCorMoIdToNodeNameTableClient.c \
                   clCorObjTreeApi.c            \
                   clCorRouteApis.c             \
                   clCorTxnJobStream.c          \
                   clCorClassApis.c             \
                   clCorEventClient.c           \
                   clCorMoId.c                  \
                   clCorMOTreeApi.c             \
                   clCorProvDataClient.c        \
                   clCorTxnClient.c             \
                   clCorTxnImpl.c               \
                   clCorUtils.c			\
                   clCorExpr.c                  \
                   clCorLog.c                   \
                   clCorClientDebug.c           \
                   clCorAmf.c \
                   clCorAmfEntities.c \
                   clCorSimpleApi.c \
                   clCorAlarm.c \


# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer cnt ioc rmd eo amf  om event debug log txn idl name

# Library name:

LIB_NAMES		= libClCorClient
CPPFLAGS        = -I ../common/idl/xdr/
CPPFLAGS        += -I ../server

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

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
