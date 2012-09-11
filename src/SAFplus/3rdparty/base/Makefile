# There are several steps to get this makefile working

# --- 1 --- Choose your installation directory

# If you are cross compiling you must also specify a different installation target, or the executables built for your target will pollute your host machine!

# Example using powerpc-unknown-linux-gnu cross compile toolchain
#InstDir:=/opt/clovis/buildtools/powerpc-unknown-linux-gnu
InstDir ?= $(PREFIX)

# /usr/local is the standard installation path
ifeq ($(InstDir),)
InstDir:=/usr/local
endif

# --- 2 --- Choose Cross Compilation Target

# If you are cross compiling, set 'Xtarget' to the cross-compilation executable prefix (the argument to --host= in 'normal' .configure scripts).  For example, if your gcc binary is called "i586-wrs-linux-gnu-gcc" then the Xtarget should be "i586-wrs-linux-gnu".  You can generally discover this name by going into your cross-compilation install directory and doing a "find" on a known binary, for example 'find <cross-comp install base dir> -name "*-gcc" -print'.
Xtarget:=$(TARGET)
# Example using PNE 1.4
#Xtarget:=i586-wrs-linux-gnu
# Example using clovis el4 cross compile toolchain
#Xtarget:=i686-unknown-linux-gnu

# --- 3 --- Set Up Environment

# If you are cross compiling, set this to the necessary commands to set up your cross-compilation environment.
# Of course, if your setup is complex it can be moved to a separate script, and be "source"d from this variable.
# If you are not cross-compiling, echo "" is a nop
Xenv:=

#Example using PNE 1.4 with ASP installed /opt/clovis
#Xenv:=PATH=/opt/clovis/buildtools/i586-wrl-pnele1.4-2.6.14-cp3020/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/i586-wrl-pnele1.4-2.6.14-cp3020/lib

# Example using clovis el4 cross compile toolchain
#Xenv:=PATH=/opt/clovis/buildtools/ia32-linux-rhel4-2.6.9/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/ia32-linux-rhel4-2.6.9/lib

# Example using powerpc-unknown-linux-gnu cross compile toolchain
#Xenv:=PATH=/opt/clovis/buildtools/powerpc-unknown-linux-gnu/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/powerpc-unknown-linux-gnu/lib

# --- 4 --- Change the dependents package version here
# These are packages that buildtool depends upon. 
#PKGCONFIG=pkgconfig
#PKGCONFIG_VER=0.6.0
#PKGCONFIG_PKG=$(PKGCONFIG)-$(PKGCONFIG_VER).tar.gz

NETSNMP=net-snmp
NETSNMP_VER=5.4.4
NETSNMP_PKG=$(NETSNMP)-$(NETSNMP_VER).tar.gz

OPENHPI=openhpi
OPENHPI_VER=3.0.0.oc
OPENHPI_PKG=$(OPENHPI)-$(OPENHPI_VER).tar.gz

GLIB=glib
GLIB_VER=2.28.5
GLIB_PKG=$(GLIB)-$(GLIB_VER).tar.gz

ZLIB=zlib
ZLIB_VER=1.2.7
ZLIB_PKG=$(ZLIB)-$(ZLIB_VER).tar.gz

OPENHPISUBAGENT=openhpi-subagent-2.3.4
OPENHPISUBAGENTPKG=$(OPENHPISUBAGENT).tar.gz
PYTHON=py-openhpi-1.1
PYTHONPKG=$(PYTHON).tar.gz

# THAT IS ALL
ifneq ($(Xtarget),)
  Xconfigure:=--host=$(Xtarget)
  Xargs:=CPPFLAGS="-I$(InstDir)/include -I$(InstDir)/$(Xtarget)/include -I$(InstDir)/glib-2.0/glib" LDFLAGS="-L$(InstDir)/lib -L$(InstDir)/$(Xtarget)/lib" CC=$(Xtarget)-gcc
ifeq ($(Xenv),)
  Xenv:=PATH=$(InstDir)/bin:$$PATH;LD_LIBRARY_PATH=$(InstDir)/lib
endif
else
  Xconfigure:=
  Xenv:=echo
endif

$(warning InstDir: $(InstDir))
$(warning Xtarget: $(Xtarget))
$(warning Xenv: $(Xenv))

all: $(InstDir) $(OPENHPI) $(NETSNMP) 
#$(OPENHPISUBAGENTPKG) $(PYTHONPKG)

ifneq ($(Xtarget),)

# ---- Extra configuration for NETSNMP
ifeq ($(ARCH),i386)
XconfNETSNMP_PKG:=--with-endianness=little
else 
ifeq ($(ARCH),x86_64)
XconfNETSNMP_PKG:=--with-endianness=little
else 
XconfNETSNMP_PKG:=--with-endianness=big
endif
# ---- End for NETSNMP

# ---- Extra configuration for OPENHPI
XconfOPENHPI_PKG:=--cache-file=cross.cache --enable-non32bit-int
XconfCROSS_OPENHPI:=ac_cv_lib_uuid_uuid_generate=yes
# ---- End for OPENHPI

# ---- Extra configuration for GLIB
XconfGLIB_PKG:=--cache-file=cross.cache --disable-dependency-tracking
#work-around for cross compile
XconfCROSS_GLIB:=glib_cv_stack_grows=no glib_cv_uscore=no ac_cv_func_posix_getgrgid_r=yes ac_cv_func_posix_getpwuid_r=yes
#CROSS:=$(shell touch $(GLIB)-$(GLIB_VER)/cross.cache)
#update := $(shell touch $(GLIB)-$(GLIB_VER)/cross.cache)
# ---- End for GLIB

endif
else
XconfNETSNMP_PKG:=
endif


$(InstDir):
	mkdir -p $@

$(NETSNMP): $(NETSNMP_PKG)
	(tar xvfz $<  > /dev/null)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfNETSNMP_PKG) \
		--without-rpm \
		--with-defaults \
		--with-default-snmp-version="2" \
		--disable-embedded-perl \
		--without-perl-modules)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); make)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); make install)

$(NETSNMP_PKG):
	wget http://sourceforge.net/projects/net-snmp/files/net-snmp/$(NETSNMP_VER)/$(NETSNMP)-$(NETSNMP_VER).tar.gz

$(OPENHPI): $(OPENHPI_PKG) $(GLIB) $(ZLIB)
	(tar xvfz $<  > /dev/null)
	(cd $(OPENHPI)-$(OPENHPI_VER); $(foreach check, $(XconfCROSS_OPENHPI), \
		echo $(check) >> cross.cache; \
	))
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); $(Xargs) LIBS="-lglib-2.0 -lgthread-2.0 -lrt" ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfOPENHPI_PKG))
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); make)
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); make install)

$(OPENHPI_PKG):
	wget http://sourceforge.net/projects/openhpi/files/openhpi-stable/$(OPENHPI_VER)/$(OPENHPI)-$(OPENHPI_VER).tar.gz

$(GLIB): $(GLIB_PKG)
	(tar xvfz $<  > /dev/null)
	(cd $(GLIB)-$(GLIB_VER); $(foreach check, $(XconfCROSS_GLIB), \
		echo $(check) >> cross.cache; \
	))
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfGLIB_PKG))
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); make)
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); make install)

$(GLIB_PKG):
	wget http://ftp.gtk.org/pub/glib/2.28/$(GLIB)-$(GLIB_VER).tar.gz

$(ZLIB): $(ZLIB_PKG) 
	(tar xvfz $<  > /dev/null)
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(XconfZLIB_PKG))
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); make)
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); make install)

$(ZLIB_PKG):
	wget http://zlib.net/$(ZLIB)-$(ZLIB_VER).tar.gz

