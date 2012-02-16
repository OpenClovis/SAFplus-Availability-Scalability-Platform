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
# ModuleName  : gms
# File        : Makefile
################################################################################
# Description :
#
# Makefile for tmpcomp component server
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= gms

# List local source files needed for the component server:

SRC_FILES	:= clGmsEo.c \
		   clGms.c \
		   clGmsDb.c \
		   clGmsTrack.c \
		   clGmsCli.c \
		   clGmsView.c \
		   clGmsRmdServer.c \
                   clGmsApiHandler.c \
                   clGmsUtils.c\
                   clGmsLog.c\
                   clGmsConf.c\
                   clGmsEngine.c \
				   clGmsCkpt.c

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo amf  cnt debug log gms ckpt

# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:

# List ASP libraries needed to get this server built (or None):

ASP_LIBS	:= libClTimer.a \
                   libClBuffer.a \
                   libClOsal.a \
                   libClEo.a \
                   libClIoc.a \
                   libClRmd.a \
                   libClUtils.a \
                   libClAmfClient.a \
                   libClCnt.a \
                   libClNameClient.a \
                   libClIdl.a \
                   libClDebugClient.a \
                   libClCorClient.a \
                   libClEventClient.a \
                   libClLogClient.a \
                   libClCkpt.a \
                   libClDbal.a \
                   libaisexec.a \
                   libtotem_pg.a\
				   libClGms.a \
		   libClMain.a 
ifdef SOLARIS_BUILD
   ASP_LIBS	+= liblogsys.a
endif
                   
# List any third party libs needed to get this server built (or None):
ifdef SOLARIS_BUILD
   OPENAIS_DIR=openais-0.82
else
   OPENAIS_DIR=openais-0.80.3
endif

OPENAIS_OBJDIR = $(ASP_TARGET)/obj/3rdparty/openais/$(OPENAIS_DIR)/exec

THIRDPARTY_LIBS	:= -L$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable/ -lezxml \
                   -L$(CLOVIS_ROOT)/ASP/3rdparty/openais/$(OPENAIS_DIR)/exec \
                   
ifeq ($(TARGET_QNX), 0)
ifeq ($(TARGET_VXWORKS), 0)
THIRDPARTY_LIBS += -ldl -lgdbm
endif
endif

ifdef SOLARIS_BUILD
  THIRDPARTY_LIBS += $(CLOVIS_ROOT)/ASP/3rdparty/openais/openais-0.82/exec/objdb.o \
                      $(CLOVIS_ROOT)/ASP/3rdparty/openais/openais-0.82/exec/aisparser.o 
else
  THIRDPARTY_LIBS += $(OPENAIS_OBJDIR)/objdb.o \
                   $(OPENAIS_OBJDIR)/aisparser.o \
                   $(OPENAIS_OBJDIR)/vsf_ykd.o
endif

# ezxml includes & paths

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable


EXTRA_CPPFLAGS  := -I$(THIRD_PARTY_EZXML_PATH) \
                   -I$(CLOVIS_ROOT)/ASP/3rdparty/openais/$(OPENAIS_DIR) \
                   -I$(CLOVIS_ROOT)/ASP/components/include/ipi  \
                   -I$(CLOVIS_ROOT)/ASP/components/event/include  \
                   -I$(CLOVIS_ROOT)/ASP/components/cor/include  \
                   -I$(CLOVIS_ROOT)/ASP/components/ioc/client  \
                   -DCL_GMS_SERVER

ifndef SOLARIS_BUILD

ifeq ($(OPENAIS_TIPC),1)
	EXTRA_CPPFLAGS += -DOPENAIS_TIPC
endif
endif

# List any system libraries needed to get this server built (or None):



# Executable name (when building server exectuable):
# Notice the '=' in the assignment.

EXE_NAME	= $(ASP_EXE_PREFIX)_$(COMPNAME)


include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
