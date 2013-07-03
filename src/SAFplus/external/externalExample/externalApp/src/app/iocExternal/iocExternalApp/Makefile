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


# This is to build tipc without C Standards 99....
export BUILD_WITHOUT_C99=1


# Component name (using all lowercase):

KERNEL_DIRE     :=      /lib/modules/$(shell uname -r)/build
COMPNAME	:=      externalIoc

# List local source files needed for the component server:

SRC_FILES       :=  iocExternalMain.c iocExternalReceiver.c iocExternalSender.c

#DEP_COMP_LIST	:= 	utils osal timer buffer cnt ioc eo rmd 
 

ASP_LIBS	:= libexternal.a \
 
                   
#SYS_LIBS	:= 	-lpthread
EXTRA_CFLAGS += -I$(KERNEL_DIRE)/include -I$(TIPC_INC_DIR) -I$(KERNEL_DIRE)/$(TIPC_INC_DIR) -D__EXPORTED_HEADERS__

# Executable name (when building server exectuable):
# Notice the '=' in the assignment.
EXE_NAME       	= 	$(COMPNAME)

include $(CLOVIS_ROOT)/ASP/mk/make-server.mk
