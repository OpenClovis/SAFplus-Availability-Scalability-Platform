
$(info PKG_DIRS = $(PKG_DIRS))
DIRECTORYS = $(wildcard $(PKG_DIRS)/*/)

$(info RPM_BUILD_DIR = $(RPM_BUILD_DIR))
$(info DIRECTORYS = $(DIRECTORYS))

install:
	echo $(ROOT)
	echo $(INSTALLDIR)
	mkdir -p $(ROOT)/$(INSTALLDIR)
	
	for dir in $(DIRECTORYS);do \
	cp -r $$dir $(ROOT)/$(INSTALLDIR);\
	done
	
