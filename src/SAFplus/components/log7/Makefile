S7 := 1
include ../../mk/preface.mk

LOG_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
LOG_CLIENT_SRC := clLogCommon.cpp  clLog.cpp
LOG_CLIENT_OBJ := $(subst .cpp,.o,$(LOG_CLIENT_SRC))
LOG_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cpp,.o,$(LOG_CLIENT_SRC)))

all: $(LIB_DIR)/libclLog.so

$(SAFPLUS_TARGET)/lib/libclLog.so: $(LOG_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cpp Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/lib/clLog.o $(MWOBJ_DIR)/*.o
