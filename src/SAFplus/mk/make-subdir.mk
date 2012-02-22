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
#
# File        : make-subdir.mk
#
# Description :
#
#  Make include for sub-directory excalation
#
#  This also manages the two-pass make escalation used in building ASP:
#  - First pass builds all libraries using the 'libs' target.
#  - Second pass builds all other binaries (executables)
#
#  Interface:
#
#  When this file is included, the following variables are expeced to be
#  defined:
#  Variable           Optional  Values          Explanation
#  ----------------------------------------------------------------------
#  SUBDIRS		mand.	list of subdirs List of subdirs to escalate to
#
#  V			opt.	0 or 1		Sets the verbosity level
#						(only from command line)
#  BUILD_VERBOSE	opt.	0 or 1		Same as above.
#						Can be set anywhere.
#  EXTRA_CFLAGS
#  EXTRA_CPPFLAGS
#  EXTRA_LDFLAGS
#  EXTRA_LDLIBS
#
#  EXTRA_CLEAN
#  
#  Targets defined by this .mk:
#	clean
#	distclean
#	libs
#	all
#	depend
#	cscope
#	tags
#	TAGS
#
################################################################################

include $(CLOVIS_ROOT)/SAFplus/mk/make-path.mk

.PHONY: libs depend $(SUBDIRS)

################################################################################
# Escalate 'make all' or 'make'
#
all: libs $(SUBDIRS)
$(SUBDIRS):
	$(Q)$(MAKE) -C $@

################################################################################
# Escalate 'make clean'
#
CLEAN = $(addsuffix .clean,$(SUBDIRS))
.PHONY: clean $(CLEAN)
clean: $(CLEAN)
$(CLEAN):
	$(Q)$(MAKE) -C $(basename $@) clean

# Escalate 'make distclean'
#
DISTCLEAN = $(addsuffix .distclean,$(SUBDIRS))
.PHONY: distclean $(DISTCLEAN)
distclean: $(DISTCLEAN)
$(DISTCLEAN):
	$(Q)$(MAKE) -C $(basename $@) distclean

################################################################################
# Escalate 'make depend'
#
DEPEND = $(addsuffix .dep,$(SUBDIRS))
.PHONY: depend $(DEPEND)
depend: $(DEPEND)
$(DEPEND):
	$(Q)$(MAKE) -C $(basename $@) depend

################################################################################
# Escalate 'make libs'
#
# Make sure 'make libs' is executed before a make all is executed
#
MAKELIBSFIRST = $(addsuffix .libs,$(SUBDIRS))
.PHONY: libs $(MAKELIBSFIRST)
libs: $(MAKELIBSFIRST)
$(MAKELIBSFIRST):
	$(Q)$(MAKE) -C $(basename $@) libs

# Escalate deploy but make clean and all first .
# Current deploy is still out of the makefile
# Reason : Inputs are both dynamic and static 
#		so we run a shell script instead
# TODO:
# Python port in RC2 
################################################################################
# Escalate 'make deploy'
#
DEPLOY = $(addsuffix .deploy,$(SUBDIRS))
.PHONY: deploy $(DEPLOY)
deploy: libs $(DEPLOY)
$(DEPLOY):
	$(Q)$(MAKE) -C $(basename $@) deploy

################################################################################
# Escalate 'make splint'
#
SPLINT = $(addsuffix .splint,$(SUBDIRS))
.PHONY: splint $(SPLINT)
splint: $(SPLINT)
$(SPLINT):
	$(Q)$(MAKE) -C $(basename $@) splint

################################################################################
GCC_VERSION := $(shell gcc -dumpversion)
GCC_MAJOR_VERSION := $(shell gcc -dumpversion | cut -f1 -d.)
GCC_MINOR_VERSION := $(shell gcc -dumpversion | cut -f2 -d.)
GCC_MAJOR_VERSION_MAX_RANGE := $(shell expr 5 \<= $(GCC_MAJOR_VERSION))
GCC_MAJOR_VERSION_MIN_RANGE := $(shell expr 4 \== $(GCC_MAJOR_VERSION))
GCC_MINOR_VERSION_MIN_RANGE := $(shell expr 5 \< $(GCC_MINOR_VERSION))

SPECIAL_CFLAGS =
SPECIAL_LDFLAGS =

ifeq ("$(GCC_MAJOR_VERSION_MIN_RANGE)", "1")
    ifeq ("$(GCC_MINOR_VERSION_MIN_RANGE)", "1")
        SPECIAL_CFLAGS = -Wno-error=unused-but-set-variable
        SPECIAL_LDFLAGS =,--no-as-needed
    endif
endif

ifeq ("$(GCC_MAJOR_VERSION_MAX_RANGE)", "1")
    SPECIAL_CFLAGS = -Wno-error=unused-but-set-variable
    SPECIAL_LDFLAGS =,--no-as-needed
endif

export SPECIAL_CFLAGS SPECIAL_LDFLAGS

export CC
export AR
export BIN_DIR
export OBJ_DIR
export LIB_DIR
export CLOVIS_ROOT

include $(CLOVIS_ROOT)/SAFplus/mk/make-common.mk
