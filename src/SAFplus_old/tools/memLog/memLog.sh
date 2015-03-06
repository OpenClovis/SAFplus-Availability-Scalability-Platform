#!/bin/sh
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
##Author - A.R.Karthick (karthick@openclovis.com)
##Run the C code which strips Vikrams python implementation for the 
##same in terms of memory usage for large files. Python seems to 
##suck a lot of memory. For example: It touches a whopping 701MB
##for a 110MB asp_amf log file

$ASP_BINDIR/memLog $*
