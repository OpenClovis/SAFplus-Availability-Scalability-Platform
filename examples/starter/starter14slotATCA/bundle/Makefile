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

VER ?= 1.3.0.0

all:  vipapp$(VER).tgz
	@echo 
	@echo Remember to use VER=a.b.c.d to create a custom versioned bundle
	@echo Software bundle $< created.

deploy/bin/vip: ../target/*/*/bin/App11plus1
	cp ../target/*/*/bin/App11plus1 deploy/bin/vip


vipapp$(VER).tgz: Makefile appcfgNoVer.xml deploy/bin/vip deploy/bin/virtualIp
	sed s/replaceWithTheVersion/$(VER)/g appcfgNoVer.xml > appcfg.xml
	tar cvfz vipapp$(VER).tgz --exclude=.svn appcfg.xml deploy


