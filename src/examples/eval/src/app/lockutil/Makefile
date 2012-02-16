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
#
# Makefile
#
############################# Description #####################################
#
#
# Build the lockutil utility.  Lockutil is intended to manipulate the lock
# status of service groups (SGs) without having to use the debug CLI
#
###############################################################################

# Component name
COMPNAME := lockutil

# List of local source files needed to build component
SRC_FILES := lockutil.c

# List of other ASP client libraries that the utility depends on
# TODO: trim this list down, if possible
DEP_COMP_LIST := utils osal timer buffer ioc rmd eo cnt debug event ckpt dbal\
amf om cor log idl name gms ams cpm

# List any additional header file locations
# This is what the lockutil.c code requires.
AMF_PATH := $(CLOVIS_ROOT)/ASP/components/amf
EXTRA_CPPFLAGS := -I../ev/ -I $(AMF_PATH)/common/ams 

# Unfortunately all the header files that lockutil.c includes are a lot
# more lazy about what they #include.  They all need to have the complete
# set of directories for their header files completely spelled out.  So...
#EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/ASP/components/amf/include
#EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/ASP/components/amf/common/ams
#EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/ASP/components/log/include
#EXTRA_CPPFLAGS += -I $(AMF_PATH)/common/ams/parser
#EXTRA_CPPFLAGS += -I $(AMF_PATH)/common/ams
#EXTRA_CPPFLAGS += -I $(AMF_PATH)/server/ams
#EXTRA_CPPFLAGS += -I $(CLOVIS_ROOT)/ASP/components/rmd/include

# List any third party libraries needed to get this util built
# TODO: try to eliminate this, if possible
THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils
THIRDPARTY_LIBS := -L$(THIRD_PARTY_UTILS_PATH) -l3rdpartyutils

# List ASP libraries needed to get this util built
# TODO: try to trim this list down, if possible
#                libClAmsServerStandalone.a  \

ASP_LIBS	:= libClMain.a \
			 libClTimer.a  \
            libClBuffer.a  \
            libClOsal.a  \
            libClCnt.a  \
             \
             \
             \
            libClEo.a  \
             \
             \
             \
             \
            libClIoc.a \
            libClRmd.a \
            libClUtils.a \
            libClAmfClient.a \
             \
             \
             \
            libClDbal.a \
            libClCkpt.a \
            libClEventClient.a \
            libClTxnAgent.a  \
            libClLogClient.a \
            libClCorClient.a \
            libClDebugClient.a \
            libClIdl.a  \
             \
            libClNameClient.a \
            libClAmsMgmt.a \
            libClAmsXdr.a \
              \
              \



# List any "extra" system libraries needed to get this server built
# We need libpthread?  Yes, apparently we need to run as a asp
# application which means we need to have a thread for listening
# for asp communication.  At least this is the way the debug
# application runs
SYS_LIBS := -lpthread -ldl

#
# Executable name
EXE_NAME = $(ASP_EXE_PREFIX)_$(COMPNAME)

include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
