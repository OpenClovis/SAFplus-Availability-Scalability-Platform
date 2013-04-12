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

# Deprecated:  alarm om cor med prov mso pm fault txn

SUBDIRS    = $(CM_DIR) ground \
             cnt    \
             buffer \
             osal   \
             timer  \
             utils  \
             eo     \
             log    \
             idl    \
             ioc    \
             rmd    \
             dbal   \
             debug  \
             event  \
             ckpt   \
             name   \
             sm     \
	     hal    \
	     msg    \
             gms    \
             ntf amf    

SUBDIRS += mw

ifeq ($(BUILD_SHARED), 1)
	SUBDIRS += ioc/plugins
endif

ifeq ($(SAFPLUS_BUILD_PYTHON), 1)
	SUBDIRS += python
endif
ifeq ($(SAFPLUS_BUILD_JAVA), 1)
	SUBDIRS += java
endif
ifeq ($(SAFPLUS_BUILD_CPP), 1)
        SUBDIRS += cpp
endif

#
# If you have any ordering dependence among subdirectories, list them as
# target-prerequisite pairs.
# 
#.PHONY: libs depend $(SUBDIRS) iocPlugins
#all1: libs $(SUBDIRS) iocPlugins

include $(CLOVIS_ROOT)/SAFplus/mk/make-subdir.mk

#iocPlugins:  
#	make -C ioc/plugins all