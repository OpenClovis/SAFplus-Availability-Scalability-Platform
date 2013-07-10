################################################################################
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
# ModuleName  : ckpt
# File        : Makefile
################################################################################
# Description :
################################################################################

THIRD_PARTY_UTILS_PATH = $(CLOVIS_ROOT)/SAFplus/3rdparty/utils/


##### Component name (using all lowercase):

#COMPNAME	:= ut_ckpt

### Other libs needed
EXTRA_CPPFLAGS := -I../include


#THIRDPARTY_LIBS	:= -L$(THIRD_PARTY_UTILS_PATH) -l3rdpartyutils

##### Source files for the client of this component

SRC_FILES	:= clTcCkptApi.c

##### SAFplus libraries needed to get this client built

DEP_COMP_LIST	:= utils osal timer buffer cnt ioc rmd eo debug amf cm cor log ckpt

##### Libraries built for this component

LIB_NAMES	=  libClTcCkpt 

SRC_FILES_libClTcCkpt	= clTcCkptApi.c


include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
