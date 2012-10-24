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
# ModuleName  : clUdp
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

COMPNAME	:= clUdp

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard *.c)

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer cnt debug eo rmd ioc

SRC_FILES_libClUDP      := $(SRC_FILES)
PURIFY_SHARED_LIB_NAMES := libClUDP

ifdef SOLARIS_BUILD
EXTRA_SHARED_LDFLAGS=-L$(ASP_LIB) -lmw $(LIB_BERKELEY_DB) -L$(MODEL_LIB) -L$(COMP_ROOT)/ground/client -lClGround
else
EXTRA_SHARED_LDFLAGS=-Wl,--start-group -L$(ASP_LIB) -lmw -L$(MODEL_LIB) -Wl,--end-group -L$(COMP_ROOT)/ground/client -lClGround
endif

CHECK_SCTP := $(shell ./check_sctp.sh $(TOOLCHAIN_DIR)/bin/$(TARGET)-gcc)
ifeq ($(strip $(CHECK_SCTP)), )
	EXTRA_CPPFLAGS += -DHAVE_SCTP
endif

THIRD_PARTY_EZXML_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable

EXTRA_SHARED_LDFLAGS+=-L$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable -lezxml 
EXTRA_SHARED_CPPFLAGS= -I$(THIRD_PARTY_EZXML_PATH)

THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_EZXML_PATH) -lezxml

CPPFLAGS += -I$(THIRD_PARTY_EZXML_PATH)
CPPFLAGS += -I ../../common/ -I ../../client
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
include $(CLOVIS_ROOT)/ASP/mk/make-client.mk

