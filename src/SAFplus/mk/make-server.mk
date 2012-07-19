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
# ModuleName : mk                                                                
# File        : make-server.mk
################################################################################
# Description :
# 
#  Make include for building executables (servers)
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
#  ASP_LIBS		mand.   List of ASP library files we need to link with
#  THIRDPARTY_LIBS	opt.    List of 3rd party libraries we need to link with
#  SYS_LIBS		opt.	List of system libraries we need to link with
#  EXE_NAME		mand.   Name of the target executable to be built
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

include $(CLOVIS_ROOT)/SAFplus/mk/preface.mk
include $(CLOVIS_ROOT)/SAFplus/mk/make-path.mk

ASP_EXE_PREFIX		:= safplus

ifdef COMPNAME
LIB_NAMES		:= libCl$(COMPNAME)Server
endif

# Remove the requirement for CPP_FILES and SRC_FILES to be separate
SRC_FILES += $(CPP_FILES)

# Convert all possible extensions to .o
oFilesTmp := $(SRC_FILES:.c=.o)
oFilesTmp := $(oFilesTmp:.C=.o)
oFilesTmp := $(oFilesTmp:.cc=.o)
oFilesTmp := $(oFilesTmp:.CC=.o)
oFilesTmp := $(oFilesTmp:.cxx=.o)
oFilesTmp := $(oFilesTmp:.CXX=.o)
oFilesTmp := $(oFilesTmp:.cpp=.o)
oFilesTmp := $(oFilesTmp:.CPP=.o)

# Convert all possible extensions to .d
dFilesTmp := $(SRC_FILES:.c=.d)
dFilesTmp := $(dFilesTmp:.C=.d)
dFilesTmp := $(dFilesTmp:.cc=.d)
dFilesTmp := $(dFilesTmp:.CC=.d)
dFilesTmp := $(dFilesTmp:.cxx=.d)
dFilesTmp := $(dFilesTmp:.CXX=.d)
dFilesTmp := $(dFilesTmp:.cpp=.d)
dFilesTmp := $(dFilesTmp:.CPP=.d)



################################################################################
# Generating object file list from SRC_FILES list:
# file1.c           --->  $(OBJ_DIR)/file1.o
# ../common/file2.c --->  $(OBJ_DIR)/file2.o
# ../config/file3.c --->  $(OBJ_DIR)/file3.o

obj_files		:= $(addprefix $(OBJ_DIR)/,$(notdir $(oFilesTmp)))

################################################################################
# Generating dependency file list
# file1.c           --->  $(DEP_DIR)/file1.d
# ../common/file2.c --->  $(DEP_DIR)/file2.d
# ../config/file3.c --->  $(DEP_DIR)/file3.d
dep_files		:=$(addprefix $(DEP_DIR)/,$(notdir $(dFilesTmp) ))

################################################################################
# Deriving component directroy list, include lists, and client lists
comp_dirs		=
ifdef DEP_COMP_LIST
    ifneq ($(DEP_COMP_LIST), None)
		ifeq ($(ASP_BUILD),1)
        comp_dirs	= $(addprefix $(COMP_ROOT)/,$(DEP_COMP_LIST))
		else
		comp_dirs = $(ASP_LIBDIR)
		endif
    endif
endif

ifeq ($(ASP_BUILD),1)
comp_include_dirs	= $(addsuffix /include,$(comp_dirs))
comp_client_dirs	= $(addsuffix /client,$(comp_dirs))
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
CPPFLAGS		+= -I $(COMP_ROOT)/include
CPPFLAGS		+= -I $(COMP_ROOT)/include/ipi

# Component level include directory is always added:
CPPFLAGS		+= -I../include
CPPFLAGS		+= -I../common
#CPPFLAGS		+= -I../config
CPPFLAGS		+= -I.

# Now we add the include directories of all components we depend on:
CPPFLAGS		+= $(addprefix -I,$(comp_include_dirs))

CPPFLAGS		+= -I$(CLOVIS_ROOT)/SAFplus/include
else
CPPFLAGS		+= $(addprefix -I,$(comp_include_dirs))
CPPFLAGS		+= -I$(ASP_INCLUDE) -I$(ASP_INCLUDE)/ipi -L$(ASP_LIBDIR)
CPPFLAGS		+= -I.
endif
################################################################################
# Building the linker flags
MODEL_CONFIG_LIBRARY := 
#libClConfig.a

ASP_AMF_LIBS := libClAmfClient.a libClAmsMgmt.a libClAmsXdr.a libClGms.a

# Figure out if we are using the libmw (everything) shared library or not
ifneq (,$(findstring libmw.a,$(ASP_LIBS)))
USING_LIBMW := 1
endif

ASP_LIBS		+= $(MODEL_CONFIG_LIBRARY) $(ASP_AMF_LIBS) 

ifeq ($(TARGET_QNX), 1)
SYS_LIBS		+= -lm
PURIFY_LIBS     += -ldb
else
ifeq ($(TARGET_VXWORKS), 1)
SYS_LIBS += -ldb -ldl
PURIFY_LIBS += -ldb
else
ifdef SOLARIS_BUILD
SYS_LIBS		+= -L/usr/local/lib -L/usr/local/BerkeleyDB.4.2/lib -lrt -lm -ldl -lpthread #-lgdbm -lsqlite3 -ldb
PURIFY_LIBS             += #-lgdbm -ldb -lsqlite3
else
SYS_LIBS		+= -lrt -lm -ldl -lpthread  #-lgdbm -lsqlite3 -ldb 
PURIFY_LIBS             += #-lgdbm -ldb -lsqlite3
endif
endif
endif

