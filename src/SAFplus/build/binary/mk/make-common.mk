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
# ModuleName  : binary
# File        : make-common.mk
################################################################################
# Description :
#  Common make include for all ASP directories
#
#  Note that ASP Makefiles typically don't directly include this file,
#  but include it via make-subdir.mk, make-client.mk, or make-server.mk
#
#  Interface:
#
#  When this file is included, the following variables are expeced to be
#  defined:
#  Variable           Optional  Values          Explanation
#  ----------------------------------------------------------------------
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
# For help type 'make help'
#
################################################################################

################################################################################
#
# Command line option handling
#
################################################################################

include $(CLOVIS_ROOT)/ASP/mk/make-cross.mk
include $(CLOVIS_ROOT)/ASP/mk/make-distro.mk

# For now, set the default verbosity mode to 1:
BUILD_VERBOSE	= 1

#-------------------------------------------------------------------------------
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands
ifdef V
  ifeq ("$(origin V)", "command line")
    BUILD_VERBOSE = $(V)
  endif
endif
ifndef BUILD_VERBOSE
  BUILD_VERBOSE = 0
endif
export BUILD_VERBOSE

#-------------------------------------------------------------------------------
# To build for purify, use the P=1 flag on the make command line
ifdef P
  ifeq ("$(origin P)", "command line")
    BUILD_PURIFY = $(P)
  endif
endif
ifndef BUILD_PURIFY
  BUILD_PURIFY = 0
endif
export BUILD_PURIFY

ifdef PLUS
  ifeq ("$(origin PLUS)", "command line")
    BUILD_PLUS = $(PLUS)
  endif
endif
ifndef BUILD_PLUS
  BUILD_PLUS = 0
endif
export BUILD_PLUS

#-------------------------------------------------------------------------------
# To build for gcov, use the G=1 flag on the make command line
ifdef G
  ifeq ("$(origin G)", "command line")
    BUILD_GCOV = $(G)
  endif
endif
ifndef BUILD_GCOV
  BUILD_GCOV = 0
endif
export BUILD_GCOV

#-------------------------------------------------------------------------------
# To build with debug, use the CL_DEBUG=1 flag on the make command line
ifdef R
  ifeq ("$(origin R)", "command line")
	CFLAGS += -O2

  endif
endif
ifndef R
  CFLAGS += -g -DCL_DEBUG

endif
export CL_DEBUG

#-------------------------------------------------------------------------------
# To build shared libraries, use the S=1 flag on the make command line
ifdef S
  ifeq ("$(origin S)", "command line")
    BUILD_SHARED = $(S)
  endif
endif
ifndef BUILD_SHARED
  BUILD_SHARED = 0
endif
export BUILD_SHARED


################################################################################
# Tool chain definitions go here
#
# FIXME: for now we rely on the local environment of the development host.
################################################################################
CP              = cp -f
#CC              = gcc
MV              = mv -f
#GCC             = gcc
#AR              = ar
MKDIR		= mkdir -p
DEPLOY		= $(CLOVIS_ROOT)/build/scripts/deploy.sh

ifeq ($(BUILD_PLUS),1)
CC              = g++
GCC             = g++
endif
################################################################################
# Flags for various tools
#
################################################################################
ARFLAGS		= -r
TOP_LDFLAGS     = -g -lpthread
TOP_CFLAGS      = -c -Wall -Werror -D_GNU_SOURCE
SHARED_LDFLAGS  = -shared -fPIC
ifeq ($(BUILD_PLUS),0)
TOP_CFLAGS      += -std=c99 -pedantic
endif
ifeq ($(BUILD_PLUS),1)
TOP_CFLAGS      += -Wno-deprecated
endif
OPT_CFLAGS	= -O2
PURIFY_CFLAGS	= -DWITH_PURIFY
ifeq ($(BUILD_PURIFY),1)
    CFLAGS	+= $(PURIFY_CFLAGS)
    CC		:= purify $(CC)
endif

GCOV_CFLAGS	= -fprofile-arcs -ftest-coverage -DGCOV
ifeq ($(BUILD_GCOV),1)
    CFLAGS	+= $(GCOV_CFLAGS)
    LDFLAGS	+= $(GCOV_CFLAGS)
endif

