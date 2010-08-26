###############################################################################
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
#
###############################################################################
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
