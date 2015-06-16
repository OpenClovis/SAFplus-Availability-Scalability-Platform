S7 := 1
include ../mk/preface.mk

Release all: ide standalone

ide:
	codeblocks --build ide.cbp

standalone:
	(cd standalone; codeblocks --build standalone.cbp)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk