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

BUILDTOOLS_GLIB = $(wildcard $(TOOLCHAIN_DIR)/include/glib-2.0)

ifneq ($(BUILDTOOLS_GLIB),)
GLIB_INC = -I$(TOOLCHAIN_DIR)/include/glib-2.0 -I$(TOOLCHAIN_DIR)/lib/glib-2.0/include
else

GLIB_INC = -I/usr/include/glib-2.0 -I$(wildcard /usr/lib/*/glib-2.0/include/)
endif

# Possible CM directories
CM_SEARCH_PATH := $(CLOVIS_ROOT)/../PSP/src/cm $(CLOVIS_ROOT)/../../PSP/src/cm

USING_CM := $(wildcard $(CM_SEARCH_PATH))
ifneq ($(USING_CM),)

ifeq ($(HCL_CM),1)
        $(warning Using the actual chassis manager from the Platform Support Package located at $(USING_CM) and Radisys HPI)
	HPI_LIBS = -L$(TOOLCHAIN_DIR)/lib -lhcl -lopenhpiutils
	HPI_CFLAGS = -I$(TOOLCHAIN_DIR)/include/openhpi -I$(TOOLCHAIN_DIR)/include/radisys $(OPENHPICFLAGS)
else
ifeq ($(HPI_EMERSON),1)
        $(warning Using the actual chassis manager from the Platform Support Package located at $(USING_CM) and Emerson HPI)
	HPI_LIBS = -L$(TOOLCHAIN_DIR)/emerson/lib -lbbs-hpibmultishelf -lbbs-hpibcommon -lbbs-hpibutils
	HPI_CFLAGS = -I$(TOOLCHAIN_DIR)/emerson/include $(GLIB_INC)

else
        $(warning Using the actual chassis manager from the Platform Support Package located at $(USING_CM) and OpenHPI)
	HPI_LIBS = $(OPENHPILIBS)
	HPI_CFLAGS += $(OPENHPICFLAGS)
endif
endif

CM_DIR      := $(realpath $(USING_CM))

# The chassis mgr uses hpi data structures so I need the HPI includes but not the library
CL_CM       := -lClCm
CM_CFLAGS   := -DCL_USE_CHASSIS_MANAGER $(HPI_CFLAGS) -I$(CM_DIR)/include

CM_COMP_DEP := cm/client
CM_CONFLICT_RESOLVE_OBJS :=

else
HPI_LIBS :=
HPI_CFLAGS :=
CM_DIR :=
CL_CM  :=
CM_CFLAGS :=
$(warning Not using the chassis manager)
endif

endif