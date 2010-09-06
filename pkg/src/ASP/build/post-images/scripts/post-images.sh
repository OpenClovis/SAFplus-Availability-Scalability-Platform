#!/bin/bash
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
# Build: 4.1.0
#

##############################################################################
# Run model-specific post-images script for further customization
# and integration of target images by customer
##############################################################################

SCRIPT=${MODEL_PATH}/build/scripts/post-images.sh
if [ -f ${SCRIPT} ]; then
    chmod a+x ${SCRIPT}
    echo "Running post-images script..."
    ${SCRIPT}
    if [ $? -ne 0 ]; then
        echo "Failure in model post-images script"
        return 1
    fi
fi
