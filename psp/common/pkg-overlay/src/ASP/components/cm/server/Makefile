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
# ModuleName  : cm
# File        : Makefile
################################################################################
# Description :
#
# Makefile for Chassis manager component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= cm

# List local source files needed for the component server:

SRC_FILES	:= clCmMain.c \
               clCmHpiInterface.c\
               clCmDbgCli.c \
               clCmEventInterface.c\
               clCmUtils.c\
               clCmFruHotSwapSm.c \
               clCmHpiDebug.c
#../../cor/config/cmhpiModel.c 


# List other ASP components of which client libraries your component
# depends on:
DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo   cor om amf cm ../3rdparty/openhpi event debug alarm txn log

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
DEP_COMP_LIST	+= cnt

CPPFLAGS += -I./ -DHAVE_CONFIG_H -I../common/include -Wunused-function

# CPPFLAGS += -DCL_CM_SERVER_DEBUG -DCL_CM_HS_DEBUG 

# Uncomment the following line to log all saHpi API function calls
# CPPFLAGS += -DCL_CM_HPI_CALL_DEBUG

ifeq ($(HCL_CM),1) 
    CPPFLAGS += -DHCL_CM
endif	

# List ASP libraries needed to get this server built (or None):

ASP_LIBS   :=      libClMain.a \
                   libClEo.a \
                   libClLogClient.a\
				   libClAmfClient.a \
				   libClCorClient.a \
				   libClEventClient.a\
				   libClDebugClient.a\
                                   libClNameClient.a\
                                   libClIdl.a \
			           libClRmd.a \
				   libClCnt.a \
				   libClUtils.a \
				   libClIoc.a \
				   libClOsal.a \
				   libClBuffer.a \
				   libClCm.a\
				   libClTimer.a 

# List any third party libs needed to get this server built (or None):
# $(OPENHPILIBS) contains the value returned by pkg-config --libs openhpi openhpiutils
ifeq ($(HCL_CM),1)
	THIRDPARTY_LIBS += -L$(TOOLCHAIN_DIR)/lib -lhcl -lopenhpiutils
	EXTRA_CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/openhpi -I$(TOOLCHAIN_DIR)/include/radisys $(OPENHPICFLAGS)
else
	THIRDPARTY_LIBS += $(OPENHPILIBS)
	EXTRA_CPPFLAGS += $(OPENHPICFLAGS)
endif
THIRDPARTY_LIBS	+= -L$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable -lezxml 

# List any system libraries needed to get this server built (or None):


SYS_LIBS	:= 

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
