# There are several steps to get this makefile working
include ../../mk/preface.mk
# --- 0 --- Install required tools:

# sudo apt-get install curl autotools
# sudo apt-get install python-dev
# sudo apt-get install bzip2-dev

# --- 1 --- Choose your installation directory

# If you are cross compiling you must also specify a different installation target, or the executables built for your target will pollute your host machine!

# Example using powerpc-unknown-linux-gnu cross compile toolchain
#InstDir:=/opt/clovis/buildtools/powerpc-unknown-linux-gnu
InstDir ?= $(INSTALL_DIR)

# /usr/local is the standard installation path
ifeq ($(InstDir),)
InstDir:=/usr/local
endif

# --- 2 --- Choose Cross Compilation Target

# If you are cross compiling, set 'Xtarget' to the cross-compilation executable prefix (the argument to --host= in 'normal' .configure scripts).  For example, if your gcc binary is called "i586-wrs-linux-gnu-gcc" then the Xtarget should be "i586-wrs-linux-gnu".  You can generally discover this name by going into your cross-compilation install directory and doing a "find" on a known binary, for example 'find <cross-comp install base dir> -name "*-gcc" -print'.
Xtarget:=$(strip $(CROSSPFX))

$(info crossbuild: $(Xtarget))
$(info install dir: $(InstDir))

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
#Xenv:=PATH=/opt/clovis/buildtools/i586-wrl-pnele1.4-2.6.14-cp3020/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/i586-wrl-pnele1.4-2.6.14-cp3020/lib;export PKG_CONFIG_PATH=/opt/clovis/buildtools/i586-wrl-pnele1.4-2.6.14-cp3020/lib/pkgconfig/:$$PKG_CONFIG_PATH

# Example using clovis el4 cross compile toolchain
#Xenv:=PATH=/opt/clovis/buildtools/ia32-linux-rhel4-2.6.9/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/ia32-linux-rhel4-2.6.9/lib;export PKG_CONFIG_PATH=/opt/clovis/buildtools/ia32-linux-rhel4-2.6.9/lib/pkgconfig/:$$PKG_CONFIG_PATH

# Example using powerpc-unknown-linux-gnu cross compile toolchain
#Xenv:=PATH=/opt/clovis/buildtools/powerpc-unknown-linux-gnu/bin:$$PATH;LD_LIBRARY_PATH=/opt/clovis/buildtools/powerpc-unknown-linux-gnu/lib;expor PKG_CONFIG_PATH=/opt/clovis/buildtools/powerpc-unknown-linux-gnu/lib/pkgconfig/:$$PKG_CONFIG_PATH

# --- 4 --- Change the dependents package version here
# These are packages that buildtool depends upon. 

# Note: Some issue counting with cross compile:
# glib depend on dbus-1.0 lib, then need to install dbus-1.0 first:
#   + We can get dbus at http://dbus.freedesktop.org/releases/dbus/dbus-1.6.4.tar.gz
#   + tar xvf dbus-1.6.4.tar.gz
#   + cd dbus-1.6.4
#   + ./configure --prefix=/opt/clovis/buildtools/powerpc-unknown-linux-gnu --host=powerpc-unknown-linux-gnu
#   + make
#   + make install
#    
# dbus-1.6.4 depend on expat lib:
#   + We can get expat at http://nchc.dl.sourceforge.net/project/expat/expat/2.1.0/expat-2.1.0.tar.gz
#   + tar xvf expat-2.1.0.tar.gz
#   + cd expat-2.1.0
#   + ./configure --prefix=/opt/clovis/buildtools/powerpc-unknown-linux-gnu --host=powerpc-unknown-linux-gnu
#   + make
#   + make install

PROTOBUF_VER=v2.6.1

NETSNMP=net-snmp
NETSNMP_VER=5.4.4
NETSNMP_PKG=$(NETSNMP)-$(NETSNMP_VER).tar.gz

OPENHPI=openhpi
OPENHPI_VER=3.0.0
OPENHPI_PKG=$(OPENHPI)-$(OPENHPI_VER).tar.gz

E2FSPROGS=e2fsprogs
E2FSPROGS_VER=1.42.5
E2FSPROGS_PKG=$(E2FSPROGS)-$(E2FSPROGS_VER).tar.gz

GLIB=glib
GLIB_VER=2.28.5
GLIB_PKG=$(GLIB)-$(GLIB_VER).tar.gz

