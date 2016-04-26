# This file overrides the relevant make variables to create a cross compilation of SAFplus for ARM machines.
# It expects that you have:

# 1. Install Yocto build environment, build an image with some additional components.
# 1a. Install meta-openembedded and add these layers by setting the poky/build/conf/bblayers.conf BBLAYERS variable (see Yocto documentation).
# Add this to your poky/build/conf/local.conf to build the additional components:
# IMAGE_INSTALL_append = "  gdb gcc g++ openssh python-core boost glib-2.0 dbus zlib libxml2 e2fsprogs sqlite3 db "
# (gdb gcc g++ and openssh are not strictly necessary)

# 2. Installed the Yocto crossbuild, and sourced the appropriate file to set up the build environment

# Next, run:
# make CROSS=qemuarm.mk cross_setup
# Now, go to SAFplus/src/3rdparty/base and build any missing dependencies.  The only one you should need is Google protobufs:
# (cd SAFplus/src/3rdparty/base; make CROSS=qemuarm.mk protobufs)
# Now you can build SAFplus:
# (cd SAFplus/src; make CROSS=qemuarm.mk)

# Output files are located at: SAFplus/target/qemuarm.  Copy these to the target:
# (cd SAFplus/target/qemuarm; scp -r bin lib plugin test share root@192.168.7.2:/code; scp -r install/lib/* root@192.168.7.2:/code/lib)
# You need to copy both the SAFplus files and the 3rdparty libs (located in install/lib).

# Now you should be able to go to the embedded machine and run SAFplus in the normal manner.

# NOTE: This file overrides specific definitions located in "preface.mk" so please refer to that file to understand this one.

# YOU MUST CHANGE:

# Set this to where you copied your target's system libraries and headers
CROSS_SYS_ROOT:=/clovis/git/poky/build/tmp/sysroots/qemuarm

# Which protobuf version does your embedded platform supply? Add a "p" in front, i.e. 2.6.1 -> p2.6.1.  To figure this out, run "bitbake-layers show-recipes protobuf", if not > 2.5.0 build protobufs in 3rdparty/base)
PROTOBUFVER:=p2.6.1

# Select which database you want
SAFPLUS_WITH_GDBM:=false
SAFPLUS_WITH_BERKELEYDB:=false
#SAFPLUS_WITH_SQLITE := false

# Select other compilation options
SAFPLUS_WITH_BOOST_CHRONO:=false

# Skip Python (because it is not included by default in the Yocto images)
# Languages := 

# Override the target platform because ARM compiled code differs based on the architecture and thumb instruction set.
# So getting the target platform by asking the compiler is not sufficient
TARGET_PLATFORM?=qemuarm


# YOU MAY NEED TO CHANGE:

# Let's make sure that the user has "sourced" the Yocto build environment
ifeq ($(strip $(CROSS_COMPILE)),)
$(error Yocto build environment has not been sourced!  This is typically located in a /opt/poky/<ver>/environment-setup* file.)
endif
ifeq ($(strip $(CXX)),)
$(error Yocto build environment has not been sourced!  This is typically located in a /opt/poky/<ver>/environment-setup* file.)
endif

# Define the prefix used for all cross compilation tools.
CROSSPFX:=$(patsubst %-, %,$(CROSS_COMPILE))

# Specify the name of the compiler.
$(echo Using compiler supplied by Yocto poxy setup file: $(CXX))
COMPILER:=$(strip $(CXX))

# Set up the pkg-config program to point to the crossbuild environment not the local environment
PKG_CONFIG_PATH = $(INSTALL_DIR)/lib/pkgconfig:$(CROSS_SYS_ROOT)/usr/lib/pkgconfig
PKG_CONFIG = PKG_CONFIG_LIBDIR=$(INSTALL_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# Have to override this because pkg-config is returning a bogus directory
XML2_CFLAGS = -I$(SYSTEM_INC_DIR)/libxml2

# We are not building a .deb or .rpm
USE_DIST_LIB := 0

# Point sysroot to where you copied the embedded libraries, AND include -L paths for the same directories "outside" of sysroot (perhaps needed due to compiler/linker bug?)
LINK_FLAGS += -L$(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt/lib -L$(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt/lib3rdparty --sysroot=$(CROSS_SYS_ROOT)  -L/opt/safplus/7.0/tgt/lib -L/opt/safplus/7.0/tgt/lib3rdparty -lprotobuf -lprotoc

# Define the all target here so that it remains the default target
all:

# Set up library directories underneath your CROSS_SYS_ROOT so that the crossbuild linker can find SAFplus and 3rdparty libs when the --sysroot flag is used
cross_setup:
	mkdir -p $(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt
	mkdir -p $(INSTALL_DIR)/lib
	(cd $(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt; ln -s $(INSTALL_DIR)/lib lib3rdparty)
	(cd $(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt; ln -s $(LIB_DIR) lib)
	(cd $(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt; ln -s $(PLUGIN_DIR) plugin)

cross_cleanup:
	(cd $(CROSS_SYS_ROOT)/opt/safplus/7.0/tgt; rm *)

