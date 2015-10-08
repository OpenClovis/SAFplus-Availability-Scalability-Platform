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

# Link with Google's performance toolkit http://code.google.com/p/gperftools
# GPERFTOOLS := 1

#ifdef S7  # SAFplus v7
$(info SAFplus7)

#? By default we link with the local Linux distribution's installed libraries.  Override this to 0 if you are doing a crossbuild.
USE_DIST_LIB ?= 1  

SAFPLUS_MAKE_DIR := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

#$(info A: $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
#$(info B: $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))
#$(info C: $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST))))))
#$(info D: $(patsubst %/,%,$(dir $(realpath $(SAFPLUS_MAKE_DIR)))))

ifdef CROSS
include $(SAFPLUS_MAKE_DIR)/$(CROSS)
endif

SAFPLUS_SRC_DIR ?= $(patsubst %/,%,$(dir $(realpath $(SAFPLUS_MAKE_DIR))))
SAFPLUS_INC_DIR ?= $(SAFPLUS_SRC_DIR)/include
SAFPLUS_3RDPARTY_DIR ?= $(SAFPLUS_SRC_DIR)/3rdparty

SYSTEM_INC_DIR ?= /usr/include

$(info SAFplus source: $(SAFPLUS_SRC_DIR))
$(info SAFplus include: $(SAFPLUS_INC_DIR))

COMPILER ?= g++
LOCAL_COMPILER ?= g++

TMP := $(shell which protoc) # Forces the evaluation right now
PROTOC ?= $(TMP)

# we need to have -Wno-deprecated-warnings because boost uses std::auto_ptr
COMPILE_CPP ?= $(COMPILER) -std=c++11 -Wno-deprecated-declarations  -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     ?= $(COMPILER) $(LINK_FLAGS) -g -shared -o
LINK_EXE    ?= $(COMPILER) -g -O0 -fPIC $(LINK_FLAGS) -o

# Use these variables if you are building something to run on the local machine (a tool or something)
LOCAL_COMPILE_CPP ?= $(LOCAL_COMPILER) -std=c++11 -Wno-deprecated-declarations  -g -O0 -fPIC -c $(CPP_FLAGS) -o
LOCAL_LINK_SO     ?= $(LOCAL_COMPILER) $(LINK_FLAGS) -g -shared -o
LOCAL_LINK_EXE    ?= $(LOCAL_COMPILER) -g -O0 -fPIC $(LINK_FLAGS) -o
LOCAL_TARGET_OS ?= $(shell uname -r)
LOCAL_TARGET_PLATFORM ?= $(shell uname -p)

LINK_LIBS ?=

TARGET_OS ?= linux # $(shell uname -r)
TARGET_PLATFORM ?= `$(COMPILER) -dumpmachine` #$(shell uname -p)

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt
TAE_DIR ?= $(SAFPLUS_SRC_DIR)/../../tae

NOOP := $(shell mkdir -p $(SAFPLUS_SRC_DIR)/../target/$(TARGET_PLATFORM)/$(TARGET_OS))
SAFPLUS_TARGET ?= $(shell (cd $(SAFPLUS_SRC_DIR)/../target/$(TARGET_PLATFORM)/$(TARGET_OS); pwd))

# Put compilation tools (that you have to build) here
SAFPLUS_TOOL_TARGET ?= $(shell (cd $(SAFPLUS_SRC_DIR)/../target/$(LOCAL_TARGET_PLATFORM)/$(LOCAL_TARGET_OS); pwd))
$(info STT $(SAFPLUS_TOOL_TARGET))

SAFPLUS_CODEBLOCKS_BIN_DIR ?= /opt/SAFplus/7.0/ide/bin/

NOOP := $(shell echo $(SAFPLUS_TARGET))

#? All 3rdparty libs, etc will go here by default i.e. configure --prefix=$(INSTALL_DIR)
INSTALL_DIR ?=  $(SAFPLUS_TARGET)/install
TEST_DIR ?= $(SAFPLUS_TARGET)/test
LIB_DIR ?= $(SAFPLUS_TARGET)/lib
PLUGIN_DIR ?= $(SAFPLUS_TARGET)/plugin
BIN_DIR ?= $(SAFPLUS_TARGET)/bin
# All objects that should end up into libmw.so:
MWOBJ_DIR ?= $(SAFPLUS_TARGET)/mwobj
# All other objects
OBJ_DIR ?= $(SAFPLUS_TARGET)/obj

LOCAL_OBJ_DIR ?= $(SAFPLUS_TOOL_TARGET)/obj

