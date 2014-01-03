S7 := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cpp,.o,$(CLIENT_SRC)))

all: $(LIB_DIR)/clOsal.so

$(LIB_DIR)/clOsal.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