ZLIB=zlib
ZLIB_VER=1.2.11
ZLIB_PKG=$(ZLIB)-$(ZLIB_VER).tar.gz

DBUS=dbus
DBUS_VER=1.6.4
DBUS_PKG=$(DBUS)-$(DBUS_VER).tar.gz

EXPAT=expat
EXPAT_VER=2.1.0
EXPAT_PKG=$(EXPAT)-$(EXPAT_VER).tar.gz

OPENHPISUBAGENT=openhpi-subagent
OPENHPISUBAGENT_VER=2.3.4
OPENHPISUBAGENT_PKG=$(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER).tar.gz

LIBXML2=libxml2
LIBXML2_VER=2.9.0
LIBXML2_PKG=$(LIBXML2)-$(LIBXML2_VER).tar.gz

BOOST=boost
BOOST_VER=1.62.0
BOOST_PKG=$(BOOST)_$(subst .,_,$(BOOST_VER)).tar.gz
BOOST_FLAVOR=link=shared runtime-link=shared variant=debug threading=multi --without-python --without-iostreams --without-mpi --without-context

#PyHPI=py-openhpi
#PyHPI_VER=1.1
#PyHPI_PKG=$(PyHPI)-$(PyHPI_VER).tar.gz
#

PYTHON=Python
PYTHON_VER=2.7.10
PYTHON_PKG=$(PYTHON)-$(PYTHON_VER).tgz

PROTOBUF=protobuf
PROTOBUF_PKG=$(PROTOBUF).tar.gz

PYANG=pyang
PYANG_PKG=$(PYANG).tar.gz

PYTHON_VER :=$(shell python3 -c "import sys; print (\"%d.%d\" % (sys.version_info[0], sys.version_info[1]))")

# THAT IS ALL
ifneq ($(Xtarget),)
  CC?=$(strip $(Xtarget)-gcc)
  CXX?=$(strip $(Xtarget)-g++
  Xconfigure:=--host=$(strip $(Xtarget))
#  Xconfigure:=--build=$(Xtarget)
  Xargs:=CXXFLAGS="-I$(InstDir)/include -I$(InstDir)/$(Xtarget)/include -I$(InstDir)/glib-2.0/glib" CPPFLAGS="-I$(InstDir)/include -I$(InstDir)/$(Xtarget)/include -I$(InstDir)/glib-2.0/glib" LDFLAGS="-L$(InstDir)/lib -L$(InstDir)/$(Xtarget)/lib" CC="$(CC)" CXX="$(CXX)"
ifeq ($(Xenv),)
  Xenv:=export PKG_CONFIG_PATH=$(InstDir)/lib/pkgconfig/:$$PKG_CONFIG_PATH;PATH=$(InstDir)/bin:$$PATH;LD_LIBRARY_PATH=$(InstDir)/lib
endif
else
  Xconfigure:=
  Xenv:=echo
endif

GDBM=gdbm
GDBM_VER=1.13
GDBM_PKG=$(GDBM)-$(GDBM_VER).tar.gz

BERKELEY=db
BERKELEY_VER=6.2.32
BERKELEY_PKG=$(BERKELEY)-$(BERKELEY_VER).tar.gz

SQLITE=sqlite-autoconf
SQLITE_VER=3210000
SQLITE_PKG=$(SQLITE)-$(SQLITE_VER).tar.gz


#$(warning InstDir: $(InstDir))
#$(warning Xtarget: $(Xtarget))
#$(warning Xenv: $(Xenv))

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
XconfOPENHPI_PKG:=--cache-file=config.cache --enable-non32bit-int
XconfCROSS_OPENHPI:=ac_cv_lib_uuid_uuid_generate=yes
# ---- End for OPENHPI

# ---- Extra configuration for GLIB
XconfGLIB_PKG:=--cache-file=config.cache --disable-dependency-tracking
#work-around for cross compile
XconfCROSS_GLIB:=glib_cv_stack_grows=no glib_cv_uscore=no ac_cv_func_posix_getgrgid_r=yes ac_cv_func_posix_getpwuid_r=yes
# ---- End for GLIB

# ---- Extra configuration for XconfCROSS_PYTHON
XconfPYTHON_PKG:=--cache-file=config.cache
#work-around for cross compile
XconfCROSS_PYTHON:=ac_cv_have_long_long_format=yes
# ---- End for XconfCROSS_PYTHON
endif
else
XconfNETSNMP_PKG:=
endif

libs: $(InstDir) $(ZLIB) $(GDBM) $(BERKELEY) $(SQLITE) protobuffer $(LIBXML2) $(BOOST) pyyang
	echo building libs
#libs: $(InstDir) protobuffer pyyang
#	echo building libs

# $(NETSNMP)  $(OPENHPI) $(OPENHPISUBAGENT)
all: $(InstDir) $(ZLIB) $(GDBM) $(BERKELEY) $(SQLITE) protobuffer $(LIBXML2) $(BOOST) pyyang
#all: $(InstDir) protobuffer pyyang

wipe:
	rm -rf $(LIBXML2)-$(LIBXML2_VER)
	rm -rf protobuf

$(InstDir):
	mkdir -p $@

protobuf:
	git clone https://github.com/google/protobuf.git
	(cd protobuf; git checkout -b $(PROTOBUF_VER) $(PROTOBUF_VER))
	(cd protobuf; curl -L https://github.com/google/googletest/archive/release-1.5.0.tar.gz | tar zx)
	(cd protobuf; mv googletest-release-1.5.0 gtest)
	(cd protobuf; ./autogen.sh)

protobufs: protobuf
	(cd protobuf; $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) --with-protoc=$(shell which protoc) $(Xconfigure) )
	(cd protobuf; make )
	(cd protobuf; make install )

protobuffer: $(PROTOBUF_PKG)
	echo "REMOVING $(PROTOBUF)"
	(rm -rf $(PROTOBUF))
	(tar xvfz $<  > /dev/null)
	(cd protobuf; curl -L https://github.com/google/googletest/archive/release-1.5.0.tar.gz | tar zx)
	(cd protobuf; mv googletest-release-1.5.0 gtest)
	(cd protobuf; ./autogen.sh)
	(cd protobuf; $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) --with-protoc=$(shell which protoc) $(Xconfigure) )
	(cd protobuf; make )
	(cd protobuf; make install )
	

$(NETSNMP): $(NETSNMP_PKG)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfNETSNMP_PKG) \
		--without-rpm \
                --disable-manuals \
		--with-defaults \
		--with-default-snmp-version="2" \
		--disable-embedded-perl \
		--without-perl-modules)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); make)
	(cd $(NETSNMP)-$(NETSNMP_VER); $(Xenv); make install)

