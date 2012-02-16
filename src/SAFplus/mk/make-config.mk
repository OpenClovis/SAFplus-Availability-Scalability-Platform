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
# ModuleName  : 
# File        : make-config.mk
################################################################################
# Description :
#
#  Make include for building (client) libraries
#
#  If SUBDIRS are defined, this mk can also manage the two-pass make
#  into such subdirectories:
#  - First pass builds all libraries using the 'libs' target.
#  - Second pass builds all other binaries (executables)
#
#  Interface:
#
#  When this file is included, the following variables are expeced to be
#  defined:
#  Variable           Optional  Values          Explanation
#  ----------------------------------------------------------------------
#  COMPNAME		mand.   Name of component
#  SRC_FILES		mand.   List of source (*.c) files
#  DEP_COMP_LIST	mand.   List of component names this server depend on
#  LIB_NAMES		mand.   List of libraries to build
#
#  SUBDIRS		opt.	list of subdirs List of subdirs to escalate to
#
#  V			opt.	0 or 1		Sets the verbosity level
#						(only from command line)
#  BUILD_VERBOSE	opt.	0 or 1		Same as above.
#						Can be set anywhere.
#  EXTRA_CFLAGS
#  EXTRA_CPPFLAGS
#  EXTRA_LDFLAGS
#  EXTRA_LDLIBS
#
#  EXTRA_CLEAN
#  
#  Targets defined by this .mk:
#	clean
#	distclean
#	libs
#	all
#	depend
#	cscope
#	tags
#	TAGS
#
################################################################################

include $(CLOVIS_ROOT)/ASP/mk/make-path.mk

################################################################################
# Generating object file list from SRC_FILES list:
# file1.c           --->  $(OBJ_DIR)/file1.o
# ../common/file2.c --->  $(OBJ_DIR)/file2.o
# ../config/file3.c --->  $(OBJ_DIR)/file3.o
obj_files		:= $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_FILES:.c=.o)))

################################################################################
# Generating dependency file list
# file1.c           --->  $(DEP_DIR)/file1.d
# ../common/file2.c --->  $(DEP_DIR)/file2.d
# ../config/file3.c --->  $(DEP_DIR)/file3.d
dep_files		:=$(addprefix $(DEP_DIR)/,$(notdir $(SRC_FILES:.c=.d) $(CPP_FILES:.C=.d)))

################################################################################
# Deriving component directroy list, include lists, and client lists
ifndef CL_NOLOG    
DEP_COMP_LIST  += log idl name
endif
comp_dirs		= $(addprefix $(COMP_ROOT)/,$(DEP_COMP_LIST))
ifeq ($(ASP_BUILD),1)
comp_include_dirs	= $(addsuffix /include,$(comp_dirs))
else
comp_include_dirs = $(ASP_INCLUDE)
endif

################################################################################
# Automatically deriving vpath from SRC_FILES
vpath_list		= $(dir $(SRC_FILES))

################################################################################
# Building the compiler flags
#
# Top-level include directory is always added:
ifeq ($(ASP_BUILD),1)
CPPFLAGS		+= -I$(COMP_ROOT)/include
CPPFLAGS        += -I$(COMP_ROOT)/include/ipi
# Component level include directory is always added:
CPPFLAGS		+= -I../include
CPPFLAGS		+= -I../common
#CPPFLAGS		+= -I../config
CPPFLAGS		+= -I.

# Now we add the include directories of all components we depend on:
CPPFLAGS		+= $(addprefix -I,$(comp_include_dirs))
else
CPPFLAGS		+= -I$(ASP_INCLUDE)
CPPFLAGS		+= -I$(ASP_INCLUDE)/ipi
CPPFLAGS		+= -I.
CPPFLAGS		+= -L$(ASP_LIBDIR)
endif

CPPFLAGS		+= -I$(CLOVIS_ROOT)/ASP/3rdparty/ezxml/stable
################################################################################
# Rules and targets
#

# But if all is given, we will build the same:
all: libs
.PHONY:
deploy:
#############################################################
# Building libraries
#
# We handle the following two cases separately:
#    - only one library is specified
#    - two or more libraries are specified; in this case for each library
#      the source-files must be specified in a SRC_FILES_libname.a
#      variable.  E.g.
#	LIB_NAMES = libfoo.a libbar.a
#	SRC_FILES_libfoo.a = foo1.c foo2.c foo3.c
#	SRC_FILES_libbar.a = bar1.c bar2.c bar3.c
#

$(OBJ_DIR):
	$(call cmd,mkdir)

.PHONY: clean depend
CLEAN_FILES     = $(OBJ_DIR) $(DEP_DIR) ./*.bb ./*.bbg ./*.da  ./*.gcov ./*.so

$(DEP_DIR):
	$(call cmd,mkdir)

#############################################################
# Dependency management
#
# Don't include or remake dependencies when doing clean or splint
#
ifeq ($(findstring clean,$(MAKECMDGOALS))$(findstring splint,$(MAKECMDGOALS)),)
    -include $(dep_files)    
endif

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk
