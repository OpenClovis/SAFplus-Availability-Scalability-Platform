# This file overrides the relevant make variables to create a cross compilation of SAFplus for ARM machines.
# It expects that you have installed the ARM toolchain (apt-get install g++-arm-linux-gnueabihf)
# and copied all system libraries and headers (/lib, /include, /usr/lib, /usr/include, /usr/local/lib, /usr/local/include) over to the development machine.

# This file overrides specific definitions located in "preface.mk" so please refer to that file to understand this one.

# YOU MUST CHANGE:

# Set this to where you copied your target's system libraries and headers
CROSS_SYS_ROOT := /code/armEnv

# Select which database you want
SAFPLUS_WITH_GDBM := false
#SAFPLUS_WITH_SQLITE := false


# YOU MAY NEED TO CHANGE:

# Define the prefix used for all cross compilation tools.
CROSSPFX := arm-linux-gnueabihf

# Specify the name of the compiler.
COMPILER := $(CROSSPFX)-g++

# Set up the pkg-config program to point to the crossbuild environment not the local environment
PKG_CONFIG_PATH = $(INSTALL_DIR)/lib/pkgconfig
PKG_CONFIG = PKG_CONFIG_LIBDIR=$(INSTALL_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# We are not building a .deb or .rpm
USE_DIST_LIB := 0

# Point sysroot to where you copied the embedded libraries
LINK_FLAGS += --sysroot=$(CROSS_SYS_ROOT) -L/lib/arm-linux-gnueabihf

