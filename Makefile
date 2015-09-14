S7 := 1
include ./mk/preface.mk


export PKG_NAME=safplus
export PKG_VER ?=7.0
export PKG_REL ?=1
TOP_DIR := $(CURDIR)
TAR_NAME := $(dir $(TOP_DIR))$(PKG_NAME)_$(PKG_VER).tar.gz
BUILD := $(TOP_DIR)/build
$(info PACKAGE NAME is $(PKG_NAME))
$(info VERSION is $(PKG_VER))
$(info RELEASE is $(PKG_REL))
$(info TOP DIR is $(TOP_DIR))

all: rpm deb

prepare:
	@echo  "This target prepares your environment for package generation.  You only need to run this once (as root)"
	apt-get install dh-make

archive: $(TAR_NAME)

$(TAR_NAME):
	$(info entering $(TOP_DIR))
	echo "Generating $(TAR_NAME) archive"; \
	tar cvzf $(TAR_NAME) -C $(TOP_DIR) . --exclude=build --exclude=target --exclude=images --exclude=boost_1_55_0;\
	mkdir -p $(BUILD)

rpm: archive
	$(info Packing the $(PKG_NAME) in RPM)
	$(eval PKG_DIR:=$(dir $(TOP_DIR))rpmbuild) 
	$(info PKG_DIR is $(PKG_DIR))
	rm -rf $(PKG_DIR)
	mkdir -p $(PKG_DIR)
	$(eval BUILD_DIR:=$(PKG_DIR)/BUILD)
	$(eval SRC_DIR:=$(PKG_DIR)/SOURCES)
	$(eval SPECS_DIR:=$(PKG_DIR)/SPECS)
	$(eval SRPMS_DIR:=$(PKG_DIR)/SRPMS)
	$(eval RPMS_DIR:=$(PKG_DIR)/RPMS)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(SRC_DIR)
	mkdir -p $(SPECS_DIR)
	mkdir -p $(SRPMS_DIR)
	mkdir -p $(RPMS_DIR)
	cp -r $(TOP_DIR)/SPECS/* $(SPECS_DIR)
	cp $(TAR_NAME) $(SRC_DIR)
	sed -i '/Version:/c Version:\t$(PKG_VER)' $(SPECS_DIR)/$(PKG_NAME).spec
	sed -i '/Release:/c Release:\t$(PKG_REL)%{dist}' $(SPECS_DIR)/$(PKG_NAME).spec
	cd $(PKG_DIR)
	rpmbuild  --define '_topdir $(PKG_DIR)' -ba $(SPECS_DIR)/$(PKG_NAME).spec
	cp $(RPMS_DIR)/$(shell uname -p)/*.rpm $(BUILD)
	cp $(SRPMS_DIR)/*.rpm $(BUILD)
	rm -rf $(PKG_DIR)

deb-src:
	@echo TBD: create source install package


$(BIN_DIR)/safplus_amf: 
	cd src && make USE_DIST_LIB=1


deb: $(BIN_DIR)/safplus_amf
	$(info Packing the $(PKG_NAME) in DEBIAN)
	$(info from the archive $(TAR_NAME))
	$(eval PKG_DIR:=$(dir $(TOP_DIR))debbuild)
	$(info PKG_DIR is $(PKG_DIR))

	rm -rf $(PKG_DIR)
	mkdir -p $(PKG_DIR)
	#cp $(TAR_NAME) $(PKG_DIR)
	$(eval DEB_TOP_DIR=$(PKG_DIR)/$(PKG_NAME)-$(PKG_VER))
	mkdir -p $(DEB_TOP_DIR)
	#tar xvzf $(TAR_NAME) -C $(DEB_TOP_DIR)
        touch $(TAR_NAME)
        cp -rf $(BIN_DIR) $(PKG_DIR)/<whatever>
        cp -rf $(PLUGIN_DIR) $(PKG_DIR)/<whatever>
        cp -rf $(LIB_DIR) $(PKG_DIR)/<whatever>
        cp -rf $(INC_DIR) $(PKG_DIR)/<whatever>  # Copy symlinks as full files

	$(eval DEBIAN_DIR:=$(DEB_TOP_DIR)/debian)
	echo $(DEBIAN_DIR)
	mkdir -p  $(DEBIAN_DIR)
	cp -r $(TOP_DIR)/DEB/* $(DEBIAN_DIR)
	if [ "$(shell uname -p)" = "x86_64" ]; \
	then \
	     sed -i '/Architecture:/c Architecture: amd64' $(DEBIAN_DIR)/control; \
	else \
	     sed -i '/Architecture:/c Architecture: i386' $(DEBIAN_DIR)/control; \
	fi;
	sed -i '/prefix:/c prefix=/opt/$(PKG_NAME)/$(PKG_VER)/sdk' $(DEBIAN_DIR)/postrm
	sed -i '/prefix:/c export PREFIX?=/opt/$(PKG_NAME)/$(PKG_VER)/sdk\nexport PACKAGENAME?=$(PKG_NAME)' $(DEBIAN_DIR)/rules
	sed -i '/Destdir:/c export DESTDIR?=$(DEBIAN_DIR)/$(PKG_NAME)\nexport LIBRARY_DIR=$(DEB_TOP_DIR)/src/target/$(shell uname -p)/$(shell uname -r)' $(DEBIAN_DIR)/rules
	sed -i '/$(PKG_NAME):/c$(PKG_NAME) ($(subst .,-,$(PKG_VER))-$(PKG_REL)) stable; urgency=medium' $(DEBIAN_DIR)/changelog
	cd $(DEB_TOP_DIR) && dpkg-buildpackage -uc -us -b
	cp $(PKG_DIR)/*.deb $(BUILD)
	# rm -rf $(PKG_DIR)
deb_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))
	mkdir -p $(DESTDIR)/$(PREFIX)
	for value in $(REQ_FILES);do \
        cp -r $$value $(DESTDIR)/$(PREFIX); \
        done
	cp -r $(PWD)/.git $(DESTDIR)/$(PREFIX)
deb_clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars
deb_build:
	cd src && make USE_DIST_LIB=1

clean:
	rm -rf $(TAR_NAME) $(BUILD)
