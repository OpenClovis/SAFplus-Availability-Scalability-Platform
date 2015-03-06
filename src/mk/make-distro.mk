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



# The intention in this file is to capture the differences between the different
# Linux distributions but to do so in a way that does not identify the name
# of the distribution itself.  By not using the name, derivative distributions
# are more likely to work.

# On Ubuntu sourcing scripts with . causes problems, the script runs but "source" does
# not work.
RUN_SCRIPT := /bin/bash


# On Ubuntu when $(shell echo -e 'foo') is run it outputs '-e foo', almost as if
# make is cheating and executing an internal version of echo...
# An alternative ECHO := $(shell which echo)
ifdef SOLARIS_BUILD
    SHELL := /bin/bash
    ECHO  := echo 
else
    ECHO := /bin/echo
endif

# BerkeleyDB headers are sometimes not installed in /usr/include/db.h (SUSE)
# If not, coordinate the library version with the header version
ifdef SOLARIS_BUILD
CFLAGS += -I/usr/local/BerkeleyDB.4.2/include
LIB_BERKELEY_DB := -L/usr/local/BerkeleyDB.4.2/lib/ -ldb
else
LIB_BERKELEY_DB := -ldb
endif

EXISTS := $(shell ls /usr/include/db45/db.h 2> /dev/null)
ifneq ($(EXISTS),)
CFLAGS += -I/usr/include/db45
LIB_BERKELEY_DB := /usr/lib/libdb-4.5.a
else

EXISTS := $(shell ls /usr/include/db44/db.h 2> /dev/null)
ifneq ($(EXISTS),)
CFLAGS += -I/usr/include/db44
LIB_BERKELEY_DB := /usr/lib/libdb-4.4.a
else

EXISTS := $(shell ls /usr/include/db43/db.h 2> /dev/null)
ifneq ($(EXISTS),)
CFLAGS += -I/usr/include/db43
LIB_BERKELEY_DB := /usr/lib/libdb-4.3.a
else

EXISTS := $(shell ls /usr/include/db42/db.h 2> /dev/null)
ifneq ($(EXISTS),)
CFLAGS += -I/usr/include/db42
LIB_BERKELEY_DB = /usr/lib/libdb-4.2.a    # Verify on SUSE
else

EXISTS := $(shell ls /usr/include/db41/db.h 2> /dev/null)
ifneq ($(EXISTS),)
CFLAGS += -I/usr/include/db41
LIB_BERKELEY_DB = /usr/lib/libdb-4.1.a
endif
endif
endif
endif
endif
