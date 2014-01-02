S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(subst .cpp,.o,$(CLIENT_SRC))

# TODO use proper prefix variable
# LINK_STD_LIBS := /opt/clovis/6.1/sdk/prebuild/target/x86_64/linux-3.5.7/lib/libmw.so $(LINK_STD_LIBS)

all: testLog

testLog: $(CLIENT_OBJ)
	$(LINK) $+ $(LINK_LIBS) ../clLog.so ../../utils7/clUtils.so ../../osal7/clOsal.so  $(LINK_STD_LIBS) 

%.o: %.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f *.o testLog