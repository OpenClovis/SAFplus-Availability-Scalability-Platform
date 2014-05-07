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
# ModuleName  : ioc                                                             
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
COMPNAME	:= ioc

# This is to build tipc without C99 Standards....
export BUILD_WITHOUT_C99=1

# List local source files needed for the component server:

SRC_FILES := $(wildcard *.c) $(wildcard ../common/*.c)

# List any third party libs needed to get this server built (or None):
CPPFLAGS       += -I$(COMP_ROOT)/include/ipi -I$(COMP_ROOT)/ioc/server -I$(COMP_ROOT)/ioc/common
CPPFLAGS       += -I$(COMP_ROOT)/amf/common/cpm
CPPFLAGS       += -I$(COMP_ROOT)/amf/server/cpm
CPPFLAGS       += -I$(COMP_ROOT)/amf/server/ams

CHECK_INOTIFY := $(shell ./check_inotify.sh $(TOOLCHAIN_DIR)/bin/$(TARGET)-gcc)
ifeq ($(strip $(CHECK_INOTIFY)), )
	EXTRA_CPPFLAGS += -DHAVE_INOTIFY
endif
#CPPFLAGS	   += -I$(TIPC_INCLUDE)
EXTRA_CPPFLAGS += -D__EXPORTED_HEADERS__ -I$(TIPC_INCLUDE)
EXTRA_CPPFLAGS += -I$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable


ifdef SOLARIS_BUILD
EXTRA_CPPFLAGS += -I/opt/SUNWtipc/include/net/tipc
endif

# List other ASP components of which client libraries your component
# depends on:
DEP_COMP_LIST	:= utils osal timer buffer cnt debug eo rmd amf event cor gms ckpt

# Library name:
LIB_NAMES	= libClIoc


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
