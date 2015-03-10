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

#ifdef S7  # SAFplus v7
$(info SAFplus7)

SAFPLUS_MAKE_DIR := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

SAFPLUS_SRC_DIR ?= $(shell (cd $(SAFPLUS_MAKE_DIR)../; pwd))
SAFPLUS_INC_DIR ?= $(SAFPLUS_SRC_DIR)/include
SAFPLUS_3RDPARTY_DIR ?= $(SAFPLUS_SRC_DIR)/3rdparty

PROTOBUF_LINK ?= `pkg-config --libs protobuf` -lprotoc
PROTOBUF_FLAGS ?= `pkg-config --cflags protobuf`

BOOST_DIR ?= $(shell (cd $(SAFPLUS_SRC_DIR)/../../boost; pwd))
CPP_FLAGS += -I$(SAFPLUS_INC_DIR)
CPP_FLAGS += -I$(BOOST_DIR) $(PROTOBUF_FLAGS) -I. -DSAFplus7
# we need to have -Wno-deprecated-warnings because boost uses std::auto_ptr
COMPILE_CPP = g++ -std=c++11 -Wno-deprecated-declarations  -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     = g++ -g -shared -o
LINK_EXE    = g++ -g -O0 -fPIC $(LINK_FLAGS) -o

LINK_LIBS ?=

LINK_STD_LIBS += $(PROTOBUF_LINK) -L$(BOOST_DIR)/stage/lib -L$(BOOST_DIR)/lib -lboost_thread -lboost_system -lboost_filesystem -lpthread -lrt -ldl
LINK_SO_LIBS += $(PROTOBUF_LINK) -L$(BOOST_DIR)/stage/lib -L$(BOOST_DIR)/lib -lboost_thread -lboost_system -lboost_filesystem -lpthread -lrt -ldl

TARGET_OS ?= $(shell uname -r)
TARGET_PLATFORM ?= $(shell uname -p)

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

#? Flags (include directories) needed to compile programs using the SAFplus Mgt component.
SAFPLUS_MGT_INC_FLAGS := -I$(SAFPLUS_SRC_DIR)/mgt -I$(SAFPLUS_SRC_DIR)/3rdparty/build/include/libxml2/ -I$(SAFPLUS_SRC_DIR)/3rdparty/base/libxml2-2.9.0/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

NOOP := $(shell mkdir -p $(SAFPLUS_SRC_DIR)/target/$(TARGET_PLATFORM)/$(TARGET_OS))
SAFPLUS_TARGET ?= $(shell (cd $(SAFPLUS_SRC_DIR)/target/$(TARGET_PLATFORM)/$(TARGET_OS); pwd))

NOOP := $(shell echo $(SAFPLUS_TARGET))

#? All 3rdparty libs, etc will go here by default i.e. configure --prefix=$(INSTALL_DIR)
INSTALL_DIR ?=  $(SAFPLUS_TARGET)/install
TEST_DIR ?= $(SAFPLUS_TARGET)/test
LIB_DIR ?= $(SAFPLUS_TARGET)/lib
BIN_DIR ?= $(SAFPLUS_TARGET)/bin
# All objects that should end up into libmw.so:
MWOBJ_DIR ?= $(SAFPLUS_TARGET)/mwobj
# All other objects
OBJ_DIR ?= $(SAFPLUS_TARGET)/obj

NOOP := $(shell mkdir -p $(INSTALL_DIR))
NOOP := $(shell mkdir -p $(TEST_DIR))
NOOP := $(shell mkdir -p $(LIB_DIR))
NOOP := $(shell mkdir -p $(BIN_DIR))
NOOP := $(shell mkdir -p $(MWOBJ_DIR))
NOOP := $(shell mkdir -p $(OBJ_DIR))

#Function to do codegen RPC from .yang
define SAFPLUS_MGT_RPC_GEN
	@PYTHONPATH=$$PYTHONPATH:$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$$PYANG_PLUGINPATH:$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $1.yang --y2cpp-output $2 --y2cpp-mgt $(MGT_SRC_DIR) --y2cpp-rpc
endef

#1. Google protoc generated
#2. Rename pb.h => pb.hxx, pb.cc => pb.cxx if param = true
#3. Code generated fro SAFplus RPC architecture 
define SAFPLUS_RPC_GEN
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib protoc -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --cpp_out=$2 $1.proto
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib $(SAFPLUS_TARGET)/bin/protoc-gen-rpc -I$(SAFPLUS_3RDPARTY_DIR) -I$(dir $1.proto) -I$(SAFPLUS_SRC_DIR)/rpc --rpc_out=$2 --rename=$3 $1.proto
endef

#endif