$(NETSNMP_PKG):
	wget http://sourceforge.net/projects/net-snmp/files/net-snmp/$(NETSNMP_VER)/$(NETSNMP)-$(NETSNMP_VER).tar.gz

$(OPENHPI): $(OPENHPI_PKG) $(GLIB) $(E2FSPROGS)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(OPENHPI)-$(OPENHPI_VER); $(foreach config, $(XconfCROSS_OPENHPI), \
		echo $(config) >> config.cache; \
	))
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); $(Xargs) LIBS="-lglib-2.0 -lgthread-2.0 -lrt" ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfOPENHPI_PKG))
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); make)
	(cd $(OPENHPI)-$(OPENHPI_VER); $(Xenv); make install)

$(OPENHPI_PKG):
	wget http://sourceforge.net/projects/openhpi/files/openhpi-stable/$(OPENHPI_VER)/$(OPENHPI)-$(OPENHPI_VER).tar.gz

$(OPENHPISUBAGENT): $(OPENHPISUBAGENT_PKG)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER); $(foreach config, $(XconfCROSS_OPENHPISUBAGENT), \
		echo $(config) >> config.cache; \
	))
	(cd $(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER); $(Xenv); $(Xargs) LDFLAGS="-L$(InstDir)/lib -L$(InstDir)/$(Xtarget)/lib -Xlinker -R -Xlinker $(InstDir)/lib -Xlinker -R -Xlinker $(InstDir)/$(Xtarget)/lib" ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfOPENHPISUBAGENT_PKG))
	(cd $(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER); $(Xenv); make)
	(cd $(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER); $(Xenv); make install)

$(OPENHPISUBAGENT_PKG):
	wget http://sourceforge.net/projects/openhpi/files/openhpi%20snmp%20subagent/$(OPENHPISUBAGENT_VER)/$(OPENHPISUBAGENT)-$(OPENHPISUBAGENT_VER).tar.gz

