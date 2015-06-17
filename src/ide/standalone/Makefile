S7 := 1
include ../../mk/preface.mk

Release all: standalone

standalone:
	INSTALL_DIR=$(INSTALL_DIR) codeblocks --build standalone.cbp

clean:
	rm -rf obj bin

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk