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

AMF_PATH        := $(CLOVIS_ROOT)/SAFplus/components/amf

##### Source files for the client of this component

SRC_FILES	:= clAmsParser.c $(AMF_PATH)/server/ams/clAmsDefaultConfig.c  \

##### SAFplus libraries needed to get this client built

DEP_COMP_LIST	:= utils osal timer buffer cnt ioc rmd eo debug amf log ckpt event

##### Libraries built for this component

LIB_NAMES	= libClAmsParser


SRC_FILES_libClAmsParser		= clAmsParser.c $(AMF_PATH)/server/ams/clAmsDefaultConfig.c

# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable
AMF_PATH               = $(CLOVIS_ROOT)/SAFplus/components/amf

EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_EZXML_PATH) -I$(AMF_PATH)/server/ams -I$(AMF_PATH)/common/cpm
EXTRA_CPPFLAGS  += -I$(AMF_PATH)/common/ams -I$(AMF_PATH)/server/cpm


include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