CFLAGS		+= $(TOP_CFLAGS)
CFLAGS		+= $(EXTRA_CFLAGS)
CPPFLAGS	+= $(EXTRA_CPPFLAGS)
LDFLAGS 	+= $(EXTRA_LDFLAGS)
LDLIBS	 	+= $(EXTRA_LDLIBS)
SHARED_LDFLAGS += $(EXTRA_SHARED_LDFLAGS)

# The compilation/link flags are passed to lower directories as well
export EXTRA_CFLAGS EXTRA_CPPFLAGS EXTRA_LDFLAGS EXTRA_LDLIBS

#-------------------------------------------------------------------------------
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed.
# If it is set to "silent_", nothing wil be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's usefull
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If BUILD_VERBOSE equals 0 then the above command will be hidden.
# If BUILD_VERBOSE equals 1 then the above command is displayed.

ifeq ($(BUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
  MAKEFLAGS += --no-print-directory
endif
ifeq ($(BUILD_PURIFY),1)
   libs			+= $(subst lib,-l,$(basename $(PURIFY_LIBS)))
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q BUILD_VERBOSE

# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

################################################################################
# Command definitions
#
################################################################################

#-------------------------------------------------------------------------------
# Remove files (assumes the rm_files variable is set):
quiet_cmd_rmfiles = $(if $(wildcard $(rm_files)),CLEAN   $(wildcard $(rm_files)))
      cmd_rmfiles = $(if $(rm_files), rm -rf $(rm_files))

#-------------------------------------------------------------------------------
# Create a subdirectory (recursively):
quiet_cmd_mkdir  = MKDIR   $@
      cmd_mkdir  = $(MKDIR) $@

#-------------------------------------------------------------------------------
# Generate dependency file for a .c file:
define make-depend
    $(GCC) -M \
           -MF $2 \
	   -MP \
	   -MT $3 \
	   -MT $2 \
	   $(CPPFLAGS) \
	   $(CFLAGS) \
	   $(TARGET_ARCH) \
	   $1
endef
quiet_cmd_depend = DEP     $@
      cmd_depend = $(call make-depend,$<,$@,\
		     $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(notdir $<))))

#-------------------------------------------------------------------------------
# Build .o files from .c
quiet_cmd_cc_o_c = CC      $@
      cmd_cc_o_c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(OUTPUT_OPTION) $<

#-------------------------------------------------------------------------------
# Link an executable from .o and lib files
# To be called as $(call cmd,link,<object-files>)
# So that $2 represents here all the <object-files>.
quiet_cmd_link = LINK    $@
      cmd_link = $(CC) $(LDFLAGS) $(TARGET_ARCH) $2 $(LOADLIBES) $(LDLIBS) -o $@

#-------------------------------------------------------------------------------
# Create an archive (lib) from .o files
quiet_cmd_ar = AR      $@: $^
      cmd_ar = $(AR) $(ARFLAGS) $@ $^

#-------------------------------------------------------------------------------
# Create a shared library from .o files
quiet_cmd_link_shared = CC -shared $@
      cmd_link_shared =  $(CC) $(LDFLAGS)  $(TARGET_ARCH) $^ -o $@ $(SHARED_LDFLAGS) 


################################################################################
# Rule definitions
#
################################################################################

#-------------------------------------------------------------------------------
# Generating .o from .c (compiling):
$(OBJ_DIR)/%.o: %.c
	$(call cmd,cc_o_c)

#-------------------------------------------------------------------------------
# Generating dependency file from .c
$(DEP_DIR)/%.d: %.c $(DEP_DIR)
	$(call cmd,depend)


################################################################################
# Make targets
#
################################################################################

#-------------------------------------------------------------------------------
# clean: clean up most derived files
.PHONY: clean
CLEAN_FILES += $(EXTRA_CLEAN)
rm_files = $(CLEAN_FILES)
clean:
	$(call cmd,rmfiles)

#-------------------------------------------------------------------------------
# distclean: clean up all derived files
.PHONY: distclean
distclean: rm_files += TAGS tags cscope.*
distclean: clean
	$(call cmd,rmfiles)

#-------------------------------------------------------------------------------
# cscope, tags, TAGS: generate cscope.out, tags, TAGS, respectively for the
# current directory as root directory.
define all_sources
    ( find . -name '*.[ch]' -print )
endef
.PHONY: cscope

quiet_cmd_cscope-files = FILELST cscope.files
      cmd_cscope-files = $(all_sources) > cscope.files

quiet_cmd_cscope = MAKE    cscope.out
      cmd_cscope = cscope -b -q

