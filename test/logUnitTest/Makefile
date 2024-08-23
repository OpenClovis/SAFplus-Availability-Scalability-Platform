S7 := 1
SAFPLUS_LOG_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../src/mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clMgt clName clCkpt clMsg clFault clTimer clDbal clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

IMAGE_FILE ?= logUnitTest.zip

all: $(TEST_DIR)/testLog

$(TEST_DIR)/testLog: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<


image: $(IMAGE_FILE)

$(IMAGE_FILE): $(TEST_DIR)/testLog
	cp start setup cleanup $(SAFPLUS_TARGET)
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -a $(TARGET_PLATFORM) -x "(cp start setup cleanup {image_dir})" $@


$(IMAGE_FILE)_old: $(TEST_DIR)/testLog
	@mkdir -p image
	@rm -f image/*
	(cp $(LIB_DIR)/*.so $(PLUGIN_DIR)/*.so image; cp -av $(INSTALL_DIR)/lib/*.so* image; cp $(LIB_DIR)/*.py image; cp $(TEST_DIR)/testLog image; cp start.sh image; cp $(BIN_DIR)/safplus_logd $(BIN_DIR)/dbalpy.py image)
	(cd image; tar cvfz ../$(IMAGE_FILE) *)

clean:
	rm -rf $(OBJ_DIR)/*.o $(TEST_DIR)/testLog image

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
