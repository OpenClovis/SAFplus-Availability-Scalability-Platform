###############################################################################
#
# Makefile
#
############################# Legal Notice ####################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All rights reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been deposited
# with the U.S. Copyright office.
#
# All Rights Reserved. US Government Users Restricted Rights - Use,
# duplication or disclosure restricted unless granted by formal written
# contract with OpenClovis Inc.
#
############################# Description #####################################
#
# $Id: //depot/dev/main/Andromeda/ASP/3rdparty/ezxml/Makefile#2 $
# $DateTime: 2006/09/14 14:31:51 $
# $Change: 22279 $
# $Author: bkpavan $
#
################################################################################

LIBUNWIND_DIR = libunwind-trunk
#
# We define an empty SUBDIRS for all recursive targets:
#
SUBDIRS :=

#
# If target is clean, we allow recursive escalation since ezxml-0.8.5
# supports this target.
#
ifeq "$(MAKECMDGOALS)" "clean"
    SUBDIRS := $(LIBUNWIND_DIR)
endif

#
# Instead of relying on the recursive make-subdir.mk feature, we
# explicitely dive into the directory:
#
# We build the library in the 'make libs' phase.
#
libs:
	$(Q)$(MAKE) -C $(LIBUNWIND_DIR) all install

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk
