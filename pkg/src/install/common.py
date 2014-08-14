import os, sys
from subprocess import call


# ------------------------------------------------------------------------------
# Localizable strings
# ------------------------------------------------------------------------------

# error strings
#PATCH_ERROR = 'Error: Patch failed\n'
#CONFIGURE_ERROR = 'Error: Configure failed\n'
#MAKE_ERROR = 'Error: Make failed\n'
#MAKE_INSTALL_ERROR = 'Error: Make install failed\n'
#EXTRACT_ERROR = 'Error: Could not extract\n'

ERROR_MKDIR_FAIL = 'Error: Unable to create directory \'%s\'\n'
ERROR_RMDIR_FAIL = 'Error: Unable to nuke directory \'%s\'\n'
ERROR_CANT_CONTINUE = 'Error: Script could not continue\n'
ERROR_WPERMISSIONS = 'Error: No write permission to \'%s\'\n'
WARNING_INCOMPLETE = '\n[WARNING] The installation is incomplete, rerun this script to restart and complete installation'



# points to root directory where install.py

def syscall(cmd):
    """ 
        Preform a given unix commandline call
        and return the result in string or list
        """
    results = os.popen(cmd).readlines()
    if len(results) == 1:
        return results[0].rstrip()
    return results

    
def determine_bit():
    """ determines 32 or 64 bit os """
    
    uname = syscall('uname -m')
    
    if uname == 'unknown':
        # just in case
        uname = syscall('uname -a')
    
    # defaults to 32 bit
    bit = 32
    
    if '64' in uname:
        bit = 64
    
    return bit
    
    
def cli_cmd(cli):
    """ 
        Preform a given unix commandline call
        and return the exit code
    """
    try:
        ecode = call(cli, shell=True)
        if ecode < 0:
            print >> sys.stderr, "[ERROR] Child was terminated by signal %d" % (-ecode)
            sys.exit(1)
    except KeyboardInterrupt:
        print WARNING_INCOMPLETE
        print '[WARNING] Script killed in an unrecoverable state'
        sys.exit(1)
    except OSError, e:
        print >> sys.stderr, "Child Execution failed:", e
        sys.exit(1)

    return ecode


    
# sometimes we need source code to be represented as a pythin multiline string
# that code is below here

CL_LOG_VIEWER = """#!/bin/sh
cd $PACKAGE_ROOT/bin
./cl-log-viewer
""".strip()


CLUTILS_RC = """
###########################################################################
#  Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
#
#  The source code for this program is not published or otherwise divested
#  of its trade secrets, irrespective of what has been deposited with  the
#  U.S. Copyright office.
#
#  No part of the source code  for this  program may  be use,  reproduced,
#  modified, transmitted, transcribed, stored  in a retrieval  system,  or
#  translated, in any form or by  any  means,  without  the prior  written
#  permission of OpenClovis Inc
###########################################################################

myexit_notput () {

	printf "\\b%-17s" 'Failed'
	tput el
	ROWS=\`tput lines\`
	ROWS=\`expr \$ROWS - 3\`
	tar zcf \$WORKING_DIR/log.tar.gz \$WORKING_DIR/log >/dev/null 2>&1
	#tput cup \$ROWS 0
	echo -e \$red
	echo "\$*, send log.tar.gz to support@openclovis.com for help" echo -e \$black
	tput cnorm
	kill \`cat \$INSTALLER \`
	exit
}

# roll cursor to indicate busy
roll () {
	p1=\$1
	echo -n " "
	while [ true ];
	do
        kill -0 \$p1 > /dev/null 2>&1
		if [ \$? -ne 0 ]; then
			return
		fi
		printf "\\b/"
		sleep 0.1
		printf "\\b-"
		sleep 0.1
		printf "\\b\\\\"
		sleep 0.1
		printf "\\b|"
		sleep 0.1
	done
}

cleanup () {
	echo
	echo -n "Cleaning up ... "
	cd \$WORKING_DIR
	if [ -d log ]; then
		mv log log.\`date "+%F-%H-%M"\`
	fi
	echo "done"
}

CONFIGURE_ERROR="[ERROR] Configure failed"
MAKE_ERROR="[ERROR] Make failed"
MAKE_INSTALL_ERROR="[ERROR] Make install failed"
EXTRACT_ERROR="[ERROR] Could not extract tarball"

black='\E[30;49m'
red='\E[31;49m'
green='\E[32;49m'
yellow='\E[33;49m'
blue='\E[34;49m'
magenta='\E[35;49m'
cyan='\E[36;49m'
white='\E[37;49m'

trap 'rm -f \$INSTALLER; tput cup \`tput lines\` 0; tput cnorm; tput sgr0 ' 0
trap "exit 1; tput sgr0" 1 2 3 15 20

""".strip()
