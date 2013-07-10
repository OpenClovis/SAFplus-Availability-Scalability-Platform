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

COMPNAME	:= none

##### Source files for the client of this component

SRC_FILES	:= clCheckpoint.cxx  clApp.cxx clMsgServer.cxx clAppEvent.cxx

##### SAFplus libraries needed to get this client built

DEP_COMP_LIST	:= cpp utils osal timer buffer cnt ioc eo amf rmd debug log ckpt

##### Libraries built for this component

#LIB_NAMES	= libClAspCpp
LIB_NAMES       = libmwcpp

#SRC_FILES_libClAspCpp		:= clCheckpoint.cxx  clSaApp.cxx  clMsgServer.cxx
SRC_FILES_libClSafAspCpp	:= clCheckpoint.cxx clApp.cxx


# List any third party libs needed to get this server built (or None):

#THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable
#AMF_PATH               = $(CLOVIS_ROOT)/SAFplus/components/amf

#EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_EZXML_PATH) -I$(AMF_PATH)/server/ams -I$(AMF_PATH)/common/cpm
#EXTRA_CPPFLAGS  += -I$(AMF_PATH)/common/ams 

BUILD_WITHOUT_C99 := 1

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
