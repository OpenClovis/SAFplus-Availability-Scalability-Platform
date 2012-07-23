################################################################################
#
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
#
# The source code for  this program is not published  or otherwise
# divested of  its trade secrets, irrespective  of  what  has been
# deposited with the U.S. Copyright office.
#
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
################################################################################
#
# Build: 4.2.0
#
################################################################################
# ModuleName  : Makefile
# File        : Makefile
################################################################################
# Description :
################################################################################

include $(CLOVIS_ROOT)/ASP/mk/preface.mk

# Subdirectories having Makefiles and need recursive escalation:
# common should always be before client and server

SUBDIRS    = $(CM_DIR) ground \
             cnt    \
             buffer \
             osal   \
             timer  \
             utils  \
             ioc    \
             rmd    \
             idl    \
             dbal   \
             eo     \
             debug  \
             amf    \
             alarm  \
             event  \
             ckpt   \
             name   \
             om    	\
             cor 	\
			 fault  \
             sm     \
             txn    \
	     	 med	\
	     	 prov 	\
	     	 hal 	\
	      	 gms	\
	      	 msg	\
             log    \
			 mso    \
			 pm     \
             ntf

SUBDIRS += mw

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs.
# 

include $(CLOVIS_ROOT)/ASP/mk/make-subdir.mk
