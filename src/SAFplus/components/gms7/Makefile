S7 := 1
SAFPLUS_LOG_LIB := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

DEP_LIBS +=$(LIB_DIR)/libclLog.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclOsal7.so

all: $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclGroup.a

$(LIB_DIR)/libclGroup.so: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS)

$(LIB_DIR)/libclGroup.a: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclGroup.so $(MWOBJ_DIR)/group.o

include ../../mk/safplus_targets.mk
