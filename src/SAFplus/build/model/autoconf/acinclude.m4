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
# ModuleName  : model
# File        : acinclude.m4
################################################################################
# Description :
## aclocal macro for ASP src SDK					 
################################################################################


AC_DEFUN([OH_CHECK_FAIL],
    [
    OH_MSG=`echo -e "- $1 not found!\n"`
    if test "x" != "x$4"; then
        OH_MSG=`echo -e "$OH_MSG\n- $4"`
    fi
    if test "x$2" != "x"; then
        OH_MSG=`echo -e "$OH_MSG\n- Try installing the $2 package\n"`
    fi
    if test "x$3" != "x"; then
        OH_MSG=`echo -e "$OH_MSG\n- or get the latest software from $3\n"`
    fi
    
    AC_MSG_ERROR(
!
************************************************************
$OH_MSG
************************************************************
)
    ]
)

# Macro to check for GCC version
AC_DEFUN([OH_CHECK_GCC],
    [
    GCCVERSIONOK=`$CC --version | grep "(GCC)" | \
    sed 's/.*GCC.//' | sed 's/\./ /g' | \
    awk '{ \
        if ( $[1] > $1) { \
            print "OK"; \
        } \
        if ( $[1] == $1 ) { \
            if( $[2] > $2 ) { \
                print "OK"; \
            } \
            if( $[2] == $2 ) { \
                if( $[3] >= $3 ) { \
                    print "OK"; \
                } \
            } \
        } \
    }'` \
    
    if test "$GCCVERSIONOK" == "OK"; then
        AC_MSG_RESULT(yes)
    else
        OH_CHECK_FAIL(gcc >= $1.$2.$3 is required to build ASP)
    fi
])

# Macro to check for netsnmp version
AC_DEFUN([OH_CHECK_NETSNMP],
    [
    AC_MSG_CHECKING(for net-snmp 5.3.0)
        have_netsnmp=yes
        SNMPFLAGS=`$NET_SNMP_CONFIG --cflags`
        SNMPALIBS=`$NET_SNMP_CONFIG --agent-libs`
	SNMPVERSIONOK=`$NET_SNMP_CONFIG --version | awk -F\. '{ \
			if ( $[1] >= 5 ) { \
  				if ( $[2] > 3 ) print "OK"; \
 			if ( $[2] == 3) { \
   				if ( $[3] >= 0) print "OK"; \
 			} \
		} \
	}'`
        # the following seems to work... thankfully.
        SNMPCONFDIR=`$NET_SNMP_CONFIG --configure-options | perl -p -e 's/.*sysconfdir=([\/\w]+).*/\1/'`
	if test "$SNMPVERSIONOK" == "OK"; then
	        AC_MSG_RESULT(yes)
            export SNMP_BUILD=1
	else
        AC_MSG_RESULT(no)
        export SNMP_BUILD=0
        sleep 2
		AC_MSG_WARN([
********************************************
*** SNMP Server will not be built .      ***
*** Need netsnmp version 5.3.0 or higher.***
*********************************************
])
	fi
])


