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
# ModuleName  : event
# $File: //depot/dev/main/Andromeda/SAFplus/components/event/test/unit-test/emAutomatedTestSuite/emApp/Makefile $
# $Author: bkpavan $
# $Date: 2006/09/14 $
################################################################################
# Description :
#
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#       - CLOVIS_ROOT environment variable is specified properly
#       - Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
#
################################################################################
                                                                                                                             
# Component name (using all lowercase):
                                                                                                                             
COMPNAME        := emTestApp
                                                                                                                             
# List local source files needed for the component server:
                                                                                                                             
SRC_FILES       := emTestApp.c\
        ../common/emTestPatterns.c\
		emTestAppMaster.c
# List other SAFplus components of which client libraries your component
# depends on:
                                                                                                                             
DEP_COMP_LIST   := utils osal timer buffer ioc rmd eo event amf  debug log
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST   += cnt
                                                                                                                             
CPPFLAGS += -I$(CLOVIS_ROOT)/SAFplus/components/event/common
CPPFLAGS += -I$(CLOVIS_ROOT)/SAFplus/components/include
CPPFLAGS += -I$(CLOVIS_ROOT)/SAFplus/components/event/client
# List SAFplus libraries needed to get this server built (or None):
                                                                                                                             
ASP_LIBS       := libClTimer.a \
                   libClBuffer.a \
                   libClOsal.a \
                   libClCnt.a \
                   libClEo.a \
                   libClIoc.a \
                   libClRmd.a \
                   libClEventClient.a \
                   libClUtils.a \
                   libClAmfClient.a\
                   libClDebugClient.a\
                   libClLogClient.a \
                   libClCorClient.a \
                   libClIdl.a \
                   libClNameClient.a \
                                                                                                                             
# List any third party libs needed to get this server built (or None):
                                                                                                                             
THIRDPARTY_LIBS := None
                                                                                                                             
# List any system libraries needed to get this server built (or None):
                                                                                                                             
SYS_LIBS        := -lpthread
                                                                                                                             
# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
                                                                                                                             
EXE_NAME        = $(ASP_EXE_PREFIX)_$(COMPNAME)
                                                                                                                             
include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk

