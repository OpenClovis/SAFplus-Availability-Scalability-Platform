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
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

include $(CLOVIS_ROOT)/ASP/mk/preface.mk

# Component name (using all lowercase):

COMPNAME	:= cm

# List local source files needed for the component server:

SRC_FILES	:= clCmApis.c ../common/clCmCommons.c

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= utils osal timer buffer ioc rmd cor cnt ../3rdparty/openhpi eo event om debug  amf txn

CPPFLAGS += -I../common/include

CPPFLAGS += $(CM_CFLAGS)

# Library name:

LIB_NAMES	= libClCm 

ifeq ($(HCL_CM),1)
	#CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/radisys 
        EXTRA_CPPFLAGS += $(OPENHPICFLAGS)
else
	# This is a work around as openhpi compilation is not yet included 
	# the install package.
	ifdef SOLARIS_BUILD
		EXTRA_CPPFLAGS  += -I$(CLOVIS_ROOT)/ASP/3rdparty/Solaris/openhpi/include
        else
		#CPPFLAGS += -I$(TOOLCHAIN_DIR)/include/openhpi
	        EXTRA_CPPFLAGS += $(OPENHPICFLAGS)
	endif
endif
              

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
#CPPFLAGS += -DCL_CM_CLIENT_DEBUG  

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
