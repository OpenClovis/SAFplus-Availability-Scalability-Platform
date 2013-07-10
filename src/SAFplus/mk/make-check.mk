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
# File        : make-check.mk
################################################################################
# Description :
#
# Perform various checks on the source tree and on various derived binaries
#
################################################################################

#
# Allowed component names:
#
COMP_NAMES	= Alarm Ams \
		  Bit Buffer \
		  Cap Ckpt Cksm Clist Cm Cnt Common Cor Cpm \
		  Dbal Debug Diag \
		  Event Eo \
		  Fault \
		  Gms \
		  Hal Hpi \
		  Ioc \
		  Log \
		  Med \
		  Name \
		  Om Osal \
		  Queue \
		  Parser Policy Pool Prov \
		  Rmd Rule \
		  Snmp Sm \
		  Timer Trace Txn Tmpcomp1 Tmpcomp2 \
                  Um \
                  Xdr

ASP_ROOT	= $(CLOVIS_ROOT)/SAFplus
SCRIPT_DIR	= $(ASP_ROOT)/scripts

#
# Always run this in silent mode
#
Q = @

# All API directories, relative from ASP_ROOT:
api_dirs = $(shell cd $(ASP_ROOT); find components -maxdepth 2 -name include)

# All API filenames, relative from ASP ROOT:
api_files = $(shell cd $(ASP_ROOT); \
	for d in $(api_dirs); do \
	    find $$d -name "*.h" -print ; \
	done)

check: checkapi checklib FORCE

checkapi: checkapi_fnames checkapi_cond checkapi_syms FORCE

filename-only		= sed 's/.*\///g'
comp_name_pattern	= $(shell echo $(COMP_NAMES) | sed -e 's/ /|/g')
cl_filename_pattern	= "cl($(comp_name_pattern))([A-Z][^\.]*)?\.h"
saf_filename_pattern	= "sa(Ckpt|Ais|Amf|Evt)\.h"

checkapi_fnames: FORCE
	@echo '###############################################################'
	@echo '### Verifying API header file names                         ###'
	@echo '###############################################################'
	@echo 'Non-conforming header files:'
	$(Q)cd $(ASP_ROOT); \
	    for f in $(api_files) ; do \
	        echo $$f | $(filename-only) | \
		    egrep -v -e $(cl_filename_pattern) | \
                    egrep -v -e $(saf_filename_pattern) | \
                        awk '{if (NF>0)print}' ; \
	    done ;

checkapi_cond: FORCE
	@echo '###############################################################'
	@echo '### Verifying conditional guard in API header files         ###'
	@echo '###############################################################'
	@echo 'Problems:'
	$(Q)cd $(ASP_ROOT); \
	    for f in $(api_files) ; do $(SCRIPT_DIR)/check_h_file $$f ; done

checkapi_syms: FORCE
	@echo '###############################################################'
	@echo '### Verifying proper symbol names in header files           ###'
	@echo '###############################################################'
	@echo 'The following symbols are not in Clovis name space:'
	$(Q)cd $(ASP_ROOT); \
	    for f in $(api_files) ; do \
	        ctags -x $$f | awk '$$2=="member"{next}{print $$4,$$1,$$2}' | \
		awk '{if (match($$2, "^_?CL_")!=0){next} \
		      if (match($$2, "^Cl")!=0) {next} \
		      if (match($$2, "^cl")!=0){next} \
                      if (match($$2, "^Sa")!=0){next} \
                      if (match($$2, "^_?SA_?")!=0){next} \
                      if (match($$2, "^sa")!=0){next} \
		      print}' | \
		awk '{printf("%-39s %-25s %s\n", $$1, $$2, $$3)}'; \
	    done

################################################################################
checklib: checklib_syms FORCE

checklib_syms: FORCE
	@echo '###############################################################'
	@echo '### Verifying global Symbols in all libraries               ###'
	@echo '###############################################################'
	@echo 'Symbols not properly name-spaced:'
	$(Q)cd $(ASP_LIB); \
	    for f in `ls *.a | grep -v Server`; do \
	    	nm --defined-only -g $$f | \
		awk '/^[a-zA-Z0-9]+\.o/{fname=$$1}NF==3{print fname,$$0}' | \
		awk '{if (match($$4, "^_?CL_")!=0){next} \
		      if (match($$4, "^_?Cl")!=0) {next} \
		      if (match($$4, "^_?cl")!=0){next} \
                      if (match($$4, "^sa")!=0){next} \
		      print}' | \
		awk '{printf("%-30s %s (%s)\n", $$1,$$4,$$3)}' ; \
	    done

################################################################################
help:
	@echo '  checkapi_fnames (1) - Check all header file names in inlcude/'
	@echo '  checkapi_cond (1)   - Checks if #ifndef conditions are used in'
	@echo '                        exported header files'
	@echo '  checkapi_syms (1)   - Verifies that all symbols in exported'
	@echo '                        headers are properly name-spaced'
	@echo '  checkapi            - Runs all tests marked by (1)'
	@echo '  checklib_syms (2)   - Name space analysis on all libraries'
	@echo '  checklib            - Runs all tests marked by (2)'
	@echo '  check               - Run all above checks'
	
