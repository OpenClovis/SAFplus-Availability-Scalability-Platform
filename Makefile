export PKG_NAME=safplus
export PKG_VER ?= 7.0
export PKG_REL ?= 1
TOP_DIR := $(CURDIR)
TAR_NAME := $(dir $(TOP_DIR))$(PKG_NAME)_$(PKG_VER).tgz
BUILD := $(TOP_DIR)/build
$(info PACKAGE NAME is $(PKG_NAME))
$(info VERSION is $(PKG_VER))
$(info RELEASE is $(PKG_REL))
$(info TOP DIR is $(TOP_DIR))
gen_archive:
	$(info entering $(TOP_DIR))
	if [ ! -f $(TAR_NAME) ]; \
	then \
	     echo "Genearting $(TAR_NAME) archive"; \
	     tar cvzf $(TAR_NAME) -C $(TOP_DIR) . --exclude=build ;\
	fi;
	mkdir -p $(BUILD)
rpm: gen_archive
	$(info Packing the $(PKG_NAME) in RPM)
	$(eval PKG_DIR:=$(dir $(TOP_DIR))rpmbuild) 
	$(info PKG_DIR is $(PKG_DIR))
	rm -rf $(PKG_DIR)
	mkdir -p $(PKG_DIR)
	mkdir -p $(PKG_DIR)/{BUILD,SOURCES,SPECS,SRPMS,RPMS}
	$(eval BUILD_DIR:=$(PKG_DIR)/BUILD)
	$(eval SRC_DIR:=$(PKG_DIR)/SOURCES)
	$(eval SPECS_DIR:=$(PKG_DIR)/SPECS)
	$(eval SRPMS_DIR:=$(PKG_DIR)/SRPMS)
	$(eval RPMS_DIR:=$(PKG_DIR)/RPMS)
	cp -r $(TOP_DIR)/SPECS/* $(SPECS_DIR)
	cp $(TAR_NAME) $(SRC_DIR)
	sed -i '/Version:/c Version:\t$(PKG_VER)' $(SPECS_DIR)/$(PKG_NAME).spec
	sed -i '/Release:/c Release:\t$(PKG_REL)%{dist}' $(SPECS_DIR)/$(PKG_NAME).spec
	cd $(PKG_DIR)
	rpmbuild  --define '_topdir $(PKG_DIR)' -ba $(SPECS_DIR)/$(PKG_NAME).spec
	cp $(RPMS_DIR)/$(shell uname -p)/*.rpm $(BUILD)
	cp $(SRPMS_DIR)/*.rpm $(BUILD)
	rm -r $(PKG_DIR)
deb: gen_archive
	$(info Packing the $(PKG_NAME) in DEBIAN)
	$(eval PKG_DIR:=$(dir $(TOP_DIR))debbuild)
	$(info PKG_DIR is $(PKG_DIR))
	rm -rf $(PKG_DIR)
	mkdir -p $(PKG_DIR)
	cp $(TAR_NAME) $(PKG_DIR)
	$(eval DEB_TOP_DIR=$(PKG_DIR)/$(PKG_NAME)-$(PKG_VER))
	mkdir -p $(DEB_TOP_DIR)
	tar xvzf $(TAR_NAME) -C $(DEB_TOP_DIR)
	$(eval DEBIAN_DIR:=$(DEB_TOP_DIR)/debian)
	echo $(DEBIAN_DIR)
	mkdir -p  $(DEBIAN_DIR)
	cp -r $(TOP_DIR)/DEB/* $(DEBIAN_DIR)
	if [ "$(shell uname -p)" == "x86_64" ]; \
	then \
	     sed -i '/Architecture:/c Architecture: amd64' $(DEBIAN_DIR)/control; \
	else \
	     sed -i '/Architecture:/c Architecture: i386' $(DEBIAN_DIR)/control; \
	fi;
	sed -i '/prefix:/c prefix=/opt/$(PKG_NAME)/$(PKG_VER)/sdk' $(DEBIAN_DIR)/postrm
	sed -i '/prefix:/c export PREFIX ?= /opt/$(PKG_NAME)/$(PKG_VER)/sdk\nexport PACKAGENAME ?=$(PKG_NAME)\nexport DESTDIR ?= $(DEBIAN_DIR)/$(PKG_NAME)' $(DEBIAN_DIR)/rules
	cd $(DEB_TOP_DIR)
deb_install:
	$(eval REQ_FILES:=$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))
	for value in $(REQ_FILES);do \
        cp -r $$value $(DESTDIR)/$(PREFIX); \
        done
deb_clean:
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars
all:
	echo "Packaging the SAFplus in DEBIAN and RPM"

clean:
	rm -rf $(TAR_NAME) $(BUILD)
