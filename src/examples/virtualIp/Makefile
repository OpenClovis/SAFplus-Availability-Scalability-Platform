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
# File        : Makefile.in
################################################################################
# Description :
################################################################################


# This makefile escalates the following make targets to all
# subdirectories of build_dir
targets_to_propagate 	= all libs clean images
build_dir 		= ./build

# Derived variables
subdirs = $(shell find $(build_dir) -mindepth 1 -maxdepth 1 -type d)
subdirs := $(filter-out $(build_dir)/.svn, $(subdirs))
subdirs := $(filter-out $(build_dir)/.cvs, $(subdirs))

$(warning The following diretcories will be traversed: $(subdirs))
$(warning $(MAKECMDGOALS))

.PHONY: $(targets_to_propagate)

$(targets_to_propagate):
	@failed=0; \
	for d in $(subdirs); do \
	    echo "Running 'make $(MAKECMDGOALS)' in $$d"; \
	    if ! (cd $$d; make $(MAKECMDGOALS)); then \
	    	echo "*** ERROR: 'make $(MAKECMDGOALS)' failed in $$d; aborting group make"; \
		failed=1; \
		break; \
	    fi; \
	done; \
	if [ ! "$$failed" -eq "1" ]; then \
	    echo "'make $(MAKECMDGOALS)' succeeded for all targets under $(build_dir)"; \
	else \
	    exit 1; \
	fi
