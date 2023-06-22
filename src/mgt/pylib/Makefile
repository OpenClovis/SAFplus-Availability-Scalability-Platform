S7 := 1
SAFPLUS_DBAL_PYLIB :=1
include ../../mk/preface.mk

CP=cp -f
PYTHON_VER :=$(shell python3 -c "import sys; print (\"%d.%d\" % (sys.version_info[0], sys.version_info[1]))")
CPP_FLAGS +=-I$(SYSTEM_INC_DIR) -I$(SYSTEM_INC_DIR)/python$(PYTHON_VER)m -I$(SYSTEM_INC_DIR)/python$(PYTHON_VER)

# Specify the required libraries
SAFPLUS_LIBS := clMgt clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_PYLIB_H   := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_PYLIB_SRC := $(wildcard *.cxx)
CLIENT_PYLIB_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_PYLIB_SRC)))

PYLIB_SRC := $(LIB_DIR)/safplusMgtDb.py $(LIB_DIR)/common.py $(LIB_DIR)/microdom.py $(LIB_DIR)/pyDbal.py

Release all: $(LIB_DIR)/pyDbal.so $(BIN_DIR)/safplus_db

$(LIB_DIR)/pyDbal.so: $(CLIENT_PYLIB_OBJ) $(PYLIB_SRC) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_PYLIB_OBJ) $(SAFPLUS_LINK_LIBS) $(XML2_LINK) $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_PYLIB_H)
	$(COMPILE_CPP) $@ $(<F)

$(BIN_DIR)/safplus_db: safplus_db $(LIB_DIR)/safplusMgtDb.py $(LIB_DIR)/pyDbal.so
	$(CP) $< $@

$(LIB_DIR)/%.py: %.py
	$(CP) $< $@

clean:
	rm -f $(LIB_DIR)/pyDbal.so $(CLIENT_PYLIB_OBJ) $(PYLIB_SRC)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
