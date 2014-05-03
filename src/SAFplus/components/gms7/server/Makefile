S7 := 1
SAFPLUS_GROUP_SERVER_LIB := 1
include ../../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clGroup clName clIoc7 clUtils7 clLog clOsal7 
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(LIB_DIR)/libclGroupServer.so

$(LIB_DIR)/libclGroupServer.so: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclGroupServer.so $(MWOBJ_DIR)/groupMessageHandler.o $(MWOBJ_DIR)/GroupServer.o

include ../../../mk/safplus_targets.mk
