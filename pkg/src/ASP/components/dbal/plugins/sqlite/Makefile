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
# ModuleName  : dbal
# File        : Makefile
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

COMPNAME	:= dbal

# List local source files needed for the component server:

SRC_FILES	:= clovisSQLite.c 

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer cnt debug dbal eo rmd ioc

SRC_FILES_libClSQLiteDB      :=  clovisSQLite.c
PURIFY_SHARED_LIB_NAMES := libClSQLiteDB
                                        
ifdef SOLARIS_BUILD
EXTRA_SHARED_LDFLAGS=-L$(ASP_LIB) -lClTimer -lClBuffer -lClOsal -lClCnt \
                     -lClUtils -L/usr/local/lib -lsqlite3 -L$(MODEL_LIB)\
            -L$(COMP_ROOT)/ground/client -lClGround
else
EXTRA_SHARED_LDFLAGS=-Wl,--start-group -L$(ASP_LIB) -lClTimer -lClBuffer -lClOsal -lClCnt \
						     -lClUtils -lsqlite3 -L$(MODEL_LIB)\
						-Wl,--end-group \
			-L$(COMP_ROOT)/ground/client -lClGround
endif

EXTRA_SHARED_LDFLAGS+=-L$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable -lezxml 
EXTRA_SHARED_CPPFLAGS= -I$(THIRD_PARTY_EZXML_PATH)

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
CPPFLAGS += -I ../../common/

SYS_LIBS := -ldl -L/usr/local/lib -lsqlite3

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
include $(CLOVIS_ROOT)/ASP/mk/make-client.mk

