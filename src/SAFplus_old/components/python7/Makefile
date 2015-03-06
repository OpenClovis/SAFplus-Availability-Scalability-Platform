S7 := 1
SAFPLUS_PY_LIB:=1
include ../../mk/preface.mk

PY_INC := -I/usr/include/python2.7
# Note to make boost_python: cd boost/libs/python/example/quickstart; ../../../../bjam toolset=gcc --verbose-test test
PY_LIB := -lpython2.7 -L$(BOOST_DIR)/libs -lboost_python

PY_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
PY_CLIENT_SRC := $(wildcard *.cxx)
PY_CLIENT_OBJ := $(subst .cxx,.o,$(PY_CLIENT_SRC))
PY_CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(PY_CLIENT_SRC)))

all: $(LIB_DIR)/pySAFplus.so

$(SAFPLUS_TARGET)/lib/pySAFplus.so:  $(PY_CLIENT_OBJ)  $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so
	$(LINK_SO) $@ $+  $(LIB_DIR)/libclLog.so $(PY_LIB) $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP)  $@ $(PY_INC) $<

clean:
	rm -f  $(PY_CLIENT_OBJ) $(LIB_DIR)/pySafplus.so

include $(MAKE_DIR)/safplus_targets.mk