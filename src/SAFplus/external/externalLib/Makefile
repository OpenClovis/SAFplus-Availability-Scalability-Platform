
SAFPLUS_SDK_DIR ?=/opt/clovis/6.1
SAFPLUS_DIR ?=/code/git/SAFplus/src/SAFplus

all: libmwext.so

prebuildExternal:
	mkdir -p prebuildExternal


libmwext.so: prebuildExternal
	(cd prebuildExternal; $(SAFPLUS_DIR)/configure --with-sdk-dir=$(SAFPLUS_SDK_DIR))
	(cd prebuildExternal/asp/build/local; make V=1 O=0 CL_DEBUG=1 EXTRA_CFLAGS=-DNO_SAF)
	(cp -f prebuildExternal/target/x86_64/linux-3.5.7/lib/libmw.so libmw.so)
#	(cp -f prebuildExternal/target/x86_64/linux-3.5.7/lib/libmw.so libmwext.so)

