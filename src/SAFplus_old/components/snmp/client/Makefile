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
# ModuleName  : snmp
# File        : Makefile
################################################################################
# Description :
################################################################################

##### Component name (using all lowercase):

COMPNAME	:= snmp

SNMP_PATH := $(CLOVIS_ROOT)/SAFplus/components/snmp

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

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
