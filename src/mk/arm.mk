CROSSPFX := arm-linux-gnueabihf
COMPILER := $(CROSSPFX)-g++

PKG_CONFIG_PATH = $(INSTALL_DIR)/lib/pkgconfig

PKG_CONFIG = PKG_CONFIG_LIBDIR=$(INSTALL_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# SYSTEM_INC_DIR = /code/armEnv/include
# $(INSTALL_DIR)/include

USE_DIST_LIB := 0

LINK_FLAGS += --sysroot=/code/armEnv -L/lib/arm-linux-gnueabihf

SAFPLUS_WITH_GDBM := false
#SAFPLUS_WITH_SQLITE := false
