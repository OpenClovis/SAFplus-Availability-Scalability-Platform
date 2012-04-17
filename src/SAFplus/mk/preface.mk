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


# This file is included at the top of all other makefiles
# It discovers the environment and sets standard variables.


# If the chassis manager directory is not defined go look for it
ifndef CM_DIR

# Possible CM directories
CM_SEARCH_PATH := $(CLOVIS_ROOT)/../PSP/src/cm $(CLOVIS_ROOT)/../../PSP/src/cm

USING_CM := $(wildcard $(CM_SEARCH_PATH))
ifneq ($(USING_CM),)
$(warning Using the actual chassis manager from the Platform Support Package located at $(USING_CM))
CM_DIR      := $(realpath $(USING_CM))
CL_CM       := -lClCm
CM_CFLAGS   := -DCL_USE_CHASSIS_MANAGER -I$(CM_DIR)/include
CM_COMP_DEP := cm/client
CM_CONFLICT_RESOLVE_OBJS :=
else
CM_DIR :=
CL_CM  :=
CM_CFLAGS :=
CM_COMP_DEP :=
CM_CONFLICT_RESOLVE_OBJS := $(BASE_OBJ_DIR)/components/ground/client/clGroundCm.o
$(warning Not using the chassis manager)
endif

endif