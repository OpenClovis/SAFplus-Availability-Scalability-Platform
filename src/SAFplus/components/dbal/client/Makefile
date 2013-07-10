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
# ModuleName  : dbal
# File        : Makefile
################################################################################
# Description :
#
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= dbal

# List local source files needed for the component server:

SRC_FILES	:= clovisDbal.c \
                   clDbalCfg.c\
                   clDbalParse.c\
#                   clovisBerkeleyDB.c
#		   clovisSOLID.c \
#		   clovisDummyDB.c \

ifeq ($(BUILD_SHARED), 0)
 SRC_FILES += ../plugins/berkeley/clovisBerkeleyDB.c
endif
# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer cnt debug eo rmd ioc


#SRC_FILES_libClGDBM       =  clovisGDBM.c
#SRC_FILES_libClBerkeleyDB =  clovisBerkeleyDB.c
#SRC_FILES_libClSOLID      =  clovisSOLID.c
#SHARED_LIB_NAMES = libClGDBM\
#                   libClBerkeleyDB\
#                    libClSOLID 
# Library name:

LIB_NAMES	= libClDbal 
		#  libclDummy \
		#  libclBerkeley

ifdef SOLARIS_BUILD
   EXTRA_SHARED_LDFLAGS=-L$(ASP_LIB) -lClTimer -lClBuffer -lClOsal -lClCnt -lClUtils -L$(MODEL_LIB)
else
   EXTRA_SHARED_LDFLAGS=-Wl,--start-group -L$(ASP_LIB) -lClTimer -lClBuffer -lClOsal -lClCnt -lClUtils -L$(MODEL_LIB) -Wl,--end-group
endif

EXTRA_SHARED_LDFLAGS+=-L$(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable -lezxml 
EXTRA_SHARED_CPPFLAGS= -I$(THIRD_PARTY_EZXML_PATH)

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
CPPFLAGS += -I ../common
#		  libclSOLID.a

SYS_LIBS  := -ldl
# If there are more than one client libraries to be built here, you
# need to list the source files that will make up each library:
#
# Example:
#     SRC_FILES_libClTmpcomp_basic.a: file1.c file2.c
#     SRC_FILES_libClTmpcomp_advanced.a: file1.c file2.c file3.c file4.c
#
# If your component builds one library only, than this list will automatically
# obtained from the SRC_FILES variable above, so you don't need to create it.
#
SRC_FILES_libClDbal		= clovisDbal.c clDbalCfg.c clDbalParse.c 
ifeq ($(BUILD_SHARED), 0)
 SRC_FILES_libClDbal += ../plugins/berkeley/clovisBerkeleyDB.c
endif
#SRC_FILES_libclDummy		= clovisDummyDB.c
#SRC_FILES_libclGDBM 		= clovisGDBM.c
#SRC_FILES_libclBerkeley	        = clovisBerkeleyDB.c
#SRC_FILES_libclSOLID		= clovisSOLID.c

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk

