S7 := 1
include ../../mk/preface.mk

NAME_CLIENT_SRC := $(wildcard *.cxx)
NAME_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(NAME_CLIENT_SRC)))

SAFPLUS_SDK_TARGET=$(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(CL_TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clUtils7 clOsal7 clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclName.so

$(SAFPLUS_TARGET)/lib/libclName.so: $(NAME_CLIENT_OBJ) Makefile $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(NAME_CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
#-lpthread -lrt -L$(SAFPLUS_SDK_TARGET)/lib -lmw -ldl

$(MWOBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclName.so $(MWOBJ_DIR)/*.o
