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
# ModuleName  : Makefile
# File        : Makefile
################################################################################
# Description :
################################################################################

PROGS = nodeinfo
EXE_FLAGS = -m 0755

.PHONY : all clean

all:
	$(shell mkdir -p $(MODEL_BIN))
	$(shell install $(PROGS) $(EXE_FLAGS) $(MODEL_BIN))

lib:

include $(CLOVIS_ROOT)/ASP/mk/make-common.mk

clean:
	$(shell for f in $(PROGS); do rm -f $(MODEL_BIN)/$$f; done)
