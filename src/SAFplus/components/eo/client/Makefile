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
# ModuleName  : eo
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
include $(CLOVIS_ROOT)/SAFplus/mk/preface.mk
# Component name (using all lowercase):

COMPNAME	:= eo

# List local source files needed for the component server:

SRC_FILES	:= eo.c clEo.c clEoMain.c clEoParser.c clEoLibs.c clEoEvent.c clSAClientSelect.c

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal ckpt timer buffer rmd ioc cnt amf debug log idl gms event cor

AMF_PATH = $(CLOVIS_ROOT)/ASP/components/amf
CPPFLAGS += -I $(AMF_PATH)/common/cpm/idl/xdr -I$(AMF_PATH)/server/cpm -I$(AMF_PATH)/server/ams -I$(AMF_PATH)/common/cpm -I$(AMF_PATH)/common/ams $(CM_CFLAGS)

ifeq ($(CL_INST),1)
	CPPFLAGS += -DCL_INST
endif

ifeq ($(EO_QUEUE_STATS),1)
	CPPFLAGS += -DEO_QUEUE_STATS
endif

#Third party Libs
THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_EZXML_PATH)

# Library name:

LIB_NAMES	= libClEo

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
