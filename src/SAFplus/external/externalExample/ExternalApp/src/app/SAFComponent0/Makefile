################################################################################
#
# Makefile for cssa701App sample application
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
# $Id: //depot/test/performance-test/ioc/Makefile#1 $
# $DateTime: 2006/07/11 17:59:39 $
# $Change: 21099 $
# $Author: susheel $
#
################################################################################


# This is to build tipc without C Standards 99....
export BUILD_WITHOUT_C99=1


# Component name (using all lowercase):

KERNEL_DIRE     :=      /lib/modules/$(shell uname -r)/build
COMPNAME	:=      externalApp

# List local source files needed for the component server:

SRC_FILES := $(wildcard *.c) $(wildcard ./common/*.c)

DEP_COMP_LIST	:= 	utils osal timer buffer cnt ioc eo rmd external
 

ASP_LIBS := libmw.a
 
                   
#SYS_LIBS	:= 	-lpthread
EXTRA_CFLAGS += -I$(KERNEL_DIRE)/include -I$(TIPC_INC_DIR) -I$(KERNEL_DIRE)/$(TIPC_INC_DIR)  -I$(ASP_CONFIG) -I./common -D__EXPORTED_HEADERS__ 
BUILD_CPLUSPLUS = 1

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
EXE_NAME       	= 	$(COMPNAME)

include $(CLOVIS_ROOT)/SAFplus/mk/make-server.mk
