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
# ModuleName  : gms
# File        : Makefile
################################################################################
# Description :
#
# Makefile for GMS component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
#
################################################################################

# Subdirectories having Makefiles and need recursive escalation:

SUBDIRS          = client

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs. Example:
# server: common
# client: common
#
#server: client


EXTRA_CLEAN	=
EXTRA_CFLAGS	=
EXTRA_CPPFLAGS	=
EXTRA_LDFLAGS   =

include $(CLOVIS_ROOT)/SAFplus/mk/make-subdir.mk
