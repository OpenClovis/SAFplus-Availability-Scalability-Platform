S7 := 1
SAFPLUS_FAULT_LIB := 1
include ../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. $(SAFPLUS_MGT_INC_FLAGS)

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := FaultSharedMem.cxx Fault.cxx FaultHistoryEntity.cxx FaultStatistic.cxx
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CLIENT_DEFAULLT_SRC := AmfFaultPolicy.cxx
CLIENT_DEFAULT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_DEFAULLT_SRC)))

CLIENT_CUSTOM_SRC := CustomFaultPolicy.cxx
CLIENT_CUSTOM_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_CUSTOM_SRC)))


# Specify the required libraries
SAFPLUS_LIBS := clCkpt clName clMsg clLog clUtils clOsal clGroup clMgt
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

 


all: $(LIB_DIR)/AmfFaultPolicy.so $(LIB_DIR)/CustomFaultPolicy.so $(LIB_DIR)/libclFault.so $(LIB_DIR)/libclFault.a 

$(LIB_DIR)/AmfFaultPolicy.so: $(CLIENT_DEFAULT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_DEFAULT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/CustomFaultPolicy.so: $(CLIENT_CUSTOM_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_CUSTOM_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/libclFault.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/libclFault.a: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclFault.so $(MWOBJ_DIR)/Fault.o $(MWOBJ_DIR)/Faultshm.o $(LIB_DIR)/AmfFaultPolicy.so $(LIB_DIR)/CustomFaultPolicy.so $(MWOBJ_DIR)/FaultStatistic.o $(MWOBJ_DIR)/FaultHistoryEntity.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
