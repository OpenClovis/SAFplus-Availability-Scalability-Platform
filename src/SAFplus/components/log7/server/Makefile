S7 := 1
SAFPLUS_LOG_SERVER:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. -I$(BOOST_DIR)/libs/filesystem/include 

# Specify the required libraries
SAFPLUS_LIBS := clMgt7 clUtils7 clLog clOsal7 clIoc7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

COMMON_DIR=$(SAFPLUS_SRC_DIR)/SAFplus/components/common

# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -ISAFplusLog -I$(COMMON_DIR)/SAFplusTypes -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

LOG_SERVER_H := $(wildcard SAFplusLog/*.hxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.hxx) $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp) 
LOG_SERVER_SRC := $(wildcard SAFplusLog/*.cxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.cxx) $(wildcard *.cxx)
LOG_SERVER_OBJ := $(subst .cxx,.o,$(notdir $(LOG_SERVER_SRC)))
LOG_SERVER_OBJ := $(addprefix $(OBJ_DIR)/logsvr/,$(LOG_SERVER_OBJ))

all: $(SAFPLUS_TARGET)/bin/splogd

$(SAFPLUS_TARGET)/bin/splogd: $(LOG_SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(LOG_SERVER_OBJ) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/logsvr/%.o: %.cxx Makefile $(LOG_SERVER_H)
	-mkdir -p $(OBJ_DIR)/logsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/logsvr/%.o: SAFplusLog/%.cxx
	-mkdir -p $(OBJ_DIR)/logsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/logsvr/%.o: $(COMMON_DIR)/SAFplusTypes/%.cxx
	-mkdir -p $(OBJ_DIR)/logsvr/
	(cd $(COMMON_DIR)/SAFplusTypes && $(COMPILE_CPP) $@ $(<F))

gen:	
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/SAFplusLog.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR)
	(rsync -avpDHL SAFplusTypes $(COMMON_DIR)/)
	(rm -rf  SAFplusTypes)

cleangen:
	(cd SAFplusLog; find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)
	(cd $(COMMON_DIR)/SAFplusTypes; find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)
	(cd $(COMMON_DIR)/ietfYangTypes; find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)

clean:
	rm -f $(OBJ_DIR)/logsvr/*.o $(SAFPLUS_TARGET)/bin/splogd

include ../../../mk/safplus_targets.mk
