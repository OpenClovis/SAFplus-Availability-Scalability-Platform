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
# ModuleName  : 
# File        : make-kernel.mk
################################################################################
# Description :
# 
#  Kernel make include for all ASP directories. Use this makefile to
#  define variables to use for a kernel module build.
#
#  Interface:
#
#  This file does not expect anything to be defined beforehand.
#  
#  When this file is included, the following are then defined.
#  
#  KERNEL_SOURCE: the full path to the kernel.
#
#  KERNEL_MAJOR_VERSION: Major version of the linux kernel (ie 2 for 2.6.10)
#  KERNEL_MINOR_VERSION: Minor version of the linux kernel (ie 6 for 2.6.10)
#  KERNEL_PATCH_VERSION: Patch version of the linux kernel (ie 10 for 2.6.10)
#
#  This file also sets CFLAGS appropriately to do a kernel module
#  build. Once this file in included in your module's Makefile, you
#  only have to define the make rules to build the module, whether it
#  is for kernel 2.6.x or 2.4.x.
################################################################################
include $(CLOVIS_ROOT)/ASP/mk/prefix.mk

ifndef CROSS_BUILD
# Native build
KERNEL_SOURCE = /lib/modules/`uname -r`/build
KERNEL_HEADERS := $(shell ls -d /usr/src/*-`uname -r`/include 2>/dev/null)
ifneq ($(KERNEL_HEADERS),)
CFLAGS += -I$(KERNEL_HEADERS)
endif
#KERNELVERSION = $(shell uname -r | cut -c 1)
#KERNELPATCHLEVEL = $(shell uname -r | cut -c 3)
else				# CROSS_BUILD
# cross build
KERNEL_HEADERS :=
KERNEL_SOURCE = $(KERNEL_PATH)
CROSS_COMPILE = $(TARGET)-
export CROSS_COMPILE

endif				# CROSS_BUILD

# To get the kernel version we are building for, we read the kernel
# version file version.h. This file is always present in the linux
# kernel (thus this is linux specific). From that we extract the
# values.
KERNEL_VERSION_FILE := $(KERNEL_SOURCE)/include/linux/version.h

EXISTS := $(shell ls $(KERNEL_VERSION_FILE))
ifeq ($(EXISTS),)
$(error "Error : Please install kernel headers")
endif

# rshift is a gawk specific feature, so I'm explicitly calling gawk instead of the normal awk
KERNEL_MAJOR_VERSION = $(shell gawk '$$2=="LINUX_VERSION_CODE"{print rshift($$3,16)}' $(KERNEL_VERSION_FILE))
KERNEL_MINOR_VERSION = $(shell gawk '$$2=="LINUX_VERSION_CODE"{print and(rshift($$3,8),0xff)}' $(KERNEL_VERSION_FILE))
KERNEL_PATCH_VERSION = $(shell gawk '$$2=="LINUX_VERSION_CODE"{print and($$3,0xff)}' $(KERNEL_VERSION_FILE))

# This is a kernel build, typically building modules, so set these
# defines. Kernel also typically builds -O2.
# -isystem $(KERNEL_SOURCE)/include does not work because it puts system includes after gcc-lib includes and there is a common file
# posix_types.h that we need to get from the kernel area (or you get a features.h: no such fail or directory error on some distros, debian, ubuntu)
CFLAGS += -I$(KERNEL_SOURCE)/include -O2 -DMODULE -D__KERNEL__ -DEXPORT_SYMTAB
export CFLAGS

# Version checks. Major number has to be 2.
ifneq ($(KERNEL_MAJOR_VERSION),2)
$(error "Error : Cannot compile with this kernel version $(KERNEL_MAJOR_VERSION)")
endif

ifeq ($(KERNEL_MINOR_VERSION),6)
# Define 2.6 specific items here.
CFLAGS += -D__KERNEL_2_6__
endif

ifeq ($(KERNEL_MINOR_VERSION),4)
# Define 2.4 specific items here.
endif

# For now, do nothing for splint
splint:
