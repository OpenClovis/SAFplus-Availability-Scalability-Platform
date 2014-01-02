S7 := 1
include ../../mk/preface.mk

LOG_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) 
LOG_CLIENT_SRC := clLogCommon.cpp  clLog.cpp
LOG_CLIENT_OBJ := $(subst .cpp,.o,$(LOG_CLIENT_SRC))

all: clLog.so

clLog.so: $(LOG_CLIENT_OBJ)
	$(LINK_SO) clLog.so $+  -lpthread -lrt

%.o: %.cpp Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP) $@ $<

