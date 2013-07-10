################################################################################
#
# Makefile for tmpcomp component client library
#
# This Makefile assumes the following:
#	- CLOVIS_ROOT environment variable is specified properly
#	- Entire source tree under $(CLOVIS_ROOT)/SAFplus is checked out
#
# $Id: //depot/dev/main/Andromeda/ASP/3rdparty/utils/Makefile#2 $
# $DateTime: 2006/09/14 14:31:51 $
# $Change: 22279 $
# $Author: bkpavan $
#
################################################################################

# Component name (using all lowercase):

COMPNAME	:= 3rdpartyutils

# List local source files needed for the component server:

SRC_FILES	:= crc.c
                   
# List other ASP components of which client libraries your component
# depends on:

DEP_COMP_LIST	:= 

# Library name:

LIB_NAMES	= lib3rdpartyutils

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

include $(CLOVIS_ROOT)/SAFplus/mk/make-client.mk
