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
#
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
################################################################################

# Component name (using all lowercase):
COMPNAME	:= Amsxdr 

AMF_PATH        = $(CLOVIS_ROOT)/SAFplus/components/amf

# List local source files needed for the component server:

SRC_FILES	:= $(wildcard *.c)

# List other SAFplus components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc cnt eo debug rmd idl name amf event

EXTRA_CPPFLAGS  += -I$(AMF_PATH)/common/ams -I$(AMF_PATH)/server/ams

# Library name:

LIB_NAMES	= libClAmsXdr

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
