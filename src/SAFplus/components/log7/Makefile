S7 := 1
SAFPLUS_LOG_LIB:=1
include ../../mk/preface.mk

LOG_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
LOG_CLIENT_SRC := $(wildcard *.cxx)
LOG_CLIENT_OBJ := $(subst .cxx,.o,$(LOG_CLIENT_SRC))
LOG_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(LOG_CLIENT_SRC)))

all: $(LIB_DIR)/libclLog.so

$(SAFPLUS_TARGET)/lib/libclLog.so: $(LOG_CLIENT_OBJ) $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so
	$(LINK_SO) $@ $+  $(LIB_DIR)/libclUtils7.so $(LINK_SO_LIBS)

#$(MWOBJ_DIR)/pywrapper.o: pywrappper.cxx Makefile $(LOG_SERVER_H)
#	$(COMPILE_CPP) $(PY_INC) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP)  $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/lib/clLog.o $(MWOBJ_DIR)/*.o

include $(MAKE_DIR)/safplus_targets.mk