cscope: FORCE
	$(call cmd,cscope-files)
	$(call cmd,cscope)

quiet_cmd_TAGS = MAKE    $@
      cmd_TAGS = $(all_sources) | etags -
quiet_cmd_tags = MAKE    $@

define cmd_tags
	rm -f $@; \
	CTAGSF=`ctags --version | grep -i exuberant >/dev/null && echo "-I __initdata,__exitdata,EXPORT_SYMBOL,EXPORT_SYMBOL_NOVERS"`; \
	$(all_sources) | xargs ctags $$CTAGSF -a
endef

TAGS: FORCE
	$(call cmd,TAGS)

tags: FORCE
	$(call cmd,tags)

ifneq ($(findstring check,$(MAKECMDGOALS)),)
    include $(CLOVIS_ROOT)/ASP/mk/make-check.mk
endif

################################################################################
#package: create the binary package from the source
################################################################################
.PHONY: install
install:
	. $(CLOVIS_ROOT)/ASP/build/binary/scripts/install.sh

.PHONY: uninstall
uninstall:
	. $(CLOVIS_ROOT)/ASP/build/common/scripts/uninstall.sh

.PHONY: rt-package
rt-package:
	. $(CLOVIS_ROOT)/ASP/build/rt/scripts/package.sh

################################################################################
help:
	@echo  'Cleaning targets:'
	@echo  '  clean		      - remove most generated files, but keep '
	@echo  '                        tags, TAGS, cscope.out, etc.'
	@echo  '  distclean	      - remove all generated files'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  depend              - Forces to geneate all .d dependency files'
	@echo  '  libs                - Build all lib*.a files'
	@echo  '  all		      - Build all targets, including libs and executables'
	@echo  '  tags/TAGS	      - Generate tags file for editors'
	@echo  '  cscope	      - Generate cscope index'
	@echo  ''
	@echo  'Static analysers:'
	@$(MAKE) --no-print-directory \
		               -f $(CLOVIS_ROOT)/ASP/mk/make-check.mk help
	@echo  ''
	@echo  'Cross-build and cross-compilation:'
	@$(MAKE) --no-print-directory \
		               -f $(CLOVIS_ROOT)/ASP/mk/make-cross.mk crosshelp
	@echo  ''
	@echo  'Packaging:'
	@echo  '  package			  - create binary package from source'
	@echo  ''
	@echo  'Architecture specific targets:'
	@echo  '  [coming soon]'
	@echo  ''
	@echo  'Options:'
	@echo  '  make SNMP_BUILD=1 => Build SNMP Server, Agent'
	@echo  '  make V=0|1 [targets] 0 => quiet build, 1 => verbose build (default)'
	@echo  '  make P=1   [targets] Build for Purify'
	@echo  '  make G=2   [targets] Build for gcov'
	@echo  '  make PLUS=1 [targets] Build using C++'
	@echo  '  make R=1 [targets] Build using o2 flag for release mode'
	@echo  '  make S=1 [targets] Build shared libraries also'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets'
	@echo  'Note: make will by default build in debug mode'

################################################################################
# FORCE target to allow forcing any target to be re-made
#

FORCE:


################################################################################
# Building static and shared libraries 
#
################################################################################

# The following variables define what libraries are built:
#   a. LIB_NAMES: list of libraries for which both static and shared versions
#   should be built.
#   b. STATIC_LIB_NAMES: list of libraries for which only static version should
#   be built.
#   c. SHARED_LIB_NAMES: list of libraries for whic only shared version should
#   be built.
# 
# They are of the form libCl<name> without the extension. The extensions ".a"
# and ".so" are added in the makefile.
#


# By default, libraries are created in $(ASP_LIB). But Makefiles that include
# make-common.mk can override the default by defining LIB_DIR. make-config.mk
# makes use of this option.
ifndef LIB_DIR
LIB_DIR			= $(ASP_LIB)
endif

SHARED_DIR      := shared-$(ASP_VERSION)

ifdef STATIC_LIB_NAMES
ALL_STATIC_LIB_NAMES_1 := $(STATIC_LIB_NAMES)
ALL_STATIC_LIB_NAMES_1 += $(LIB_NAMES)
else
ALL_STATIC_LIB_NAMES_1 := $(LIB_NAMES)
endif


