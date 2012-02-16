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
# ModuleName  : amf
# File        : Makefile
################################################################################
# Description :
################################################################################

# Subdirectories having Makefiles and need recursive escalation:
# common should always be before client and server

SUBDIRS = ams cpm

ifeq ($(CL_INST),1)
	CPPFLAGS += -DCL_INST
endif

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs. Example:
# server: common
# client: common
#
# We have no dependencies in this case.

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk
