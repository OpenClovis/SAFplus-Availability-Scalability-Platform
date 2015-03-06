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
# ModuleName  : txn
# File        : Makefile
################################################################################
# Description :
#
# Makefile for transaction common directory
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
# This is a specialized Makefile and it need to provide _ALL_ the
# following make targets:
#       all (should be the default)
#       depend
#       
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= txn

# list local source files needed for this common library:

SRC_FILES       := clTxnCommonUtil.c  clTxnStreamIpi.c

# List other SAFplus components of which client libraries this component
# depends on:

DEP_COMP_LIST   := osal timer ioc rmd buffer cnt utils

# Library name:
LIB_NAMES       = libClTxnCommon


# make-empty.mk provides the default targets and does nothing else
include $(CLOVIS_ROOT)/SAFplus/mk/make-empty.mk
