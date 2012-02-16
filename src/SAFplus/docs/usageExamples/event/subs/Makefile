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
# $File: //depot/dev/main/Andromeda/Yamuna/ASP/integration-test/event/subs/Makefile $
# $Author: amitg $
# $Date: 2006/10/09 $
################################################################################
# Description :
#
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#       - CLOVIS_ROOT environment variable is specified properly
#       - Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################
                                                                                                                             
# Component name (using all lowercase):
                                                                                                                             
COMPNAME        := eventSubscriber
                                                                                                                             
# List local source files needed for the component server:
                                                                                                                             
SRC_FILES       := clEvtSubsMain.c
                                                                                                                             
# List other ASP components of which client libraries your component
# depends on:
                                                                                                                             
DEP_COMP_LIST   := utils osal timer buffer cnt ioc rmd eo event amf dbal debug
                                                                                                                             
# List ASP libraries needed to get this server built (or None):
                                                                                                                             
ASP_LIBS       := libClTimer.a \
                   libClBuffer.a \
                   libClOsal.a \
                   libClCnt.a \
                   libClEo.a \
                   libClIoc.a \
                   libClRmd.a \
				   libClEventClient.a \
				   libClDebugClient.a \
                   libClUtils.a \
                   libClAmfClient.a \
                   libClLogClient.a \
                   libClIdl.a \
                   libClNameClient.a \
                   libClCorClient.a \
                                                                                                                             
# List any third party libs needed to get this server built (or None):

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

THIRDPARTY_LIBS := -L$(THIRD_PARTY_EZXML_PATH) -lezxml

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)                                                                                                                                                                                                                                                        
# List any system libraries needed to get this server built (or None):
                                                                                                                             
SYS_LIBS        := -lpthread
                                                                                                                             
# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
                                                                                                                             
EXE_NAME        = $(ASP_EXE_PREFIX)_$(COMPNAME)
                                                                                                                             
include $(CLOVIS_ROOT)/ASP/mk/make-server.mk

