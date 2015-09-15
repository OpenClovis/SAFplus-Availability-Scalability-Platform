akeS7 := 1
SAFPLUS_MSG_TEST:=1  # Identify what is being built, so safplus_targets does not override

include ../../mk/preface.mk

# DIRECTLY_LINK := $(LIB_DIR)/clMsgUdp.so

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clMsg clFault clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

IMAGE_FILE ?= testMsg.zip

all: $(TEST_DIR)/testTransport  $(TEST_DIR)/testMsgServer $(TEST_DIR)/testMsgPerf $(TEST_DIR)/msgReflector $(TEST_DIR)/reliableClient $(TEST_DIR)/reliableServer  $(TEST_DIR)/testSegmentationReceive $(TEST_DIR)/testSegmentationSend

deps:
	make -C $(SAFPLUS_SRC_DIR)/msg/transports
	make -C $(SAFPLUS_SRC_DIR)/msg

ifdef DIRECTLY_LINK
$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(DIRECTLY_LINK) $(SAFPLUS_LINK_LIBS) -lezxml $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP)  $@ -DDIRECTLY_LINKED $<
else
$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS)  $(SAFPLUS_LINK_LIBS) -lboost_program_options -lezxml $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<
endif

image: $(IMAGE_FILE)

$(IMAGE_FILE): $(TEST_DIR)/testLog
	cp setup start $(SAFPLUS_TARGET)
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -x "(cp setup start {image_dir})" $@


.PRECIOUS: $(OBJ_DIR)/%.o

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testTransport

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
