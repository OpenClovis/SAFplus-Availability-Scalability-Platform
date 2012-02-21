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
#  When this file is included, the following variables are expected to be
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

ifdef BUILD_CPLUSPLUS
    BUILD_PLUS = $(BUILD_CPLUSPLUS)
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
# To build for Solaris use SOLARIS_BUILD=1 flag on the make command line
ifdef SOLARIS_BUILD
    CFLAGS += -DSOLARIS_BUILD
    ifndef BUILD_WITHOUT_C99
        CFLAGS += -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1
    endif
    CC = gcc
endif

#-------------------------------------------------------------------------------
# To build with debug, use the CL_DEBUG=1 flag on the make command line
ifdef PROF
  ifeq ("$(origin PROF)", "command line")
	CFLAGS += -pg
        LDFLAGS += -pg
  endif
endif
ifdef O
  ifeq ("$(origin O)", "command line")
	CFLAGS += -O$(O) -fno-strict-aliasing -g

  endif
endif
ifndef O
  ifeq ("$(CL_TARGET_PLATFORM)", "ppc")
    CFLAGS += -fno-strict-aliasing -g
  else
    ifeq ($(TARGET_QNX), 1)
      CFLAGS += -fno-strict-aliasing -g
    else
     ifeq ($(TARGET_VXWORKS), 1)
      CFLAGS += -fno-strict-aliasing -g
     else
      CFLAGS += -Os -fno-strict-aliasing -g
     endif
    endif
  endif
endif

ifdef CL_DEBUG
	# -DRECORD_TXN
	CFLAGS += -g -DCL_DEBUG
endif
export CL_DEBUG

#-------------------------------------------------------------------------------
# To build static libraries, use the S=0 flag on the make command line
ifdef S
  ifeq ("$(origin S)", "command line")
    BUILD_SHARED = $(S)
  endif
endif
ifndef BUILD_SHARED
  BUILD_SHARED = 1
endif
export BUILD_SHARED

ifeq ($(BUILD_SHARED),0)
    CFLAGS += -Wl,static
endif

#-------------------------------------------------------------------------------
# To build with more strickt compiler warnings, use the W=1 flag on the make
# command line
ifdef W
  ifeq ("$(origin W)", "command line")
    BUILD_WARNINGS = $(W)
  endif
endif
ifndef BUILD_WARNINGS
  BUILD_WARNINGS = 0
endif

ifdef BUILD_TIPC
  ifeq ($(BUILD_TIPC),1)
	export BUILD_TIPC
  endif
endif

include $(CLOVIS_ROOT)/ASP/mk/make-cross.mk
include $(CLOVIS_ROOT)/ASP/mk/make-path.mk
include $(CLOVIS_ROOT)/ASP/mk/make-distro.mk

################################################################################
# Tool chain definitions go here
#
################################################################################
CP              = cp -f
MV              = mv -f
MKDIR		= mkdir -p
DEPLOY		= $(CLOVIS_ROOT)/build/scripts/deploy.sh

################################################################################
# Flags for various tools
#
################################################################################
ARFLAGS		= -r
TOP_LDFLAGS     = -g -lpthread
ifeq ($(COMPNAME),dbal)
TOP_CFLAGS      = -c -Wall -D_GNU_SOURCE
else
    ifeq ($(WIND_VER),0)
        TOP_CFLAGS  = -c -Wall -D_GNU_SOURCE
    else
        TOP_CFLAGS  = -c -Wall -Werror -Wno-error=unused-but-set-variable -D_GNU_SOURCE
    endif
endif
ifeq ($(BUILD_WARNINGS),1)
TOP_CFLAGS     += -Wcomment -Wnonnull \
                  -Wswitch-default -Wswitch-enum -Wextra \
                  -Wtraditional -Wshadow -Wbad-function-cast \
                  -Wcast-align -Wwrite-strings -Wsign-compare \
                  -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes \
                  -Wmissing-declarations -Wnested-externs -Wunreachable-code
endif
TOP_CFLAGS += -Wno-format-security
ifeq ($(BUILD_SHARED), 1)
SHARED_LDFLAGS  = -shared -fPIC 
endif
ifeq ($(BUILD_PLUS),0)
    ifndef BUILD_WITHOUT_C99 
        ifeq ($(TARGET_QNX), 1)
            TOP_C99FLAGS      := 
        else
         ifeq ($(TARGET_VXWORKS), 1)
            TOP_C99FLAGS :=
         else
            TOP_C99FLAGS      := -std=c99 -pedantic
         endif
 
        endif
    endif
endif
ifeq ($(BUILD_PLUS),1)
TOP_CFLAGS      += -Wno-deprecated
endif
OPT_CFLAGS	= -O2
PURIFY_CFLAGS	= -DWITH_PURIFY
ifeq ($(BUILD_SHARED), 0)
 CFLAGS += -DWITH_STATIC 
 BUILD_STATIC=1
 export BUILD_STATIC
endif
ifeq ($(BUILD_PURIFY),1)
    CFLAGS	+= $(PURIFY_CFLAGS)
    EXTRA_CFLAGS	+= $(PURIFY_CFLAGS)
    CC		:= purify $(CC)
endif

GCOV_CFLAGS	= -fprofile-arcs -ftest-coverage -DGCOV
ifeq ($(BUILD_GCOV),1)
    CFLAGS	+= $(GCOV_CFLAGS)
    EXTRA_CFLAGS	+= $(GCOV_CFLAGS)
    LDFLAGS	+= $(GCOV_CFLAGS)
endif

ifeq ($(BUILD_TIPC_COMPRESSION), 1)
	TOP_CFLAGS += -DCL_TIPC_COMPRESSION
endif

ifeq ($(BUILD_OSAL_DEBUG), 1)
	TOP_CFLAGS += -DCL_OSAL_DEBUG
endif

CFLAGS		+= $(TOP_CFLAGS)
ifndef BUILD_WITHOUT_C99 
    CFLAGS		+= $(TOP_C99FLAGS)
endif
ifeq ($(BUILD_SHARED), 1)
    CFLAGS		+= -fPIC 
endif
CFLAGS		+= $(EXTRA_CFLAGS)
CPPFLAGS	+= $(EXTRA_CPPFLAGS)
ifndef SOLARIS_BUILD
ifeq ($(BUILD_SHARED), 1)
LDFLAGS		+= -Wl,--export-dynamic
endif
endif
LDFLAGS 	+= $(EXTRA_LDFLAGS)
LDLIBS	 	+= $(EXTRA_LDLIBS) 
SHARED_LDFLAGS	+= $(EXTRA_SHARED_LDFLAGS)

SPLINTCMD	= splint
SPLINTFLAGS	+= +posixlib -preproc -badflag -warnsysfiles \
                   -nof -weak -line-len 360 -unrecog

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
quiet_cmd_mkdir  = MKDIR   $(call quiet-strip,$@)
      cmd_mkdir  = $(MKDIR) $@

#-------------------------------------------------------------------------------
# Generate dependency file for a .c file:
define make-depend
    $(CC) -M \
           -MF $2 \
	   -MP \
	   -MT $3 \
	   -MT $2 \
	   $(CPPFLAGS) \
	   $(CFLAGS) \
	   $(TARGET_ARCH) \
	   $1
endef
define quiet-strip
$(shell echo $1 | sed -e 's/.*target\//$$(TARGET)\//g')
endef
quiet_cmd_depend = DEP     $(call quiet-strip,$@)
      cmd_depend = $(call make-depend,$<,$@,$(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(subst .cxx,.o,$(notdir $<)))))

#-------------------------------------------------------------------------------
# Build .o files from .c
quiet_cmd_cc_o_c = CC      $(call quiet-strip,$@)
      cmd_cc_o_c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(OUTPUT_OPTION) $<

#-------------------------------------------------------------------------------
# Link an executable from .o and lib files
# To be called as $(call cmd,link,<object-files>)
# So that $2 represents here all the <object-files>.
quiet_cmd_link = LINK    $(call quiet-strip,$@)
      cmd_link = $(CC) $(LDFLAGS) $(TARGET_ARCH) $2 $(LOADLIBES) $(LDLIBS) -o $@

#-------------------------------------------------------------------------------
# Create an archive (lib) from .o files
quiet_cmd_ar = AR      $(call quiet-strip,$@)
      cmd_ar = $(AR) $(ARFLAGS) $@ $^ 2>/dev/null

#-------------------------------------------------------------------------------
# Create a shared library from .o files
quiet_cmd_link_shared = LINK-SO $(call quiet-strip,$@)
      cmd_link_shared = $(CC) $(LDFLAGS) $(TARGET_ARCH) $(EXTRA_LDLIBS) $^ -o $@ $(SHARED_LDFLAGS) 

#-------------------------------------------------------------------------------
# Run ln -sf 
quiet_cmd_ln = LN      $(call quiet-strip,$@)
      cmd_ln = ln -sf

################################################################################
# Rule definitions
#
################################################################################

#-------------------------------------------------------------------------------
# Generating .o from .c .C .cc or .cpp (compiling):
$(OBJ_DIR)/%.o: %.C
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.cc
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.cpp
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.c
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.cxx
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.CC
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.CPP
	$(call cmd,cc_o_c)

$(OBJ_DIR)/%.o: %.CXX
	$(call cmd,cc_o_c)


#-------------------------------------------------------------------------------
# Generating dependency file from a lot of possible c or c++ file endings
$(DEP_DIR)/%.d: %.c $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.C $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.cc $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.cxx $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.cpp $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.CC $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.CPP $(DEP_DIR)
	$(call cmd,depend)

$(DEP_DIR)/%.d: %.CXX $(DEP_DIR)
	$(call cmd,depend)




################################################################################
# Make targets
#
################################################################################

#-------------------------------------------------------------------------------
# clean: clean up most derived files
.PHONY: clean
CLEAN_FILES += $(EXTRA_CLEAN) $(OBJ_DIR)/asp_build.c
rm_files = $(CLEAN_FILES)
clean:
	$(call cmd,rmfiles)

#-------------------------------------------------------------------------------
# distclean: clean up all derived files
.PHONY: distclean
distclean: rm_files += TAGS tags cscope.* splint.log
distclean: clean
	$(call cmd,rmfiles)

#-------------------------------------------------------------------------------
# cscope, tags, TAGS: generate cscope.out, tags, TAGS, respectively for the
# current directory as root directory.
define all_sources
    ( find -L . -name '*.[ch]' -print )
endef
.PHONY: cscope

quiet_cmd_cscope-files = FILELST cscope.files
      cmd_cscope-files = $(all_sources) > cscope.files

quiet_cmd_cscope = MAKE    cscope.out
      cmd_cscope = cscope -b -q

cscope: FORCE
	$(call cmd,cscope-files)
	$(call cmd,cscope)

quiet_cmd_TAGS = MAKE    $(call quiet-strip,$@)
      cmd_TAGS = $(all_sources) | etags -
quiet_cmd_tags = MAKE    $(call quiet-strip,$@)

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

ifneq ($(findstring cov,$(MAKECMDGOALS)),)
    include $(CLOVIS_ROOT)/ASP/mk/make-gcov.mk
endif

#-------------------------------------------------------------------------------
# Add BUILD_NUMBER signature
$(OBJ_DIR)/asp_build.c: $(SRC_FILES)
	@if [ -f "$$CLOVIS_ROOT/ASP/BUILD" ]; then \
	    . $$CLOVIS_ROOT/ASP/BUILD; \
	else \
	    BUILD_NUMBER="unlabeled"; \
	fi; \
	$(ECHO) "char asp_build[] = \"ASP_BUILD: built on [$$HOSTNAME] at [`date`] by [$$USER] from ASP build [$$BUILD_NUMBER]\\n\";" > $(OBJ_DIR)/asp_build.c
SRC_FILES += $(OBJ_DIR)/asp_build.c
$(OBJ_DIR)/asp_build.o: $(OBJ_DIR)/asp_build.c
	$(call cmd,cc_o_c)

#-------------------------------------------------------------------------------
# splint: run splint on the code base, recursively
.PHONY: splint splint-report

# $(Q)echo "Running splint $(PWD)"
# $(Q)echo "CPP files: $(CPPFLAGS)"
# $(Q)echo "C flags: $(SRC_FILES)"
splint:
	$(Q)echo "Splint command: $(SPLINTCMD) $(SPLINTFLAGS) $(CPPFLAGS) $(SRC_FILES)"
	$(Q)if [ "$(SRC_FILES)" ]; then \
	        $(SPLINTCMD) $(SPLINTFLAGS) $(CPPFLAGS) $(SRC_FILES) 2>&1 | tee splint.log; \
	    fi

splint-summary:
	$(Q)find . -name splint.log -printf '%p: ' -exec tail -n 1 \{\} \; | sort

splint-report-tarball:
	$(Q)(find $(CLOVIS_ROOT)/ASP/components -name splint.log) | xargs tar czf splint-logs.tgz
################################################################################
#package: create the binary package from the source
################################################################################
.PHONY: install
install:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/sdk/scripts/install.sh

.PHONY: uninstall
uninstall:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/common/scripts/uninstall.sh

.PHONY: binary
binary:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/sdk/scripts/binary.sh

################################################################################
# base-images
# create run-time images from binaries
################################################################################
.PHONY: base-images
ifeq ($(STRIP), 1)
base-images:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/base-images/scripts/base-images.sh 1
else
base-images:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/base-images/scripts/base-images.sh 0
endif
################################################################################
# prerequisites
# populate run-time images with 3rd party prerequisites
################################################################################
.PHONY: prerequisites
prerequisites:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/prerequisites/scripts/prerequisites.sh

################################################################################
# instantiate
# Instantiate the generic image created by make images into individual
# distribution images for individual targets
################################################################################
.PHONY: instantiate
instantiate: 
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/instantiate/scripts/instantiate.sh

################################################################################
# post-images
# Run model-specific post-images script if it exists so that customer
# can further integrate the harvested image with their own build infrastructure
################################################################################
.PHONY: post-images
post-images:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/post-images/scripts/post-images.sh

################################################################################
# images
# Create the complete images system conditioned on target.conf settings and
# using the above make targets
################################################################################
.PHONY: images images-stripped
images:
ifeq ($(TARGET_QNX), 1)
	@ make base-images
	$(RUN_SCRIPT) $(MODEL_PATH)/target.conf;
	@ if [ "$$INSTANTIATE_IMAGES" != "NO" ]; then make instantiate; else true; fi
	@ make post-images
else
    
ifdef SOLARIS_BUILD
	@ gmake base-images
	$(RUN_SCRIPT) $(MODEL_PATH)/target.conf;
	mkdir -p ${PROJECT_ROOT}/target/${ASP_MODEL_NAME}/images/sun4u/SunOS.sun4u/share
	@ gmake instantiate
	@ gmake post-images
else
	@ make base-images
	@ $(RUN_SCRIPT) $(MODEL_PATH)/target.conf;
	@ if [ "$$INSTALL_PREREQUISITES" != "NO" ]; then make prerequisites; else true; fi
	@ make instantiate
	@ make post-images
endif

endif

images-stripped:
	@ make images STRIP=1

################################################################################
# asp-libs
# Install libraries to installation dir specified by PREFIX
################################################################################
.PHONY: asp-libs
asp-libs: libs

################################################################################
# asp-install
# Install libraries to installation dir specified by PREFIX
################################################################################
.PHONY: asp-install
asp-install:
	$(RUN_SCRIPT) $(CLOVIS_ROOT)/ASP/build/libs-install/scripts/libs-install.sh


################################################################################
help:
	@echo  'Cleaning targets:'
	@echo  '  clean           - remove most generated files, but keep '
	@echo  '                    tags, TAGS, cscope.out, etc.'
	@echo  '  distclean       - remove all generated files'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  depend          - Forces to geneate all .d dependency files'
	@echo  '  libs            - Build all lib*.a files'
	@echo  '  all             - Build all targets, including libs and executables'
	@echo  '  tags/TAGS       - Generate tags file for editors'
	@echo  '  cscope          - Generate cscope index'
	@echo  '  splint          - Run splint on the codebase'
	@echo  '  splint-summary  - Creates splint summary report from splint.log files'
	@echo  '  splint-report-tarball - Creates tarball from splint.log files'
	@echo  ''
	@echo  'Static analysers:'
	@$(MAKE) --no-print-directory \
		               -f $(CLOVIS_ROOT)/ASP/mk/make-check.mk help
	@echo  ''
	@$(MAKE) --no-print-directory \
		               -f $(CLOVIS_ROOT)/ASP/mk/make-gcov.mk gcovhelp
	@echo  'Cross-build and cross-compilation:'
	@$(MAKE) --no-print-directory \
		               -f $(CLOVIS_ROOT)/ASP/mk/make-cross.mk crosshelp
	@echo  ''
	@echo  'Packaging:'
	@echo  '  images          - Create run-time images for target platform.'
	@echo  '                    You will need to ensure that all necessary'
	@echo  '                    values in the the model-specific target.conf'
	@echo  '                    file are configured to complete this step.'
	@echo  '                    Currently configured location of this file is:'
	@echo  '                    $(MODEL_PATH)/target.conf'
	@echo  '  images-stripped - Create run-time images with stripped binaries and libraries.'
	@echo  ''
	@echo  'Architecture specific targets:'
	@echo  '  [coming soon]'
	@echo  ''
	@echo  'Options:'
	@echo  '  make V=0|1 [targets] 0 => quiet build, 1 => verbose build (default)'
	@echo  '  make P=1   [targets] Build for Purify'
	@echo  '  make G=2   [targets] Build for gcov'
	@echo  '  make PLUS=1 [targets] Build using C++'
	@echo  '  make R=1 [targets] Build using -O1|2|3 optimization flag'
	@echo  '  make S=0 [targets] Build static libraries (defaults to shared)'
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
    ifeq ($(strip $(OBJ_FILES_$(ALL_STATIC_LIB_NAMES))),)
      OBJ_FILES_$(ALL_STATIC_LIB_NAMES) := $(OBJ_FILES)
    endif
  endif
 endif
 ifneq ($(words $(ALL_SHARED_LIB_NAMES)),0)
  ifeq ($(words $(SRC_FILES_$(ALL_SHARED_LIB_NAMES))),0)
    SRC_FILES_$(ALL_SHARED_LIB_NAMES):=$(SRC_FILES)
    ifeq ($(strip $(OBJ_FILES_$(ALL_SHARED_LIB_NAMES))),)
      OBJ_FILES_$(ALL_SHARED_LIB_NAMES):= $(OBJ_FILES)
    endif
  endif
 endif
endif

$(shell mkdir -p $(INC_DIR))
$(shell $(ECHO) -e '\043 Generated make include file\056 do not modify' > $(INC_DIR)/libs.mk)
ifneq ($(strip $(ALL_OBJ_FILES)),) 
 $(foreach lib, $(ALL_STATIC_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).a: $(OBJ_FILES_$(lib))" >> $(INC_DIR)/libs.mk\
		   && $(ECHO) -e  '\t$$(call cmd,ar)' >> $(INC_DIR)/libs.mk))
 $(foreach lib, $(ALL_SHARED_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).so: $(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION)" >> $(INC_DIR)/libs.mk\
    && $(ECHO) -e '\t$$(call cmd,ln) $(SHARED_DIR)/$(lib).so.$(ASP_VERSION) $(LIB_DIR)/$(lib).so\n' >> $(INC_DIR)/libs.mk\
	&& $(ECHO) "$(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION): $(OBJ_FILES_$(lib))" >> $(INC_DIR)/libs.mk\
	&& $(ECHO) -e  '\t$$(call cmd,link_shared)' >> $(INC_DIR)/libs.mk\
  ))
else
$(foreach lib,$(ALL_STATIC_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).a: $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(notdir $(SRC_FILES_$(lib):.c=.o))))" >> $(INC_DIR)/libs.mk\
   && $(ECHO) -e  '\t$$(call cmd,ar)' >> $(INC_DIR)/libs.mk))
$(foreach lib,$(ALL_SHARED_LIB_NAMES), \
   $(shell $(ECHO) "$(LIB_DIR)/$(lib).so: $(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION)" >> $(INC_DIR)/libs.mk\
        && $(ECHO) -e '\t$$(call cmd,ln) $(SHARED_DIR)/$(lib).so.$(ASP_VERSION) $(LIB_DIR)/$(lib).so\n' >> $(INC_DIR)/libs.mk\
        && $(ECHO) "$(LIB_DIR)/$(SHARED_DIR)/$(lib).so.$(ASP_VERSION): $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(notdir $(SRC_FILES_$(lib):.c=.o))))" >> $(INC_DIR)/libs.mk\
        && $(ECHO) -e  '\t$$(call cmd,link_shared)' >> $(INC_DIR)/libs.mk\
        ))
endif
include $(INC_DIR)/libs.mk
$(LIB_DIR)/$(SHARED_DIR):
	$(call cmd,mkdir)

$(OBJ_DIR):
	$(call cmd,mkdir)

libs: $(OBJ_DIR) \
      $(LIB_DIR)/$(SHARED_DIR) \
      $(STATIC_LIB_NAMES_WITH_PATH) \
      $(SHARED_LIB_NAMES_WITH_PATH)

CLEAN_FILES += $(STATIC_LIB_NAMES_WITH_PATH)   $(SHARED_LIB_NAMES_WITH_PATH) $(SHARED_LIB_NAMES_WITH_PATH_VER) $(INC_DIR)/libs.mk

# Add the directories where the source files for different libraries are found
# into the 'vpath'.
vpath_list += $(foreach src_file,$(SRC_FILES_$(ALL_STATIC_LIB_NAMES)),$(dir $(src_file)))
vpath_list += $(foreach src_file,$(SRC_FILES_$(ALL_SHARED_LIB_NAMES)),$(dir $(src_file)))
vpath_list += ../common

vpath %.c $(vpath_list)
vpath %.cxx $(vpath_list)
