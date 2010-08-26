################################################################################
#
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
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
# Build: 4.0.0
#
################################################################################
# ModuleName  : snmp
# File        : Makefile
################################################################################
# Description :
################################################################################

##### Component name (using all lowercase):

COMPNAME	:= snmp

SNMP_PATH := $(CLOVIS_ROOT)/ASP/components/snmp

##### Source files for the client of this component

SRC_FILES := $(wildcard *.c)

#DEP_COMP_LIST	:= cor utils osal timer buffer cnt ioc rmd eo debug ckpt \
                    event amf log gms idl name cor om
DEP_COMP_LIST   := utils osal timer buffer cnt ioc rmd eo debug log med cor txn \
		   om alarm event

CFLAGS += -I $(SNMP_PATH)/include
NETSNMP_CFLAGS= $(shell net-snmp-config --base-cflags)
NETSNMP_CFLAGS+= -DNETSNMP_NO_INLINE
CFLAGS += $(NETSNMP_CFLAGS)

LDFLAGS += $(shell net-snmp-config --libs)
LDFLAGS += $(shell net-snmp-config --agent-libs)

##### Libraries built for this component
LIB_NAMES	= libClSnmpClient

SRC_FILES_libClSnmpClient = $(wildcard *.c)

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
