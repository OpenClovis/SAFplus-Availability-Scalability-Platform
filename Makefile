S7 := 1
include ./src/mk/preface.mk

export PKG_NAME=safplus
export PKG_VER ?=7.0
export PKG_REL ?=1
TOP_DIR := $(CURDIR)
TAR_NAME := $(dir $(TOP_DIR))$(PKG_NAME)_$(PKG_VER).tar.gz
BUILD := $(TOP_DIR)/build
$(info VERSION is $(PKG_VER))
$(info RELEASE is $(PKG_REL))
$(info TOP DIR is $(TOP_DIR))

all: 
	$(info TBD)

prepare:
	echo  "This target prepares your environment for package generation.  You only need to run this once (as root)"
	apt-get install dh-make

archive: $(TAR_NAME)

$(TAR_NAME):
	$(info entering $(TOP_DIR))
	echo "Generating $(TAR_NAME) archive"; 
	tar cvzf $(TAR_NAME) -C $(TOP_DIR) *  --exclude=build --exclude=target --exclude=images --exclude=boost_1_55_0;
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

define prepare_env_deb
$(info Packing the $1 in DEBIAN)
$(eval PKG_NAME=$1)
$(eval PKG_DIR:=$(dir $(TOP_DIR))debbuild_$1)
$(info PKG_DIR is $(PKG_DIR))
rm -rf $(PKG_DIR)
mkdir -p $(PKG_DIR)
$(eval DEB_TOP_DIR=$(PKG_DIR)/$(PKG_NAME)-$(PKG_VER))
mkdir -p $(DEB_TOP_DIR)
$(eval DEBIAN_DIR:=$(DEB_TOP_DIR)/debian)
echo $(DEBIAN_DIR)
mkdir -p  $(DEBIAN_DIR)
cp -r $(TOP_DIR)/DEB/* $(DEBIAN_DIR)
if [ "$(shell uname -p)" = "x86_64" ]; \
then \
     sed -i '/Architecture:/c Package: $(PKG_NAME)\nArchitecture: amd64\nSection: $2' $(DEBIAN_DIR)/control; \
else \
     sed -i '/Architecture:/c Package: $(PKG_NAME)\nArchitecture: i386\nSection: $2' $(DEBIAN_DIR)/control; \
fi;
sed -i '/Source:/c Source: $(PKG_NAME)\nSection: $2' $(DEBIAN_DIR)/control
sed -i '/prefix:/c prefix=/opt/saflus/$(PKG_VER)/$3' $(DEBIAN_DIR)/postrm
sed -i '/prefix:/c export PREFIX?=/opt/safplus/$(PKG_VER)/$3\nexport PACKAGENAME?=$(PKG_NAME)' $(DEBIAN_DIR)/rules
sed -i '/Destdir:/c export DESTDIR?=$(DEBIAN_DIR)/$(PKG_NAME)\nexport LIBRARY_DIR=$(DEB_TOP_DIR)' $(DEBIAN_DIR)/rules
sed -i '/safplus:/c$(PKG_NAME) ($(subst .,-,$(PKG_VER))-$(PKG_REL)) stable; urgency=medium' $(DEBIAN_DIR)/changelog
endef

deb-src:archive
	$(call prepare_env_deb,safplus-src,devel,sdk)
	$(info Packing the $(PKG_NAME) in DEBIAN)
	tar xvzf $(TAR_NAME) -C $(DEB_TOP_DIR)
	#echo TBD: create source install package
	cd $(DEB_TOP_DIR) && dpkg-buildpackage -uc -us -b
	mkdir -p $(BUILD)
	cp $(PKG_DIR)/*.deb $(BUILD)


$(BIN_DIR)/safplus_amf:
	cd src && make 


deb-bin: $(BIN_DIR)/safplus_amf
	$(call prepare_env_deb,safplus,lib,sdk/target)
	cp -rf $(BIN_DIR) $(DEB_TOP_DIR)
	cp -rf $(PLUGIN_DIR) $(DEB_TOP_DIR)
	cp -rf $(LIB_DIR) $(DEB_TOP_DIR)
	mkdir -p $(DEB_TOP_DIR)/src
	#Some of the header files present in the src/include contains symbolic links.
	#We need to Copy those symlinks files as full files
	#Both safplus src and binary debian package contains the header files,dpkg throws an error while installing the safplus
	#debian package. Workaround is renameing the include directory to include1. Need to fix this problem.
	rsync -rL $(SAFPLUS_INC_DIR)/ $(DEB_TOP_DIR)/src/include1
	cp -rf $(TOP_DIR)/DEB/Makefile $(DEB_TOP_DIR)
	cd $(DEB_TOP_DIR) && dpkg-buildpackage -uc -us -b
	mkdir -p $(BUILD)
	cp $(PKG_DIR)/*.deb $(BUILD)

deb: deb-bin deb-src

#Due to inclusion of src/mk/preface.mk creates a src/target directory in the package.
#This rule removes src/target from the safplus src package 
remove_target:
	rm -rf $(PWD)/src/target
deb_install:remove_target
	$(eval REQ_FILES:=$(filter-out $(PWD)/debian/, $(wildcard $(PWD)/*/)))
	mkdir -p $(DESTDIR)/$(PREFIX)
	for value in $(REQ_FILES);do \
        cp -r $$value $(DESTDIR)/$(PREFIX); \
        done
	# Need to uncomment the below line to include git repository in the SAFPlus Debian package.
	#cp -r $(PWD)/.git $(DESTDIR)/$(PREFIX)

deb_clean:remove_target
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars

deb_build:remove_target
	echo "Not required"

clean:
	rm -rf $(TAR_NAME) $(BUILD)