$(PyHPI): $(PyHPI_PKG) $(PYTHON)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(PyHPI)-$(PyHPI_VER); $(foreach config, $(XconfCROSS_PyHPI), \
		echo $(config) >> config.cache; \
	))
	(cd $(PyHPI)-$(PyHPI_VER); $(Xenv); $(Xargs) LDFLAGS="-L$(InstDir)/lib -L$(InstDir)/$(Xtarget)/lib -Xlinker -R -Xlinker $(InstDir)/lib -Xlinker -R -Xlinker $(InstDir)/$(Xtarget)/lib" python setup.py build)
	(cd $(PyHPI)-$(PyHPI_VER); $(Xenv); $(Xargs) LDFLAGS="-L$(InstDir)/lib -L$(InstDir)/$(Xtarget)/lib -Xlinker -R -Xlinker $(InstDir)/lib -Xlinker -R -Xlinker $(InstDir)/$(Xtarget)/lib" python setup.py install --prefix=$(InstDir))

$(PyHPI_PKG):
	wget http://sourceforge.net/projects/openhpi/files/$(PyHPI)/$(PyHPI_VER)/$(PyHPI)-$(PyHPI_VER).tar.gz

$(PYTHON): $(PYTHON_PKG)
	(rm -rf $@)
	(tar xvf $<  > /dev/null)
	(cd $(PYTHON)-$(PYTHON_VER); $(foreach config, $(XconfCROSS_PYTHON), \
		echo $(config) >> config.cache; \
	))
	(cd $(PYTHON)-$(PYTHON_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfPYTHON_PKG) --build=i686-pc-linux-gnu --disable-ipv6)
	(cd $(PYTHON)-$(PYTHON_VER); $(Xenv); $(Xargs) make CROSS_COMPILE_TARGET=yes)
	(cd $(PYTHON)-$(PYTHON_VER); $(Xenv); $(Xargs) make install)

$(PYTHON_PKG):
	wget http://www.python.org/ftp/python/$(PYTHON_VER)/$(PYTHON_PKG)

$(GLIB): $(GLIB_PKG) $(ZLIB) $(DBUS)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(GLIB)-$(GLIB_VER); $(foreach config, $(XconfCROSS_GLIB), \
		echo $(config) >> config.cache; \
	))
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfGLIB_PKG))
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); make)
	(cd $(GLIB)-$(GLIB_VER); $(Xenv); make install)

$(GLIB_PKG):
	wget http://ftp.gtk.org/pub/glib/2.28/$(GLIB)-$(GLIB_VER).tar.gz

$(E2FSPROGS): $(E2FSPROGS_PKG)
	(rm -rf $@)
	(tar xvfz $<  > /dev/null)
	(cd $(E2FSPROGS)-$(E2FSPROGS_VER); $(foreach config, $(XconfCROSS_E2FSPROGS), \
		echo $(config) >> config.cache; \
	))
	(cd $(E2FSPROGS)-$(E2FSPROGS_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfE2FSPROGS_PKG))
	(cd $(E2FSPROGS)-$(E2FSPROGS_VER); $(Xenv); make)
	(cd $(E2FSPROGS)-$(E2FSPROGS_VER); $(Xenv); make install)
	(cd $(E2FSPROGS)-$(E2FSPROGS_VER); $(Xenv); make install-libs)

$(E2FSPROGS_PKG):
	wget http://nchc.dl.sourceforge.net/project/e2fsprogs/e2fsprogs/v$(E2FSPROGS_VER)/$(E2FSPROGS)-$(E2FSPROGS_VER).tar.gz

$(ZLIB): $(ZLIB_PKG) 
	(rm -rf $@-$(ZLIB_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(XconfZLIB_PKG))
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); make)
	(cd $(ZLIB)-$(ZLIB_VER); $(Xenv); make install)

$(ZLIB_PKG):
	wget http://zlib.net/$(ZLIB)-$(ZLIB_VER).tar.gz

$(GDBM): $(GDBM_PKG)
	(rm -rf $@-$(GDBM_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(GDBM)-$(GDBM_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir))
	(cd $(GDBM)-$(GDBM_VER); $(Xenv); make)
	(cd $(GDBM)-$(GDBM_VER); $(Xenv); make install)

$(GDBM_PKG):
	wget ftp://ftp.gnu.org/gnu/gdbm/$(GDBM)-$(GDBM_VER).tar.gz

$(BERKELEY): $(BERKELEY_PKG) 
	(rm -rf $@-$(BERKELEY_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(BERKELEY)-$(BERKELEY_VER); $(Xenv); $(Xargs) ./dist/configure --prefix=$(InstDir))
	(cd $(BERKELEY)-$(BERKELEY_VER); $(Xenv); make)
	(cd $(BERKELEY)-$(BERKELEY_VER); $(Xenv); make install)

$(BERKELEY_PKG):
	wget http://download.oracle.com/berkeley-db/$(BERKELEY)-$(BERKELEY_VER).tar.gz

$(SQLITE): $(SQLITE_PKG) 
	(rm -rf $@-$(SQLITE_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(SQLITE)-$(SQLITE_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir))
	(cd $(SQLITE)-$(SQLITE_VER); $(Xenv); make)
	(cd $(SQLITE)-$(SQLITE_VER); $(Xenv); make install)

$(SQLITE_PKG):
	wget https://sqlite.org/2017/$(SQLITE)-$(SQLITE_VER).tar.gz

$(DBUS): $(DBUS_PKG) $(EXPAT)
	(rm -rf $@-$(DBUS_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(DBUS)-$(DBUS_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfDBUS_PKG))
	(cd $(DBUS)-$(DBUS_VER); $(Xenv); make)
	(cd $(DBUS)-$(DBUS_VER); $(Xenv); make install)

$(DBUS_PKG):
	wget http://dbus.freedesktop.org/releases/dbus/$(DBUS)-$(DBUS_VER).tar.gz

$(EXPAT): $(EXPAT_PKG)
	(rm -rf $@-$(EXPAT_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(EXPAT)-$(EXPAT_VER); $(Xenv); $(Xargs) ./configure --prefix=$(InstDir) $(Xconfigure) $(XconfEXPAT_PKG))
	(cd $(EXPAT)-$(EXPAT_VER); $(Xenv); make)
	(cd $(EXPAT)-$(EXPAT_VER); $(Xenv); make install)

$(EXPAT_PKG):
	wget http://nchc.dl.sourceforge.net/project/expat/expat/$(EXPAT_VER)/$(EXPAT)-$(EXPAT_VER).tar.gz

$(LIBXML2):$(LIBXML2_PKG)
	(rm -rf $@-$(LIBXML2_VER))
	(tar xvfz $<  > /dev/null)
        # without-python so crossbuilds don't have to build python
	(cd $(LIBXML2)-$(LIBXML2_VER); $(Xenv); $(Xargs) ./configure --without-python --with-debug --with-run-debug --with-mem-debug CFLAGS="-O0 -g" --prefix=$(InstDir) $(Xconfigure))
	(cd $(LIBXML2)-$(LIBXML2_VER); $(Xenv); make)
	(cd $(LIBXML2)-$(LIBXML2_VER); $(Xenv); make install)

$(LIBXML2_PKG):
	wget ftp://xmlsoft.org/libxml2/$(LIBXML2_PKG)

$(BOOST): $(BOOST_PKG)
	(rm -rf $@-$(BOOST_VER))
	(tar xvfz $<  > /dev/null)
	(cd $(BOOST)_$(subst .,_,$(BOOST_VER)); CC= CXX= CFLAGS= ./bootstrap.sh --with-python-version=$(PYTHON_VER) --prefix=$(InstDir) --without-icu) # This is a native tool so remove all the cross compilation env vars
ifneq ($(Xtarget),)
	-rm -f ~/user-config.jam
	echo "using gcc : cross : $(COMPILER) ;" > ~/user-config.jam
	(cd $(BOOST)_$(subst .,_,$(BOOST_VER)); $(Xenv); ./b2 $(BOOST_FLAVOR) --prefix=$(InstDir) toolset=gcc-cross install)
else
	(cd $(BOOST)_$(subst .,_,$(BOOST_VER)); $(Xenv); ./b2 --prefix=$(InstDir) install)
endif

$(BOOST_PKG):
	wget --no-check-certificate http://sourceforge.net/projects/boost/files/$(BOOST)/$(BOOST_VER)/$(BOOST_PKG)

pyyang: $(PYANG_PKG)
	(rm -rf $(PYANG))
	(tar xvfz $<  > /dev/null)
	(cd pyang; python setup.py install)
	chmod -R 757 $(SAFPLUS_TARGET)
#pyang:
#	git clone https://github.com/mbj4668/pyang pyang
#	(cd pyang; git reset --hard a6e51ba83f06829d3d26849bcb306f49f335267f)
