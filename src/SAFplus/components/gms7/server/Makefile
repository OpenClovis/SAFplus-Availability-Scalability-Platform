S7 := 1

include ../../../mk/preface.mk
SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

SERVER_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp)
SERVER_SRC := $(wildcard *.cxx)
SERVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))

DEP_LIBS := $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so

all: gmsServices

gmsServices:  $(OBJ_DIR)/groupServer.o $(OBJ_DIR)/MsgHandlerProtocols.o  $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SERVER_OBJ) gmsServices

cleanall:
	rm -f $(OBJ_DIR)/*.o $(MWOBJ_DIR)/*.o $(LIB_DIR)/*.so gmsServices

include ../../../mk/safplus_targets.mk
