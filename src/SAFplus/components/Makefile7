S7 := 1
include ../mk/preface.mk

all:
	echo blah

clean:
	rm -rf $(SAFPLUS_TARGET) $(MWOBJ_DIR) $(OBJ_DIR)

include ../mk/safplus_targets.mk

all: $(ThirdPartySOs) $(SAFplusTools) $(SAFplusSOs)  $(SAFplusServices) $(SAFplusTests) 

