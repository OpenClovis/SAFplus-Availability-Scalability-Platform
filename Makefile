S7 := 1
include ./src/mk/preface.mk
-include /etc/lsb-release

GIT_REV := $(shell git rev-parse --short=8 HEAD)

export PKG_NAME=safplus
export PKG_VER ?=7.0
export PKG_REL ?=$(GIT_REV)
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
	apt-get install dh-make reprepro

archive: $(TAR_NAME)

$(TAR_NAME):
	$(info entering $(TOP_DIR))
	echo "Generating $(TAR_NAME) archive"; 
	tar cvzf $(TAR_NAME) -C $(TOP_DIR) *  --exclude=build --exclude=target --exclude=images --exclude=boost_1_55_0;
	mkdir -p $(BUILD)

define prepare_env_rpm
$(eval PKG_NAME=$1)
$(info Packing the $(PKG_NAME) in RPM)
$(eval PKG_DIR:=$(dir $(TOP_DIR))rpmbuild_$(PKG_NAME))
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
sed -i '/Name:/c Name:\t\t$(PKG_NAME)\nVersion:\t$(PKG_VER)\nRelease:\t$(PKG_REL)%{dist}' $(SPECS_DIR)/safplus.spec
sed -i '/Prefix:/c Prefix:\t/opt/safplus/$(PKG_VER)/$2' $(SPECS_DIR)/safplus.spec
cp -rf $(SAFPLUS_TOP_DIR)/bin/*	$(BUILD_DIR)/IDE
endef

define copy_binpkg_files
$(eval DEST_PKG_DIR=$1)
cp -rf $(BIN_DIR) $(DEST_PKG_DIR)
cp -rf $(PLUGIN_DIR) $(DEST_PKG_DIR)
cp -rf $(LIB_DIR) $(DEST_PKG_DIR)
mkdir -p $(DEST_PKG_DIR)/src
mkdir -p $(DEST_PKG_DIR)/IDE
#Some of the header files present in the src/include contains symbolic links.
#Copy those symlinks files as full files
rsync -rL $(SAFPLUS_INC_DIR) $(DEST_PKG_DIR)/src
rsync -rL $(SAFPLUS_MAKE_DIR) $(DEST_PKG_DIR)/src
cp -rf $(TOP_DIR)/DEB/Makefile $(DEST_PKG_DIR)
endef

rpm-src: archive
	$(call prepare_env_rpm,safplus-src,sdk)
	tar xvzf $(TAR_NAME) -C $(BUILD_DIR)
	sed -i '/%install/aexport PREFIX=%prefix\nexport DESTDIR=$$RPM_BUILD_ROOT\nmake rpm_install' $(SPECS_DIR)/safplus.spec
	sed -i '/%defattr/a /%prefix/*' $(SPECS_DIR)/safplus.spec
	rpmbuild  --define '_topdir $(PKG_DIR)' -bb $(SPECS_DIR)/safplus.spec
	mkdir -p $(BUILD)
	cp $(RPMS_DIR)/$(shell uname -p)/*.rpm $(BUILD)

rpm-bin: build_binary
	$(call prepare_env_rpm,safplus,sdk/target/$(__TMP_TARGET_PLATFORM))
	$(call copy_binpkg_files,$(BUILD_DIR))
	sed -i '/%install/aexport PREFIX=%prefix\nexport DESTDIR=$$RPM_BUILD_ROOT\nmake rpm_install' $(SPECS_DIR)/safplus.spec
	sed -i '/%defattr/a /%prefix/*\n/%prefix/../../src/*\n%prefix/../../../ide/*' $(SPECS_DIR)/safplus.spec
	rpmbuild  --define '_topdir $(PKG_DIR)' -bb $(SPECS_DIR)/safplus.spec
	mkdir -p $(BUILD)
	cp $(RPMS_DIR)/$(shell uname -p)/*.rpm $(BUILD)

rpm: rpm-bin rpm-src

define prepare_env_deb
$(eval PKG_NAME=$1)
$(info Packing the $(PKG_NAME) in DEBIAN)
$(eval PKG_DIR:=$(dir $(TOP_DIR))debbuild_$(PKG_NAME))
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
sed -i '/prefix:/c PKG_NAME=$(PKG_NAME)\nprefix=/opt/safplus/$(PKG_VER)/$4\nIDE_DIR=/opt/safplus/$(PKG_VER)/ide' $(DEBIAN_DIR)/postrm
sed -i '/IDE_DIR:=/c IDE_DIR=/opt/safplus/$(PKG_VER)/ide' $(DEBIAN_DIR)/postinst
sed -i '/PKG_NAME:=/c PKG_NAME=$(PKG_NAME)\nPREFIX_DIR=/opt/safplus/$(PKG_VER)/$3/../../' $(DEBIAN_DIR)/prerm
sed -i '/prefix:/c export PREFIX?=/opt/safplus/$(PKG_VER)/$3\nexport PACKAGENAME?=$(PKG_NAME)' $(DEBIAN_DIR)/rules
sed -i '/Destdir:/c export DESTDIR?=$(DEBIAN_DIR)/$(PKG_NAME)\nexport LIBRARY_DIR=$(DEB_TOP_DIR)' $(DEBIAN_DIR)/rules
sed -i '/safplus:/c$(PKG_NAME) ($(subst .,-,$(PKG_VER))-$(PKG_REL)) stable; urgency=medium' $(DEBIAN_DIR)/changelog
mkdir -p $(DEB_TOP_DIR)/IDE
cp -rf $(SAFPLUS_TOP_DIR)/bin/*	$(DEB_TOP_DIR)/IDE
endef

deb-src:ide_build_clean archive ide_build
	$(call prepare_env_deb,safplus-src,devel,sdk,sdk)
	tar xvzf $(TAR_NAME) -C $(DEB_TOP_DIR)
	sed -i '/Architecture:/aReplaces: safplus' $(DEBIAN_DIR)/control
	cd $(DEB_TOP_DIR) && dpkg-buildpackage -uc -us -b
	mkdir -p $(BUILD)
	cp $(PKG_DIR)/*.deb $(BUILD)

ide_build_clean:
	cd src/ide && make clean
	rm -rf $(SAFPLUS_TOP_DIR)/bin

ide_build:
	cd src/ide && make

build_binary:
	cd src && make 


deb-bin: ide_build_clean build_binary ide_build
	$(call prepare_env_deb,safplus,lib,sdk/target/$(__TMP_TARGET_PLATFORM),sdk)
	$(call copy_binpkg_files, $(DEB_TOP_DIR))
	cd $(DEB_TOP_DIR) && dpkg-buildpackage -uc -us -b
	mkdir -p $(BUILD)
	cp $(PKG_DIR)/*.deb $(BUILD)

deb: deb-bin deb-src

#Due to inclusion of src/mk/preface.mk creates a src/target directory in the package.
#This rule removes src/target from the safplus src package 
remove_target:
	rm -rf $(PWD)/src/target

define safplus_pkg_install
mkdir -p $(DESTDIR)/$(PREFIX)
for value in $1;do \
cp -r $$value $(DESTDIR)/$(PREFIX); \
done
# Need to uncomment the below line to include git repository in the SAFPlus Debian package.
#cp -r $(PWD)/.git $(DESTDIR)/$(PREFIX)
mkdir -p $(DESTDIR)/$(PREFIX)/../ide
cp -r $(PWD)/IDE/* $(DESTDIR)/$(PREFIX)/../ide
endef

deb_install:remove_target
	$(eval REQ_FILES:=$(filter-out $(PWD)/debian/, $(filter-out $(PWD)/IDE/, $(filter-out $(PWD)/bin/, $(wildcard $(PWD)/*/)))))
	$(call safplus_pkg_install,$(REQ_FILES))

deb_clean:remove_target
	rm -rf debian/$(PACKAGENAME).*debhelper
	rm -rf debian/$(PACKAGENAME).substvars

deb_build:remove_target
	echo "Not required"

apt/debian/conf/distributions:
	mkdir -p apt/debian/conf
	python src/mk/genDebDist.py apt/debian/conf safplus $(DISTRIB_CODENAME) $(__TMP_TARGET_PLATFORM)

deb_upload: apt/debian/conf/distributions
	(cd apt/debian; reprepro -v includedeb $(DISTRIB_CODENAME) $(SAFPLUS_TOP_DIR)/build/*.deb)

rpm_install:remove_target
	$(eval REQ_FILES:=$(wildcard $(PWD)/*/))
	$(info $(REQ_FILES))
	$(call safplus_pkg_install,$(REQ_FILES))

clean:
	rm -rf $(TAR_NAME) $(BUILD) apt ../debbuild_safplus ../debbuild_safplus-src ../safplus_7.0.tar.gz ../rpmbuild_safplus ../rpmbuild_safplus-src
