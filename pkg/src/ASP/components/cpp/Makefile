################################################################################
#
#   Copyright (C) 2002-2008 by OpenClovis Inc. All Rights  Reserved.
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
# Build: 3.0.0
#
################################################################################
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################

##### Component name (using all lowercase):

COMPNAME	:= amf

##### Source files for the client of this component

SRC_FILES	:= clCheckpoint.cxx  clEoApp.cxx  clSaApp.cxx  clMsgServer.cxx

##### ASP libraries needed to get this client built

DEP_COMP_LIST	:= cpp utils osal timer buffer cnt ioc eo amf rmd debug log ckpt

##### Libraries built for this component

LIB_NAMES	= libClAspCpp
LIB_NAMES       = libClSafAspCpp

SRC_FILES_libClAspCpp		:= clCheckpoint.cxx  clSaApp.cxx  clMsgServer.cxx
SRC_FILES_libClSafAspCpp	:= clCheckpoint.cxx  clEoApp.cxx  clMsgServer.cxx


# List any third party libs needed to get this server built (or None):

#THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
#AMF_PATH               = $(CLOVIS_ROOT)/ASP/components/amf

#EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_EZXML_PATH) -I$(AMF_PATH)/server/ams -I$(AMF_PATH)/common/cpm
#EXTRA_CPPFLAGS  += -I$(AMF_PATH)/common/ams 

BUILD_WITHOUT_C99 := 1

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
