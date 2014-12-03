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

##### Component name (using all lowercase):

COMPNAME    := 

##### Source files for the client of this component

SRC_FILES := $(wildcard *.c)
CPP_FILES := $(wildcard *.C) $(wildcard *.cxx) 

##### ASP libraries needed to get this client built

DEP_COMP_LIST   := cpp utils osal timer buffer cnt ioc eo amf rmd debug log ckpt

##### Libraries built for this component

LIB_NAMES       = libENTITYSTATETCMIB

SRC_FILES_libENTITYSTATETCMIB := $(wildcard *.cxx) $(wildcard *.C)

# List any third party libs needed to get this server built (or None):
THIRD_PARTY_PATH = /code/git/SAFplus7/src/../../mgt/3rdparty
THIRD_PARTY_TARGET := $(THIRD_PARTY_PATH)/build

XML2_INC_BASE := $(THIRD_PARTY_TARGET)/include/libxml2
XML2_INCLUDE  := -I$(XML2_INC_BASE) -I$(XML2_INC_BASE)/libxml

EXTRA_CPPFLAGS := $(XML2_INCLUDE) \
                 -I$(THIRD_PARTY_TARGET)/include

THIRDPARTY_LIBS += -L$(THIRD_PARTY_TARGET)/lib -lxml2

EXTRA_LDLIBS += 

CPPFLAGS += -I$(ASP_INCLUDE) \
            -I/code/git/SAFplus7/src/../../mgt/model/src/app/include \
            -I/code/git/SAFplus7/src/../../mgt/model/src/app/client

CPPFLAGS += -I/code/git/SAFplus7/src/SAFplus/components/amf7/server/SAFplusAmf \
            -I/code/git/SAFplus7/src/SAFplus/components/amf7/server/ENTITYSTATETCMIB \
            -I/code/git/SAFplus7/src/SAFplus/components/amf7/server/ietfYangTypes

BUILD_WITHOUT_C99 := 1
LDFLAGS += -lstdc++
BUILD_CPLUSPLUS = 1

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
