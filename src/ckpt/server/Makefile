S7 := 1

include ../../mk/preface.mk


#SERVER_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. #-I$(BOOST_DIR)/libs/filesystem/include 
SERVER_SRC := $(wildcard *.cxx)
SERVER_OBJ := $(subst .cxx,.o,$(notdir $(SERVER_SRC)))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/ckptsvr/,$(SERVER_OBJ))

# Specify the required libraries
SAFPLUS_LIBS := clFault clMsg clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(BIN_DIR)/safplus_ckpt

deps:
	#make -C $(SAFPLUS_SRC_DIR)/msg/transports
	#make -C $(SAFPLUS_SRC_DIR)/msg

$(BIN_DIR)/safplus_ckpt:  $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(SERVER_OBJ)  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) -lezxml $(LINK_STD_LIBS)

$(OBJ_DIR)/ckptsvr/%.o: %.cxx #$(SERVER_H)
	-mkdir -p $(OBJ_DIR)/ckptsvr/
	$(COMPILE_CPP) $@ $<


clean:
	rm -f $(OBJ_DIR)/ckptsvr/*.o
	rm -f $(CLIENT_OBJ) $(BIN_DIR)/safplus_ckpt

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
