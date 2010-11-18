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
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################

##### Component name (using all lowercase):

COMPNAME	:= amf
AMF_PATH        = $(CLOVIS_ROOT)/ASP/components/amf

##### Source files for the server of this component

SRC_FILES	:= clAmsEntities.c		\
		   clAmsDatabase.c		\
		   clAmsPolicyEngine.c		\
		   clAms.c			\
		   clAmsModify.c 		\
		   clAmsMgmtServerApi.c 	\
		   clAmsEventServerApi.c	\
                   clAmsSAServerApi.c           \
                   clAmsMgmtServerRmd.c         \
                   clAmsDebugCli.c              \
                   clAmsMgmtDebugCli.c          \
                   clAmsDBPackUnpack.c      	\
                   clAmsCkpt.c      		\
                   clAmsDefaultConfig.c   	\
                   clAmsInvocation.c   	        \
		   clAmsNotifications.c		\
		clAmsEntityTrigger.c\
		clAmsEntityTriggerRmd.c\
        clAmsMgmtMigrate.c\
	                   $(AMF_PATH)/server/ams/clAmsServerUtils.c  \
        clAmsEntityUserData.c\
		mPlusN.c
	    	
DEP_COMP_LIST	:= utils osal timer buffer eo ioc rmd cnt debug name gms event \
                   ckpt cor alarm fault om log cm amf

##### Libraries built for this component

LIB_NAMES       = libClAmsServer 

##### Third party libs needed to get this server built

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils/

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH)	\
		   -L$(THIRD_PARTY_UTILS_PATH)	\
		   -lezxml			\
		   -l3rdpartyutils

##### Include paths for AMS server code
ifeq ($(HCL_CM),1)
EXTRA_CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/radisys
else
EXTRA_CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/openhpi
endif

# Workaround as openhpi is not part of install package
ifdef SOLARIS_BUILD
EXTRA_CPPFLAGS += -I$(CLOVIS_ROOT)/ASP/3rdparty/Solaris/openhpi/include
endif

EXTRA_CPPFLAGS  += -I$(THIRD_PARTY_EZXML_PATH)		\
		   -I$(THIRD_PARTY_UTILS_PATH)		\
                   -I$(AMF_PATH)/common/cpm		\
		   -I$(AMF_PATH)/common/ams		\
		   -I$(AMF_PATH)/common/ams/parser	\
                   -I$(AMF_PATH)/server/cpm		\
		   -I$(AMF_PATH)/common/cpm/idl/xdr 	\
                   -I$(AMF_PATH)/common/ams/idl/xdr     \

##### Defines for controlling AMS server side functionality
#
# EO			: Enable integration with EO code
# AMS_CKPT_TEST		: AMS checkpoint test code for testing without standby system controller
# CL_AMS_RANK_LIST	: Enable ranked list 

#EXTRA_CPPFLAGS  += -DAMS_CKPT_TEST
EXTRA_CPPFLAGS  += -DCL_AMS_RANK_LIST

##### System libraries needed to get this server built (or None)

ifdef SOLARIS_BUILD
   SYS_LIBS	:= -lpthread -L/usr/local/lib -lgdbm
else
   SYS_LIBS	:= -lpthread -lgdbm
endif


include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
