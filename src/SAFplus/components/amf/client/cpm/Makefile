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
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################

##### Component name (using all lowercase):

COMPNAME	:= amf

##### Source files for the client of this component

AMF_PATH = $(CLOVIS_ROOT)/SAFplus/components/amf
IDL_PATH = $(CLOVIS_ROOT)/SAFplus/components/idl

SRC_FILES :=                                                \
		   $(wildcard *.c)                                  \
		   $(wildcard $(AMF_PATH)/common/cpm/*.c)           \
		   $(wildcard $(AMF_PATH)/common/cpm/idl/xdr/*.c)   \
		   $(wildcard $(IDL_PATH)/common/*.c)               \

DEP_COMP_LIST	:= cor utils osal timer buffer cnt ioc rmd eo debug ckpt \
                    event amf log gms idl name cor om

CPPFLAGS += -I../../../include
CPPFLAGS += -I../../common/ams
CPPFLAGS += -I../../client/ams
CPPFLAGS += -I../../common/cpm
CPPFLAGS += -I../../common/cpm/idl/xdr
CPPFLAGS += -I../../server/cpm
CPPFLAGS += -I../../server/ams
CPPFLAGS += -DCL_CPM_AMS
CPPFLAGS += -DCL_CPM_GMS

##### Libraries built for this component
LIB_NAMES	= libClAmfClient 

SRC_FILES_libClAmfClient =                                  \
		   $(wildcard *.c)                                  \
		   $(wildcard $(AMF_PATH)/common/cpm/*.c)           \
		   $(wildcard $(AMF_PATH)/common/cpm/idl/xdr/*.c)   \
		   $(wildcard $(IDL_PATH)/common/*.c)               \


include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