ifeq ($(BUILD_TIPC_COMPRESSION), 1)
	SYS_LIBS += -lz
endif

ifeq ($(BUILD_OSAL_DEBUG), 1)
	CPPFLAGS += -DCL_OSAL_DEBUG
endif

ifdef P
  ifeq ("$(origin P)", "command line")
    ASP_LIBS := $(patsubst %.so,,$(ASP_LIBS))
    ASP_LIBS += libClSQLiteDB.a 
  endif
endif

ifdef S
  ifeq ("$(origin S)", "command line")
    BUILD_SHARED = $(S)
  endif
endif
ifndef BUILD_SHARED
  BUILD_SHARED = 1
endif
export BUILD_SHARED

libs			:= $(subst lib,-l,$(basename $(ASP_LIBS)))

# The hardcoding for the ground comes because it is a special case. It
# always has to be last one so that it will never shadow a real
# function definition.

ifeq ($(ASP_BUILD),1)
  ifdef SOLARIS_BUILD
    LDLIBS  +=-L$(ASP_LIB) -L$(MODEL_LIB) \
               $(libs) -L$(COMP_ROOT)/ground/client 
  else
    LDLIBS  +=-L$(ASP_LIB) -L$(MODEL_LIB) \
         -Wl,--start-group$(SPECIAL_LDFLAGS) $(libs) -Wl,--end-group\
                  -L$(COMP_ROOT)/ground/client 
  endif
else
  ifdef SOLARIS_BUILD
    LDLIBS  +=-L$(ASP_LIB) -L$(MODEL_LIB) \
              $(libs) -L$(ASP_LIBDIR)
  else
    LDLIBS  +=-L$(ASP_LIB) -L$(MODEL_LIB) \
         -Wl,--start-group$(SPECIAL_LDFLAGS) $(libs) -Wl,--end-group\
                  -L$(ASP_LIBDIR)
  endif
endif

#Executables during static build should be linked to the
#libClGroundClient because it has actual ground definitions 
#libClGround is a shared object created out of libClGroundClient and 
#it is needed for the shared build using single shared SO libmw.so

ifndef USING_LIBMW  # Andrew Stone -- libGround has been added to libmw.so, so don't put it again.
ifeq ($(BUILD_SHARED),1)
LDLIBS  += -lClGroundClient -lClGround
else
LDLIBS  += -lClGroundClient
endif
endif

ifeq ($(CM_BUILD),1)
LDLIBS += $(CL_CM)
endif

# Add SYS_LIBS if defined and not None
ifdef SYS_LIBS
    ifneq ($(SYS_LIBS), None)
        LDLIBS		+= $(SYS_LIBS)
    endif
endif

# Add THIRDPARTY_LIBS if defined and not None
ifdef THIRDPARTY_LIBS
    ifneq ($(THIRDPARTY_LIBS), None)
	LDLIBS		+= $(THIRDPARTY_LIBS)
    endif
endif
LDLIBS += -lezxml

#This is needed for the components using single shared SO libmw.so 
#for all ASP libs or ground SO of ASP
ifndef SOLARIS_BUILD     # :-(
ifneq ($(TARGET_QNX), 1) # :-((
ifneq ($(TARGET_VXWORKS), 1)
ifeq ($(BUILD_SHARED), 1)
  EXTRA_LDFLAGS += -Xlinker -rpath-link -Xlinker $(ASP_LIB)
else
  EXTRA_LDFLAGS += -static
endif
endif
endif
endif

################################################################################
# Rules and targets
#
.PHONY: exe
all: libs $(SUBDIRS) depend exe

exe: $(OBJ_DIR) $(BIN_DIR)/$(EXE_NAME) $(LIB_DIR)/$(PLUGIN_NAME)

deploy:exe



all_lib_paths   := $(subst -L,,$(subst -L ,-L,$(filter-out -Wl%,$(filter-out -l%,$(LDLIBS)))))
all_libs        := $(filter -l%,$(LDLIBS))

vpath %.a $(all_lib_paths)
vpath %.so $(all_lib_paths)
ifdef EXE_NAME
$(BIN_DIR)/$(EXE_NAME): $(obj_files) $(ASP_LIBS) $(MODEL_LIB)/$(MODEL_CONFIG_LIBRARY) $(filter %.a,$(EXTRA_LDLIBS)) $(filter %.so,$(EXTRA_LDLIBS)) $(BUILD_ROOT)/clasp.env
	$(call cmd,link,$(obj_files))
	$(shell mkdir -p $(BIN_DIR))
endif

ifdef PLUGIN_NAME
$(LIB_DIR)/$(PLUGIN_NAME): $(obj_files) $(ASP_LIBS) $(MODEL_LIB)/$(MODEL_CONFIG_LIBRARY) $(filter %.a,$(EXTRA_LDLIBS)) $(filter %.so,$(EXTRA_LDLIBS))
	$(shell mkdir -p $(LIB_DIR))
	$(call cmd,link_shared,$(obj_files))

endif


.PHONY: clean depend libs
CLEAN_FILES     = $(OBJ_DIR) $(DEP_DIR) ./*.bb ./*.bbg ./*.da ./*.gcov ./*.so

# FIXME: for a temporary time, we need to clean ./*.d and ./*.o files too
# because of the recent change to ./obj and ./dep directories
CLEAN_FILES	+= ./*.o ./*.d ./*.so

#workaround: make-server.mk is included in two directories that do not build an
#executable - app and snmp.  hence, this check has been added.
ifdef EXE_NAME
CLEAN_FILES += $(BIN_DIR)/$(EXE_NAME)
endif


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
