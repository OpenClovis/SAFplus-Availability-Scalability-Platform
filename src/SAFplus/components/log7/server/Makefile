S7 := 1
include ../../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I..
COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 
LINK_EXE     := g++ -g -o 


LOG_SERVER_H := $(wildcard *.hpp) $(wildcard ../../../include7/*.hpp) 
LOG_SERVER_SRC := $(wildcard *.cpp) ../clLogCommon.cpp
LOG_SERVER_OBJ := $(subst .cpp,.o,$(LOG_SERVER_SRC))

# TODO put the libs in a single architecture-specific place & define var
LINK_LIBS += ../../osal7/clOsal.so

all: splogd

splogd: $(LOG_SERVER_OBJ)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(LINK_STD_LIBS)

%.o: %.cpp Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f *.o splogd