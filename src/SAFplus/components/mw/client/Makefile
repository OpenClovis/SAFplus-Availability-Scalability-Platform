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
# ModuleName  : mw 
# File        : Makefile
################################################################################
# Description :
#
# Makefile for ASP standard client library
# This library internally links to all client libraries of ASP
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/ASP is checked out
#
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= mw 

# List local source files needed for the component server:

SRC_FILES	:= 

# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= ground \
             cnt    \
             buffer \
             osal   \
             timer  \
             utils  \
             ioc    \
             rmd    \
             idl    \
             dbal   \
             eo     \
             debug  \
             amf    \
             alarm  \
             event  \
             ckpt   \
             name   \
             om     \
             cor    \
             fault  \
             sm     \
             txn    \
			 med	\
			 mso    \
			 prov 	\
			 hal 	\
			 gms	\
			 log    \
			 msg    \
			 pm     \
             ntf

# If the chassis manager exists then use it
USING_CM := $(wildcard $(CLOVIS_ROOT)/../../psp/src/cm)
ifneq ($(USING_CM),)
$(warning Using the chassis manager from the Platform Support Package located at $(USING_CM))
CL_CM := -lClCm
else
$(warning Not using the chassis manager)
CL_CM := 
endif

ALL_ASP_LIBS    :=         -lClTimer -lClBuffer -lClOsal -lClIoc -lClUtils -lClEo -lClMsoClient -lClProv \
			   -lClLogClient -lClTxnClient -lClTxnAgent -lClSm -lClCnt -lClIdl -lClRmd \
			   -lClCorClient -lClAmfClient -lClAmsMgmt -lClAmsXdr -lClDebugClient -lClFaultClient \
		           -lClEventClient -lClDbal -lClGms $(CL_CM) -lClCkpt -lClMedClient -lClNameClient \
			   -lClUtils -lClIdlPtr -lClAlarmClient -lClAlarmUtils -lClAlarmXdr -lClOmClient -lClMsgClient \
			   -lClPMClient -lClNtf -l3rdpartyutils -lClGroundClient 

ifdef SOLARIS_BUILD
EXTRA_LDLIBS    += -L$(ASP_LIB)  $(ALL_ASP_LIBS) 
else
EXTRA_LDLIBS	+= -L$(ASP_LIB) -Wl,--start-group $(ALL_ASP_LIBS) -Wl,--end-group 
endif

# Library name:

LIB_NAMES	= libmw

# If there are more than one client libraries to be built here, you
# need to list the object files that will make up each library:
#
# Example:
#     libClTmpcomp_basic: file1.o file2.o
#     libClTmpcomp_advanced: file1.o file2.o file3.o file4.o
#
# If your component builds one library only, than this list will automatically
# obtained from the SRC_FILES variable above, so you don't need to create it.
#

include $(CLOVIS_ROOT)/ASP/mk/make-client.mk
