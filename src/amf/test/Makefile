S7 := 1
SAFPLUS_AMF_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clRpc clName clCkpt clGroup clMsg clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/exampleSafApp $(TEST_DIR)/exampleNonSafApp

$(TEST_DIR)/exampleSafApp:  $(OBJ_DIR)/exampleSafApp.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

exampleNonSafApp:
#TODO:
	$(shell touch ./exampleNonSafApp) 

$(TEST_DIR)/exampleNonSafApp: exampleNonSafApp
	cp -rf exampleNonSafApp $@

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/example

# TAE packaging stuff
IMAGE_FILE ?= amfTest.zip

image: $(IMAGE_FILE)

$(IMAGE_FILE): $(TEST_DIR)/exampleSafApp
	cp setup start cleanup $(SAFPLUS_TARGET)  
	cp SAFplusAmf.idx SAFplusAmf.db SAFplusAmf.xml $(SAFPLUS_TARGET)/test
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -x "(cp setup start cleanup {image_dir})" $@


include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