NOOP := $(shell mkdir -p $(INSTALL_DIR))
NOOP := $(shell mkdir -p $(TEST_DIR))
NOOP := $(shell mkdir -p $(LIB_DIR))
NOOP := $(shell mkdir -p $(BIN_DIR))
NOOP := $(shell mkdir -p $(PLUGIN_DIR))
NOOP := $(shell mkdir -p $(MWOBJ_DIR))
NOOP := $(shell mkdir -p $(OBJ_DIR))
NOOP := $(shell mkdir -p $(SAFPLUS_TOOL_TARGET)/bin)


PKG_CONFIG_PATH ?= /lib/pkgconfig:$(INSTALL_DIR)/lib/pkgconfig
PKG_CONFIG ?= PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# Determine XML2 location
XML2_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libxml-2.0)
ifeq ($(XML2_CFLAGS),)
$(info pkg-config was unable to determine libxml-2.0 header file location.  Using default)
XML2_CFLAGS ?= -I$(SAFPLUS_SRC_DIR)/3rdparty/build/include/libxml2 -I$(SAFPLUS_SRC_DIR)/3rdparty/base/libxml2-2.9.0/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/
endif
XML2_LINK ?= $(shell $(PKG_CONFIG) --libs libxml-2.0)  
ifeq ($(XML2_LINK),)
$(info pkg-config was unable to determine libxml-2.0 location.  Using default)
XML2_LINK ?= $(INSTALL_DIR)/lib -lxml2
endif

#? Flags (include directories) needed to compile programs using the SAFplus Mgt component.
SAFPLUS_MGT_INC_FLAGS := -I$(SAFPLUS_SRC_DIR)/mgt $(XML2_CFLAGS)

# Determine boost location
# For SAFplus RPM/DEBIAN package generation, distribution provided libraries need to be used
ifeq ($(strip $(USE_DIST_LIB)),0)
DISTRIBUTION_LIB = 0
BOOST_INC_DIR ?= $(INSTALL_DIR)/include
# $(shell (cd $(SAFPLUS_SRC_DIR)/../../boost; pwd))
BOOST_LIB_DIR ?= $(INSTALL_DIR)/lib
else
DISTRIBUTION_LIB = 1
BOOST_INC_DIR := $(SYSTEM_INC_DIR)
# Need to fix the BOOST_LIB_DIR path for handling various linux distributions
BOOST_LIB_DIR ?= /usr/lib:/usr/lib64
endif

ifdef GPERFTOOLS
GPERFTOOLS_LINK := -ltcmalloc
else
GPERFTOOLS_LINK :=
endif

# Determine protobuf location
PROTOBUF_LINK ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs protobuf) -lprotoc
PROTOBUF_FLAGS ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags protobuf)
# $(info PROTOBUF_FLAGS is $(PROTOBUF_FLAGS) PROTOBUF_LINK is $(PROTOBUF_LINK))

LINK_STD_LIBS += $(PROTOBUF_LINK) -L$(BOOST_LIB_DIR) -lboost_thread -lboost_system -lboost_filesystem -lpthread -lrt -ldl $(GPERFTOOLS_LINK)
LINK_SO_LIBS += $(PROTOBUF_LINK) -L$(BOOST_LIB_DIR) -lboost_thread -lboost_system -lboost_filesystem -lpthread -lrt -ldl $(GPERFTOOLS_LINK)

CPP_FLAGS := $(CPP_FLAGS) -I$(SAFPLUS_INC_DIR) -I$(BOOST_INC_DIR) $(PROTOBUF_FLAGS) -I. -DSAFplus7

ifdef GNU_PROFILE
CPP_FLAGS += -pg
LINK_FLAGS += -pg
endif

PYANG_PLUGINPATH_EXT ?= $(PYANG_PLUGINPATH)
ifeq ($(PYANG_PLUGINPATH_EXT),)
export PYANG_PLUGINPATH:=$(shell pwd)
endif

SAFplusRpcGen ?= $(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc

#Function to do codegen RPC from .yang
define SAFPLUS_YANG_RPC_GEN
	PYTHONPATH=$$PYTHONPATH:$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$$PYANG_PLUGINPATH:$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp $(strip $1).yang --y2cpp-output=`pwd` --y2cpp-sdkdir=$(SAFPLUS_SRC_DIR) --y2cpp-rpc
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib protoc -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --cpp_out=$(strip $2) $(strip $1).proto
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib $(SAFplusRpcGen) -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --rpc_out=$(strip $2) --rpc_opts=$(strip $3) $(strip $1).proto
endef

#1. Google protoc generated
#2. Rename pb.h => pb.hxx, pb.cc => pb.cxx if param = true
#3. Code generated fro SAFplus RPC architecture 
define SAFPLUS_RPC_GEN
	$(PROTOC) -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --cpp_out=$2 $1.proto
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib $(SAFplusRpcGen) -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --rpc_out=$2 --rpc_opts=$(strip $3) $1.proto
endef

#endif
