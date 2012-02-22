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

include $(CLOVIS_ROOT)/SAFplus/mk/preface.mk

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

include $(CLOVIS_ROOT)/SAFplus/mk/make-subdir.mk
