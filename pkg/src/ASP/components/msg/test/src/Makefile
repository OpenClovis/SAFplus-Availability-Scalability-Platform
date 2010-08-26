################################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
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
# ModuleName  : msgTest
# File        : Makefile
################################################################################
# Description :
################################################################################

THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/ASP/3rdparty/utils/


##### Component name (using all lowercase):

### Other libs needed
EXTRA_CPPFLAGS := -I../include


#THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_UTILS_PATH) -l3rdpartyutils

##### Source files for the client of this component

SRC_FILES	:= msgTest.c

##### ASP libraries needed to get this client built

DEP_COMP_LIST	:= amf msg

##### Libraries built for this component

LIB_NAMES	=  libClMsgTest 

SRC_FILES_libClMsgTest	= msgTest.c


include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
