###############################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
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
#
###############################################################################
################################################################################
# ModuleName  : log
# $File: //depot/dev/main/Andromeda/Bhadra/ASP/components/log/client/Makefile $
# $Author: ramesh $
# $Date: 2007/04/04 $
################################################################################
# Description :
#
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= tcLogClient 

# List local source files needed for the component server:

SRC_FILES := clLogTest.c \

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd  cnt eo dbal debug amf \
                   name idl cor ckpt event
# FIXME: Indirect dependencies via client library needs need to be
# added by hand for now:
#DEP_COMP_LIST	+= dbal 

# Library name:

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH) 

#CFLAGS   += -E	
LIB_NAMES	= libClTcLogClient

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

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
