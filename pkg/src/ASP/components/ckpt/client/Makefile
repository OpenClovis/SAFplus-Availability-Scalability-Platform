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
# ModuleName  : ckpt                                                            
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
################################################################################

# Component name (using all lowercase):

COMPNAME	:= ckpt

CKPT_PATH       :=$(CLOVIS_ROOT)/ASP/components/ckpt

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard *.c) \
                   ../common/clCkptCmnUtils.c \
                   $(CKPT_PATH)/idl/ckptClntEo/server/ckptClntEockptClntckptClntServer.c \
                   $(CKPT_PATH)/idl/ckptClntEo/server/ckptClntEoServer.c \
                   $(CKPT_PATH)/idl/ckptClntEo/client/ckptClntEoClient.c \
		   $(wildcard $(CKPT_PATH)/idl/ckptEo/client/*.c)\
		   $(wildcard $(CKPT_PATH)/idl/ckptEo/xdr/*.c)\

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo event amf dbal debug log idl name
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt
DEP_COMP_LIST	+= dbal 

# Library name:
CPPFLAGS       := -I ../idl/ckptClntEo/common -I ../idl/ckptClntEo/client -I ../idl/ckptClntEo/xdr -I ../idl/ckptClntEo/server
CPPFLAGS       += -I ../idl/ckptEo/common -I ../idl/ckptEo/client -I ../idl/ckptEo/xdr  -I ../idl/ckptEo/server
CPPFLAGS       += -I ../common 

ifeq ($(CL_INST),1)
    CPPFLAGS += -DCL_INST
endif

LIB_NAMES	= libClCkpt

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
