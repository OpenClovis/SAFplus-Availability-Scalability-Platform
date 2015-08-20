S7 := 1
SAFPLUS_PY_LIB:=1
include ../mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt clUtils clLog clCkpt clGroup clMsg clOsal clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

PY_INC := -I/usr/include/python2.7
# Note to make boost_python: cd boost/libs/python/example/quickstart; ../../../../bjam toolset=gcc --verbose-test test
PY_LIB := -lpython2.7 -L$(BOOST_DIR)/libs -lboost_python

PY_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
PY_CLIENT_SRC := $(wildcard *.cxx)
PY_CLIENT_OBJ := $(subst .cxx,.o,$(PY_CLIENT_SRC))
PY_CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(PY_CLIENT_SRC)))

all: $(LIB_DIR)/pySAFplus.so $(LIB_DIR)/amfctrl.py $(LIB_DIR)/safplus.py

$(LIB_DIR)/%.py: %.py
	cp $< $@

$(SAFPLUS_TARGET)/lib/pySAFplus.so:  $(PY_CLIENT_OBJ)  $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(PY_CLIENT_OBJ) $(LIB_DIR)/libclLog.so $(PY_LIB) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP)  $@ $(PY_INC) $<

clean:
	rm -f  $(PY_CLIENT_OBJ) $(LIB_DIR)/SAFplus.so

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
