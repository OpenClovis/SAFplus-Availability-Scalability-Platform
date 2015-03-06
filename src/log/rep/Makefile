S7 := 1
SAFPLUS_LOGREP_LIB := 1
include ../../mk/preface.mk

LOG_REP_SRC := $(wildcard *.cxx)
LOG_REP_OBJ := $(addprefix $(OBJ_DIR)/logrep/,$(subst .cxx,.o,$(LOG_REP_SRC)))

SAFPLUS_SDK_TARGET=$(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I.. -I../server/SAFplusLog -I../server/SAFplusTypes -I../server  -I$(COMMON_DIR)/SAFplusTypes $(SAFPLUS_MGT_INC_FLAGS)

# Specify the required libraries
#SAFPLUS_LIBS := clCkpt clUtils7 clOsal7 clLog
SAFPLUS_LIBS := clGroup
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

COMMON_DIR=$(SAFPLUS_SRC_DIR)/common

all: $(LIB_DIR)/libclLogRep.so

$(SAFPLUS_TARGET)/lib/libclLogRep.so: $(LOG_REP_OBJ) Makefile $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(LOG_REP_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
#-lpthread -lrt -L$(SAFPLUS_SDK_TARGET)/lib -lmw -ldl

$(OBJ_DIR)/logrep/%.o: %.cxx #Makefile
	-mkdir -p $(OBJ_DIR)/logrep 
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclLogRep.so $(OBJ_DIR)/logrep/*.o
