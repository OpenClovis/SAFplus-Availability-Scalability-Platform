# The following variables are normally passed from the top-level Makefile
# They are defined here for testing purposes
EXPORT_DIR?=.
BUILD_NUMBER?=private
PSP_PREFIX?=test-psp-
PSP_SUFFIX?=-testbuild
BUILD_NUMBER?=private

.PHONY: pkg

psp_list=$(shell find . -mindepth 1 -maxdepth 1 -type d -and -not -name '.svn' -exec basename \{\} \;)

$(warning psp_list: $(psp_list))

pkg:
	@echo "Creating PSP packages..."
	for pkg in $(psp_list); do \
	    echo "Creating PSP package [$$pkg]..."; \
	    pkgname=$(PSP_PREFIX)$$pkg$(PSP_SUFFIX); \
	    rm -fr $$pkgname; \
	    cp -a $$pkg $$pkgname; \
	    echo "BUILD_NUMBER=$(BUILD_NUMBER)" > $$pkgname/BUILD; \
	    tar --exclude=.svn -czvf $(EXPORT_DIR)/$$pkgname.tar.gz $$pkgname; \
	    rm -fr $$pkgname; \
	    (cd $(EXPORT_DIR); md5sum $$pkgname.tar.gz > $$pkgname.md5); \
	done
