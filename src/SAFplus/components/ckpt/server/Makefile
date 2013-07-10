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
# ModuleName  : ckpt                                                            
# File        : Makefile
################################################################################
# Description :
# 
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
################################################################################

# Component name (using all lowercase):

COMPNAME	:= ckpt
CKPT_PATH       :=$(CLOVIS_ROOT)/SAFplus/components/ckpt

# List local source files needed for the component server:

SRC_FILES	:= clCkptLife.c  clCkptMain.c clCkptSaf.c clCkptUtils.c \
                   clCkptEvt.c \
                   clCkptMaster.c clCkptDeputy.c clCkptImport.c\
                   clCkptDataBackup.c\
                   clCkptMasterUtils.c  clCkptPeer.c \
		       ../common/clCkptCmnUtils.c clCkptSvrCmds.c\
		 $(wildcard $(CKPT_PATH)/idl/ckptEo/server/*.c)\
		 $(wildcard $(CKPT_PATH)/idl/ckptEo/xdr/*.c)\
		 $(wildcard $(CKPT_PATH)/idl/ckptEo/client/*.c)\
# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo amf debug event ckpt dbal log idl name gms
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt

# List SAFplus libraries needed to get this server built (or None):

ASP_LIBS	:= libClUtils.a \
                   libClLogClient.a \
                   libClDebugClient.a \
                   libClEventClient.a \
                   libClNameClient.a \
                   libClEo.a \
                   libClRmd.a \
                   libClIoc.a \
                   libClBuffer.a \
                   libClCnt.a \
                   libClTimer.a \
                   libClOsal.a\
                   libClIdl.a	\
                   libClCkpt.a\
                   libClDbal.a \
                   libClIdlPtr.a

# List any third party libs needed to get this server built (or None):
THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml 

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
CPPFLAGS += -I ../idl/ckptEo/common -I ../idl/ckptEo/client -I ../idl/ckptEo/xdr -I ../idl/ckptEo/server 
CPPFLAGS += -I ../idl/ckptClntEo/common -I ../idl/ckptClntEo/client -I ../idl/ckptClntEo/xdr -I ../idl/ckptClntEo/server 
CPPFLAGS += -DCL_CKPT_GMS
# List any system libraries needed to get this server built (or None):

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk
