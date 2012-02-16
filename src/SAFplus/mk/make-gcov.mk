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
# File        : make-gcov.mk
################################################################################
# Description :
#
# Utility make targets to generate coverage report for ASP and ASP based
# components
#
################################################################################

# In order this to work, the built should have been done using make G=1
# option, which generated *.gcno files under the $TARGET/obj dir
# After that a normal ASP run should have deposited additional *.gcda
# files in the same dir.
# In case of cross-running, these gcda files are generated on the target
# (if the same $TARGET/obj path is available on the target!), and need to
# be copied back to the build server.
#
# So, make lcov-info assumes the following:
# 1.  ASP source tree is under $(CLOVIS_ROOT)/ASP/components
# 2.  The model source tree is under $(MODEL_PATH)
# 3.  The model root dir is $(PROJECT_ROOT)/$(ASP_MODEL_NAME)
# 4.  The *.gc?? files are under $(MODEL_TARGET)/obj

# Some interface variables, defined only if they are not provided from the
# outside

LCOV_INFO_DIR	?= $(PROJECT_ROOT)/lcov
LCOV_WORKDIR	?= $(PROJECT_ROOT)/lcov/tmp
LCOV_TESTNAME	?= $(ASP_MODEL_NAME)

lcov-info: FORCE
	# Step 1: Copy out the ASP/components  source tree to LCOV_WORKDIR
	rm -fr $(LCOV_WORKDIR)/ASP && \
		mkdir -p $(LCOV_WORKDIR)/ASP && \
        rsync -av --exclude='*/.svn' \
		          --exclude='*/test' \
			  --include='*/' \
			  --include='*.[ch]' \
			  --exclude='*' \
			  $(CLOVIS_ROOT)/ASP/components/ \
			  $(LCOV_WORKDIR)/ASP/
	# Step 2: For each .gcda file found, locate the corresponding .c or
	# .h file, and copy both the .gcda and .gcno files to the same
	# dir where the *.h or *.c file was found 
	for gcda in `find $(ASP_TARGET)/obj -name '*.gcda'`; do \
		gcno=$$(echo $$gcda | sed -e 's/gcda/gcno/g') ; \
		base=$$(basename $$gcno .gcno) ; \
		echo "Found gcda file:              $$gcda" ; \
		srcname=$$(strings $$gcno | grep -E "(^|/.*)$$base\." | head -n 1) ; \
		echo "  Source file candidate:      $$srcname" ; \
        if echo $$srcname | grep "^/.*" > /dev/null || echo $$gcno | grep -i "3rdParty" ; then \
            echo "Ignoring file $$(basename $$srcname)" ; \
            dirname= ; \
        else \
		    srcname=$$(basename $$srcname) ; \
            dirname=$$(dirname $$gcno) ; \
            tempdir=$(LCOV_WORKDIR) ; \
            lcovdir=$$(echo $$tempdir | sed -e "s/\//\\\\\//g") ; \
            dirname=$$(echo $$dirname | sed -e "s/^\/.*\/components/$$lcovdir\/ASP/g") ; \
        fi ; \
		if [ $$dirname ] ; then \
			echo "  Copying gcno/gcda files to: $$dirname" ; \
			cp $$gcda $$gcno $$dirname ; \
		fi ; \
    done	
	# Step 3: Run lcov on the data files and deposit info file into
	# LCOV_INFO_DIR 
	mkdir -p $(LCOV_INFO_DIR) ; \
    lcov -d $(LCOV_WORKDIR) -c -t $(LCOV_TESTNAME) \
        > $(LCOV_INFO_DIR)/$(LCOV_TESTNAME).info
	@echo "========================="
	@echo "generated lcov info file: $(LCOV_INFO_DIR)/$(LCOV_TESTNAME).info"
	@echo "========================="

lcov-report:
	mkdir -p $(LCOV_INFO_DIR)/html && \
		cd $(LCOV_INFO_DIR)/html && \
		genhtml -s ../*.info

lcov-getobjdir:
	@echo $(MODEL_TARGET)/obj

################################################################################
.PHONY: gcovhelp
gcovhelp:
	@echo 'Test coverage related (use G=1 make flag to collect .gcda files)'
	@echo '  lcov-info           - Generate an lcov .info file from the'
	@echo '                        available .gcda files found under obj dirs'
	@echo '                        label it with testname LCOV_TESTNAME and'
	@echo '                        place it under LCOV_INFO_DIR as LCOV_TESTNAME.info'
	@echo '  lcov-report         - Generate html report based on the info files'
	@echo '                        found so far under LCOV_INFO_DIR. The html'
	@echo '                        page set will be under LCOV_INFO_DIR/html'
	@echo '  lcov-getobjdir	     - Prints the location of the obj directory'
	@echo ''
	
include $(BUILD_ROOT)/mk/make-cross.mk
