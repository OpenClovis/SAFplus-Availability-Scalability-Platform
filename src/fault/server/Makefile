S7 := 1
SAFPLUS_FAULT_SERVER := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. $(SAFPLUS_MGT_INC_FLAGS)

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := FaultSharedMem.cxx faultServer.cxx FaultHistoryEntity.cxx FaultStatistic.cxx
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CLIENT_DEFAULT_SRC := AmfFaultPolicy.cxx
CLIENT_DEFAULT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_DEFAULT_SRC)))

CLIENT_CUSTOM_SRC := CustomFaultPolicy.cxx
CLIENT_CUSTOM_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_CUSTOM_SRC)))

ALARMTCMIB_H := $(wildcard IANAITUALARMTCMIB/*.hxx) $(wildcard ITUALARMTCMIB/*.hxx)
ALARMTCMIB_SRC := $(wildcard IANAITUALARMTCMIB/*.cxx) $(wildcard ITUALARMTCMIB/*.cxx)
ALARMTCMIB_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(ALARMTCMIB_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clName clMsg clLog clUtils clOsal clGroup clMgt clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

all: $(PLUGIN_DIR)/amfFaultPolicy.so $(PLUGIN_DIR)/customFaultPolicy.so $(LIB_DIR)/libclFaultServer.so $(LIB_DIR)/libclFaultServer.a

$(OBJ_DIR)/%.o: %.cxx Makefile $(ALARMTCMIB_H)
	-mkdir -p $(OBJ_DIR)/IANAITUALARMTCMIB
	-mkdir -p $(OBJ_DIR)/ITUALARMTCMIB
	$(COMPILE_CPP) $@ $<
    
$(PLUGIN_DIR)/amfFaultPolicy.so: $(CLIENT_DEFAULT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_DEFAULT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(PLUGIN_DIR)/customFaultPolicy.so: $(CLIENT_CUSTOM_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_CUSTOM_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/libclFaultServer.so: $(CLIENT_OBJ) $(ALARMTCMIB_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(ALARMTCMIB_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/libclFaultServer.a: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclFaultServer.so $(PLUGIN_DIR)/amfFaultPolicy.so $(PLUGIN_DIR)/customFaultPolicy.so $(CLIENT_OBJ)

codegen:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/yang/ITU-ALARM-TC-MIB.yang --y2cpp-output `pwd` --y2cpp-sdkdir $(SAFPLUS_SRC_DIR)
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/yang/IANA-ITU-ALARM-TC-MIB.yang --y2cpp-output `pwd` --y2cpp-sdkdir $(SAFPLUS_SRC_DIR)
include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
