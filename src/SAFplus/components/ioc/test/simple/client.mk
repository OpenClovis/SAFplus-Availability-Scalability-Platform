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
#
# Makefile for cssa701App sample application
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
# $Id: //depot/test/performance-test/ioc/Makefile#1 $
# $DateTime: 2006/07/11 17:59:39 $
# $Change: 21099 $
# $Author: susheel $
#
################################################################################


# Component name (using all lowercase):
COMPNAME	:=      iocClient

# List local source files needed for the component server:
SRC_FILES       :=  client.c

DEP_COMP_LIST	:= 	utils osal timer buffer cnt ioc eo rmd
 

ASP_LIBS       := 	libClTimer.a \
                   	libClBuffer.a \
                   	libClOsal.a \
                   	libClCnt.a \
                   	libClUtils.a \
                   	libClIoc.a \
                        libClRmd.a \
                        libClEo.a \
                        libClLogClient.a \
                        libClAmfClient.a \
                        libClCorClient.a \
                        libClIdl.a \
                        libClNameClient.a \
                        libClEventClient.a \
                        libClDebugClient.a \
 
                   
SYS_LIBS	:= 	-lpthread

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
EXE_NAME       	= 	$(COMPNAME)

include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
