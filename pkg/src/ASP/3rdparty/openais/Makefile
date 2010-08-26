################################################################################
#
# Makefile for building ezxml from our make hierarchy
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
# $Id: //depot/dev/main/Andromeda/ASP/3rdparty/openais/Makefile#2 $
# $DateTime: 2006/09/14 14:31:51 $
# $Change: 22279 $
# $Author: bkpavan $
#
################################################################################

ifdef SOLARIS_BUILD
   OPENAIS_DIR = openais-0.82
else
   OPENAIS_DIR = openais-0.80.3
endif

$(warning $(OPENAIS_DIR))

#
# We define an empty SUBDIRS for all recursive targets:
#
#SUBDIRS := openais-0.82

#
# If target is clean, we allow recursive escalation since ezxml-0.8.4
# supports this target.
#
ifeq "$(MAKECMDGOALS)" "clean"
   SUBDIRS := $(OPENAIS_DIR)
endif

#
# Instead of relying on the recursive make-subdir.mk feature, we
# explicitely dive into the directory:
#
# We build the library in the 'make libs' phase.
#
libs:
	$(Q)$(MAKE) -C $(OPENAIS_DIR)/exec lib

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk
