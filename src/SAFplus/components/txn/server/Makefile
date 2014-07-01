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
# ModuleName  : txn                                                             
# File        : Makefile
################################################################################
# Description :
# 
# Makefile for transaction component server library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= txn

# List local source files needed for the component server:

TXN_PATH        = $(CLOVIS_ROOT)/ASP/components/txn

SRC_FILES	:= clTxnAppMain.c                                   \
                   clTxnEngine.c                                    \
                   clTxnSm.c                                        \
                   clTxnJobSm.c                                     \
                   clTxnService.c                                   \
                   $(TXN_PATH)/common/clTxnCommonUtil.c             \
                   $(TXN_PATH)/common/clTxnStreamIpi.c              \
                   $(TXN_PATH)/common/clTxnDb.c                     \
                   $(TXN_PATH)/common/clTxnCommIfc.c                \
                   clTxnCliMain.c                                   \
                   clTxnServiceCkpt.c                               \
                   clTxnRecoveryImpl.c                              \
                   $(wildcard $(TXN_PATH)/common/idl/xdr/*.c)       \


# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= osal timer buffer rmd eo ioc cnt utils debug sm  amf ckpt log

CFLAGS += -I../common/idl/xdr

# List ASP libraries needed to get this server built (or None):

ASP_LIBS       := libClTxnAgent.a      \
                   libClTxnClient.a     \
                   libClCkpt.a          \
                   libClTimer.a         \
                   libClRmd.a           \
                   libClAmfClient.a     \
                   libClDebugClient.a   \
                   libClLogClient.a \
                   libClDbal.a          \
                   libClEo.a            \
                   libClSm.a            \
                   libClIoc.a           \
                   libClCnt.a           \
                   libClBuffer.a        \
                   libClUtils.a         \
                   libClOsal.a          \
                   libClEventClient.a   \
                   libClCorClient.a     \
                   libClIdl.a           \
                   libClNameClient.a    \
				   libClIdlPtr.a		\
				   libClMain.a


# Library name:

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

EXE_NAME        = $(ASP_EXE_PREFIX)_$(COMPNAME)

include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
