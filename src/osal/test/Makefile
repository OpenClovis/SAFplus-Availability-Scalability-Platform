S7 := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp)

# Specify the required libraries
SAFPLUS_LIBS := clOsal clLog clUtils 
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(TEST_DIR)/testOsal $(TEST_DIR)/testGate

$(TEST_DIR)/testOsal: $(OBJ_DIR)/testOsal.o $(OBJ_DIR)/testNodeStats.o
	$(LINK_EXE) $@ $+ $(SAFPLUS_LINK_LIBS)  $(LINK_SO_LIBS)

$(TEST_DIR)/testGate: $(OBJ_DIR)/testGate.o $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(SAFPLUS_LINK_LIBS)  $(LINK_SO_LIBS)

#$(TEST_DIR)/testNodeStats: $(CLIENT_OBJ) $(DEP_LIBS)
#	$(LINK_EXE) $@ $+ $(SAFPLUS_LINK_LIBS)  $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<


image: $(IMAGE_FILE)

$(IMAGE_FILE): $(TEST_DIR)/testOsal
	cp setup start $(SAFPLUS_TARGET)
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py  -a $(TARGET_PLATFORM) -x "(cp setup start {image_dir})" $@

.PRECIOUS: $(OBJ_DIR)/%.o

clean:
	rm $(TEST_DIR)/testOsal $(CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
