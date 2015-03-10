S7 := 1
include ./mk/preface.mk

all:
	echo "completed"

clean:
	rm -rf $(SAFPLUS_TARGET) $(MWOBJ_DIR) $(OBJ_DIR)

include ./mk/safplus_targets.mk

all: $(ThirdPartySOs) $(SAFplusTools) $(SAFplusSOs) $(SAFplusPlugins) $(SAFplusServices) $(SAFplusTests) $(LIB_DIR)/libmw.so

$(LIB_DIR)/libmw.so: $(SAFplusSOs)
	$(LINK_SO) $@  $(MWOBJ_DIR)/*.o $(LINK_STD_LIBS)
