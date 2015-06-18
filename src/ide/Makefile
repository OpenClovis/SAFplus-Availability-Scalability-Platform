S7 := 1
include ../mk/preface.mk

Release all: ide

ide:
	INSTALL_DIR=$(INSTALL_DIR) codeblocks --build ide.cbp

clean:
	rm -rf .objs *.pyc SAFplusIDE.cbplugin SAFplusIDE.so SAFplusIDE.zip helpers.zip wizard.zip

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk