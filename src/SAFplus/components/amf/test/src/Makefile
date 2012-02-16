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
# ModuleName  : amf/test
# File        : Makefile
################################################################################
# Description :
################################################################################

THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils/
AMF_PATH                = $(CLOVIS_ROOT)/ASP/components/amf


##### Component name (using all lowercase):

COMPNAME	:= amf

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_UTILS_PATH) -l3rdpartyutils
EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_UTILS_PATH) -I$(AMF_PATH)/common/ams
EXTRA_CPPFLAGS  += -I$(AMF_PATH)/server/ams
EXTRA_CPPFLAGS  += -I$(AMF_PATH)/test/include -I$(AMF_PATH)/common/cpm
#EXTRA_CPPFLAGS  += -DCL_AMS_DEBUG_PRINT

##### Source files for the client of this component

SRC_FILES	:= $(wildcard *.c)

##### ASP libraries needed to get this client built

DEP_COMP_LIST	:= utils osal timer buffer cnt ioc rmd eo debug amf cm cor log event ckpt


##### Libraries built for this component

LIB_NAMES	=  libClAmsTest 


SRC_FILES_libClAmsTest	= $(SRC_FILES)

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
