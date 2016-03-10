S7 := 1
include ../../../../../mk/preface.mk

LOG_REP_TEST_SRC := $(wildcard *.cxx)
LOG_REP_TEST_OBJ := $(addprefix $(OBJ_DIR)/logRepTest/,$(subst .cxx,.o,$(LOG_REP_TEST_SRC)))

COMMON_DIR=$(SAFPLUS_SRC_DIR)/SAFplus/components/common
#LOG_SERVER_H := $(wildcard ../../../server/SAFplusLog/*.hxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.hxx) $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp)
LOG_SERVER_SRC := $(wildcard ../../../server/SAFplusLog/*.cxx) $(wildcard ../../../server/*.cxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.cxx)
LOG_SERVER_OBJ := $(subst .cxx,.o,$(notdir $(LOG_SERVER_SRC)))
LOG_SERVER_OBJ := $(subst main.o,,$(LOG_SERVER_OBJ))
LOG_SERVER_OBJ := $(addprefix $(OBJ_DIR)/logsvr/,$(LOG_SERVER_OBJ))

SAFPLUS_SDK_TARGET=$(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I../.. -I../../.. -I../../../server/SAFplusLog -I../../../server/SAFplusTypes -I../../../server -I../../../../mgt7/include -I$(COMMON_DIR)/SAFplusTypes -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

# Specify the required libraries
#SAFPLUS_LIBS := clCkpt clUtils7 clOsal7 clLog
SAFPLUS_LIBS := clMgt7 clUtils7 clLog clOsal7 clIoc7 clName clGroup clCkpt clLogRep
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/logReader

$(TEST_DIR)/logReader: $(LOG_REP_TEST_OBJ)
	$(LINK_EXE) $@ $(LOG_REP_TEST_OBJ) $(LOG_SERVER_OBJ) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)
	#$(LINK_EXE) $@ $(LOG_REP_TEST_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

#-lpthread -lrt -L$(SAFPLUS_SDK_TARGET)/lib -lmw -ldl

$(OBJ_DIR)/logRepTest/%.o: %.cxx #Makefile
	-mkdir -p $(OBJ_DIR)/logRepTest
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(OBJ_DIR)/logRepTest/*.o $(TEST_DIR)/logReader
