S7 := 1
include ../../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I..
COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 
LINK_EXE     := g++ -g -o 


LOG_SERVER_H := $(wildcard *.hpp) $(wildcard ../../../include7/*.hpp) 
LOG_SERVER_SRC := $(wildcard *.cpp) ../clLogCommon.cpp
LOG_SERVER_OBJ := $(subst .cpp,.o,$(LOG_SERVER_SRC))
LOG_SERVER_OBJ := $(addprefix $(SAFPLUS_TARGET)/obj/,$(LOG_SERVER_OBJ))

DEP_LIBS += $(LIB_DIR)/clOsal.so

all: $(SAFPLUS_TARGET)/bin/splogd

$(SAFPLUS_TARGET)/bin/splogd: $(LOG_SERVER_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(DEP_LIBS) $(LINK_STD_LIBS)

$(SAFPLUS_TARGET)/obj/%.o: %.cpp Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f *.o splogd

include ../../../mk/safplus_targets.mk