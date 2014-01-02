S7 := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(subst .cpp,.o,$(CLIENT_SRC))

all: clOsal.so

clOsal.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

%.o: %.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