ifdef SHARED_LIB_NAMES
ALL_SHARED_LIB_NAMES_1 := $(SHARED_LIB_NAMES)
ifeq ($(BUILD_SHARED),1)
ALL_SHARED_LIB_NAMES_1 += $(LIB_NAMES)
endif    
else
ifeq ($(BUILD_SHARED),1)
ALL_SHARED_LIB_NAMES_1 := $(LIB_NAMES)
endif    
endif

ifeq ($(BUILD_PURIFY),1)
ALL_STATIC_LIB_NAMES_1 += $(PURIFY_SHARED_LIB_NAMES)
else
ALL_SHARED_LIB_NAMES_1 += $(PURIFY_SHARED_LIB_NAMES)
endif

ALL_STATIC_LIB_NAMES = $(strip $(ALL_STATIC_LIB_NAMES_1))
ALL_SHARED_LIB_NAMES = $(strip $(ALL_SHARED_LIB_NAMES_1))


STATIC_LIB_NAMES_WITH_PATH := $(addsuffix .a,$(addprefix $(LIB_DIR)/,$(ALL_STATIC_LIB_NAMES)))
SHARED_LIB_NAMES_WITH_PATH := $(addsuffix .so,$(addprefix $(LIB_DIR)/,$(ALL_SHARED_LIB_NAMES)))
SHARED_LIB_NAMES_WITH_PATH_VER := $(addsuffix .$(ASP_VERSION), $(addsuffix .so,$(addprefix $(LIB_DIR)/$(SHARED_DIR)/,$(ALL_SHARED_LIB_NAMES))))

# If only one library is being built and 'SRC_FILES' is defined, take that as
# the list of source files for that library.
ALL_LIB_NAMES:=$(LIB_NAMES)
ALL_LIB_NAMES+=$(STATIC_LIB_NAMES)
ALL_LIB_NAMES+=$(SHARED_LIB_NAMES)
ifeq ($(words $(ALL_LIB_NAMES)),1)
 ifneq ($(words $(ALL_STATIC_LIB_NAMES)),0)
  ifeq ($(words $(SRC_FILES_$(ALL_STATIC_LIB_NAMES))),0)
   SRC_FILES_$(ALL_STATIC_LIB_NAMES):=$(SRC_FILES)
  endif
 endif
 ifneq ($(words $(ALL_SHARED_LIB_NAMES)),0)
  ifeq ($(words $(SRC_FILES_$(ALL_SHARED_LIB_NAMES))),0)
   SRC_FILES_$(ALL_SHARED_LIB_NAMES):=$(SRC_FILES)
  endif
 endif
endif

$(shell $(ECHO) -e '\043 Generated make include file. do not modify' > libs.mk)
$(foreach lib,$(ALL_STATIC_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).a: $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_FILES_$(lib):.c=.o)))" >> libs.mk\
   && $(ECHO) -e  '\t$$(call cmd,ar)' >> libs.mk))
$(foreach lib,$(ALL_SHARED_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).so: $(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION)" >> libs.mk\
        && $(ECHO) -e '\tln -sf $(SHARED_DIR)/$(lib).so.$(ASP_VERSION) $(LIB_DIR)/$(lib).so\n' >> libs.mk\
        && $(ECHO) "$(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION): $(addprefix $(OBJ_DIR)/,$(notdir $(SRC_FILES_$(lib):.c=.o)))" >> libs.mk\
        && $(ECHO) -e  '\t$$(call cmd,link_shared)' >> libs.mk\
        ))
include libs.mk
$(LIB_DIR)/$(SHARED_DIR):
	$(call cmd,mkdir)

libs: $(OBJ_DIR) $(LIB_DIR)/$(SHARED_DIR) $(STATIC_LIB_NAMES_WITH_PATH)  $(SHARED_LIB_NAMES_WITH_PATH)


CLEAN_FILES += $(STATIC_LIB_NAMES_WITH_PATH)   $(SHARED_LIB_NAMES_WITH_PATH) $(SHARED_LIB_NAMES_WITH_PATH_VER) libs.mk

# Add the directories where the source files for different libraries are found
# into the 'vpath'.        
vpath_list += $(foreach src_file,$(SRC_FILES_$(ALL_STATIC_LIB_NAMES)),$(dir $(src_file)))
vpath_list += $(foreach src_file,$(SRC_FILES_$(ALL_SHARED_LIB_NAMES)),$(dir $(src_file)))
vpath_list += ../common

vpath %.c $(vpath_list)
