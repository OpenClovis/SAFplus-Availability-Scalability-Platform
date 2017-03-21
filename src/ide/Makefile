S7 := 1
include ../mk/preface.mk
IDE_TOP_DIR = $(SAFPLUS_TOP_DIR)/bin
NOOP := $(shell mkdir -p $(IDE_TOP_DIR))
IDE_DIR = $(IDE_TOP_DIR)/ide
Release all: ide

PYTHON_FILES := $(wildcard *.py)
# $(warning $(PYTHON_FILES))
PYTHON_OBJECTS := $(addprefix $(IDE_DIR)/,$(subst .py,.pyc,$(PYTHON_FILES)))
# $(warning $(PYTHON_OBJECTS))

ide: $(PYTHON_OBJECTS) $(BIN_DIR)/safplus_ide $(IDE_DIR)/SAFplusAmf.yang
	cp -rLf resources $(IDE_DIR)
	cp -rLf codegen $(IDE_DIR)

$(IDE_DIR)/SAFplusAmf.yang: $(SAFPLUS_SRC_DIR)/yang/SAFplusAmf.yang
	cp -f $< $@

$(BIN_DIR)/safplus_ide: safplus_ide 
	cp -f safplus_ide $(IDE_TOP_DIR)/safplus_ide

$(IDE_DIR)/%.pyc: %.py
	mkdir -p $(IDE_DIR)
	python -m compileall $<
	cp $<c $@


codeblocks_ide:
	INSTALL_DIR=$(INSTALL_DIR) $(SAFPLUS_CODEBLOCKS_BIN_DIR)codeblocks --build ide.cbp

clean:
	rm -rf .objs *.pyc SAFplusIDE.cbplugin SAFplusIDE.so SAFplusIDE.zip helpers.zip wizard.zip

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
