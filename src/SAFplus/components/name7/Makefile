S7 := 1
include ../../mk/preface.mk

NAME_CLIENT_SRC := $(wildcard *.cxx)
NAME_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(NAME_CLIENT_SRC)))

SAFPLUS_SDK_TARGET=$(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

DEP_LIBS := $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal7.so

all: $(LIB_DIR)/libclName.so

$(SAFPLUS_TARGET)/lib/libclName.so: $(NAME_CLIENT_OBJ) Makefile
	$(LINK_SO) $@  $(NAME_CLIENT_OBJ) -lpthread -lrt -L$(SAFPLUS_SDK_TARGET)/lib $(DEP_LIBS) -lmw -ldl

$(MWOBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclName.so $(MWOBJ_DIR)/*.o
