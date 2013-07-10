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
# ModuleName  : alarm                                                           
# $File: //depot/dev/main/Andromeda/Cauvery/SAFplus/components/alarm/utils/Makefile $
# $Author: naveen $
# $Date: 2007/05/17 $
################################################################################
# Description :
################################################################################

# Component name (using all lowercase):

COMPNAME	:= alarm

# List local source files needed for the component server:


SRC_FILES	:=  clAlarmUtil.c \
                clIdlOpen.c   \
                xdrClAlarmUtilPayLoadListTMarshall.c  \
                xdrClAlarmUtilPayLoadListTUnMarshall.c  \
                xdrClAlarmUtilPayLoadTMarshall.c  \
                xdrClAlarmUtilPayLoadTUnMarshall.c  \
                xdrClAlarmUtilTlvTMarshall.c  \
                xdrClAlarmUtilTlvTUnMarshall.c  \
                ../common/idl/xdr/xdrClCorMOHandleTMarshall.c  \
                ../common/idl/xdr/xdrClCorMOHandleTUnMarshall.c  \
                ../common/idl/xdr/xdrClCorMOIdTMarshall.c  \
                ../common/idl/xdr/xdrClCorMOIdTUnMarshall.c


# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd eo cor txn om cnt amf \
hal debug event prov log name idl

# Library name:

LIB_NAMES	= libClAlarmUtils

# List any third party libs needed to get this server built (or None):

EXTRA_CPPFLAGS += -I$(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable 
EXTRA_CPPFLAGS += -I../common/idl/xdr -I../client

# If there are more than one client libraries to be built here, you
# need to list the object files that will make up each library:
#
# Example:
#     libClTmpcomp_basic: file1.o file2.o
#     libClTmpcomp_advanced: file1.o file2.o file3.o file4.o
#
# If your component builds one library only, than this list will automatically
# obtained from the SRC_FILES variable above, so you don't need to create it.
#

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
