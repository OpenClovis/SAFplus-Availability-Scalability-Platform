################################################################################
#
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
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
# Build: 4.2.0
#
################################################################################
# ModuleName  : Makefile
# File        : Makefile
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
                                                                                                                             
COMPNAME        := snmpAgent
                                                                                                                             
# List local source files needed for the server:
                                                                                                                             
SRC_FILES       := clSNMPSubAgentConfig.c
                                                                                                                             
# List other ASP components of which client libraries your component
# depends on:
                                                                                                                             
DEP_COMP_LIST   := utils osal dbal timer buffer cnt ioc rmd eo med cor name alarm om event  amf debug cm snmp

CPPFLAGS += -I$(CLOVIS_ROOT)/ASP/3rdparty/snmpagent/net-snmp-5.1/include/

# added by hand for now:
DEP_COMP_LIST   += cnt
                                                                                                                             
# List ASP libraries needed to get this server built (or None):

ASP_LIBS       := libClTimer.a \
                   libClBuffer.a \
                   libClOsal.a \
                   libClCnt.a \
                   libClEo.a \
                   libClIoc.a \
                   libClRmd.a \
                   libClAmfClient.a\
                   libClDebugClient.a\
                   libClUtils.a \
                   libClMedClient.a\
                   libClEventClient.a\
                   libClNameClient.a\
                   libClCorClient.a \
		   libClsnmpServer.a
                                                                                                                             
# List any third party libs needed to get this server built (or None):
# List any third party libs needed to get this server built (or None):
THIRD_PARTY_LIB_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/snmpagent/net-snmp-5.1/
                                                                                
AGENT_LIB_PATH = $(THIRD_PARTY_LIB_PATH)/agent/
AGENT_HELPER_PATH =  $(THIRD_PARTY_LIB_PATH)/agent/helpers/
APPS_PATH = $(THIRD_PARTY_LIB_PATH)/apps/
SNMPLIB_PATH =  $(THIRD_PARTY_LIB_PATH)/snmplib/
                                                                                
                                                                                
THIRDPARTY_LIBS := -L$(AGENT_LIB_PATH) -lnetsnmpmibs\
                    -L$(AGENT_LIB_PATH) -lnetsnmpagent\
                     -L$(AGENT_LIB_PATH) -lnetsnmpmibs\
			-L$(ASP_LIB) -lClsnmpServer\
                    -L$(AGENT_HELPER_PATH) -lnetsnmphelpers \
                    -L$(APPS_PATH) -lnetsnmptrapd\
                    -L$(SNMPLIB_PATH) -lnetsnmp
                                                                                
LDLIBS                  += $(THIRDPARTY_LIBS)
# List any system libraries needed to get this server built (or None):
                                                                                
SYS_LIBS        := -lpthread


                                                                                                                             
THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

#THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml
                                                                                                                             
# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
                                                                                                                             
EXE_NAME        = $(ASP_EXE_PREFIX)_$(COMPNAME)
                                                                                                                             
include $(CLOVIS_ROOT)/ASP/mk/make-server.mk

