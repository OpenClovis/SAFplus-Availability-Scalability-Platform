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
# ModuleName  : common
# File        : make-cross.mk
################################################################################
# Description :
#  Cross compilation make include for all ASP directories
#
#  Note that ASP Makefiles typically don't directly include this
#  file, but include it via make-subdir.mk, make-client.mk,
#  make-server.mk or make-kernel.mk
#
#  Interface:
#
#  The following variables may be defined to control this file:
#
#  CROSS_BUILD          <name of toolchain directory under
#                        /opt/clovis/buildtools to use for the build>
#
#  KERNEL               (Optional) Name of kernel source tree under
#                       /opt/clovis/buildtools/$(CROSS_BUILD)/src.
#                       If not specified, the default "linux" link
#                       will be used.
#
#  All bianry build tools, header files, and libraries will be picked up
#  from the selected subdirectory.
#
#  In addition, the defintions of many Makefile variables will be also
#  obtained from the XXX file in the specified toolchain directory.
#
#  When this file is included, the following are then defined.
#  
#  KERNEL_PATH:         the full path to the kernel source tree
#
#  CC, GCC, LD, AR:     the path to the right binary to use
#
#  CFLAGS:              to contain system file references
#  LDFLAGS:             to contain system library references
#
#  LDFLAGS:             to contain additional -m options
#
#  PATH:                path to the toolchain's bin directory is added
#
#  NET_SNMP_CONFIG:     env variable to define where the net-snmp-config
#                       program is found
#
################################################################################

buildtools_dir:=/opt/clovis/buildtools

#
# If it is a native build, just leave everythin normal
# FIXME: This should not even be here:
#
ifndef CROSS_BUILD

# This is the native build
CC  ?= gcc
GCC ?= $(CC)
AR  ?= ar
LD  ?= ld
NET_SNMP_CONFIG = net-snmp-config

MACHINE = $(shell uname -m)
ifeq ($(MACHINE),ppc)
LDFLAGS += -m elf32ppclinux
else
# we are now assuming it is ia32, if it is not ppc. this may also have
# to change.
LDFLAGS += -m elf_i386
endif				# LDFLAGS

#
# If it is not native build then get the toolchains from the
# appropriate path.
#
else # ifndef CROSS_BUILD

# Need to define: ARCH, MARCH, 

BUILDTOOLS_PATH:=       $(buildtools_dir)/$(CROSS_BUILD)

export PATH:=$(PATH):$(BUILDTOOLS_PATH)/bin

# Pick up the ARCH, MACH, and TARGET variables from the config.mk file from
# the toolchain.

#
# FIXME: Need to test of file exists, and hsout if it does not.
#
ifneq ($(wildcard $(BUILDTOOLS_PATH)/config.mk),) 
include $(BUILDTOOLS_PATH)/config.mk
endif

#$(warning $(ARCH))
#$(warning $(MACH))
#$(warning $(TARGET))

KERNEL          ?= linux
KERNEL_PATH      = $(BUILDTOOLS_PATH)/src/$(KERNEL)
CFLAGS          += -isystem $(KERNEL_PATH)/include \
                   -I$(BUILDTOOLS_PATH)/include
LDFLAGS         += -L $(BUILDTOOLS_PATH)/lib
CC               = env ARCH=$(ARCH) \
                   CROSS_COMPILE=$(BUILDTOOLS_PATH)/bin/ \
                   $(BUILDTOOLS_PATH)/bin/$(TARGET)-gcc
GCC              = $(CC)
AR               = env ARCH=$(ARCH) \
                   CROSS_COMPILE=$(BUILDTOOLS_PATH)/bin/ \
                   $(BUILDTOOLS_PATH)/bin/$(TARGET)-ar
LD               = env ARCH=$(ARCH) \
                   CROSS_COMPILE=$(BUILDTOOLS_PATH)/bin/ \
                   $(BUILDTOOLS_PATH)/bin/$(TARGET)-ld $(LDFLAGS)
NET_SNMP_CONFIG  = $(BUILDTOOLS_PATH)/bin/net-snmp-config

#$(warning $(KERNEL_PATH))
#$(warning $(CFLAGS))
#$(warning $(CC))
#$(warning $(GCC))
#$(warning $(AR))
#$(warning $(LD))
#$(warning $(NET_SNMP_CONFIG))

ifeq ($(ARCH),ppc)
    LDEMULATION = elf32ppclinux
endif

ifeq ($(ARCH),i386)
    LDEMULATION = elf_i386
endif

ifndef LDEMULATION
    $(error "Building on $(ARCH) is not supported yet")
endif

LDFLAGS += -m $(LDEMULATION)

#$(warning $(LDFLAGS))
#$(warning $(LDEMULATION))

export LDEMULATION

endif # ifndef CROSS_BUILD

all:

.PHONY: crosshelp
crosshelp:
	@echo '  CROSS_BUILD=<toolchain-name> '
	@echo '                      - Triggers a host-independent build using the specified' 
	@echo '                        toolchain.  The following toolchains are available'
	@echo '                        at your host:'
	@(cd $(buildtools_dir); ls | awk '{print "                       ",$$1}')
	@echo '  KERNEL=<kernel-src-dir-name> '
	@echo '                      - Name of an alternative kernel under the given'
	@echo '                        toolchain.  See directories under the'
	@echo '                        /opt/clovis/buildtools/<CROSS_BUILD>/src directory'
	@echo '                        for alternative kernel choices.  The default is'
	@echo '                        the "linux" symbolic link.'

# End of the file
