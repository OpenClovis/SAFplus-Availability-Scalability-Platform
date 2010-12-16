PRODUCT_NAME=openclovis-sdk
VERSION_TAG=5.0

EXPORT_DIR?=$(shell pwd)
BUILD_NUMBER?=private

ifdef SkipDoc
SKIPDOC:=echo
endif
ifdef SkipIde
SKIPIDE:=echo
endif

PKG_DIRNAME=$(PRODUCT_NAME)-$(VERSION_TAG)-$(BUILD_NUMBER)
PKG_GPL_DIRNAME=$(PRODUCT_NAME)-$(VERSION_TAG)-$(BUILD_NUMBER)-gpl
PKG_FILENAME=$(PKG_DIRNAME).tar.gz
PKG_GPL_FILENAME=$(PKG_GPL_DIRNAME).tar.gz
MD5_FILENAME=$(PKG_DIRNAME).md5
MD5_GPL_FILENAME=$(PKG_GPL_DIRNAME).md5

$(warning $(EXPORT_DIR) $(BUILD_NUMBER) $(EXPORT_DIR)/$(PKG_FILENAME))

PSP_PREFIX=$(PRODUCT_NAME)-$(VERSION_TAG)-psp-
PSP_SUFFIX=-$(BUILD_NUMBER)

.PHONY: pkg psp

all: pkg

pkg: $(EXPORT_DIR)/$(PKG_GPL_FILENAME) psp

$(EXPORT_DIR)/$(PKG_GPL_FILENAME):
	-if [ -f doc/Makefile ]; \
	    then $(SKIPDOC) make -C doc pkg; \
	fi
	if [ -f IDE/Makefile ]; \
	    then $(SKIPIDE) make -C IDE pkg; \
	fi
	if [ -f logViewer/Makefile ]; \
	    then make -C logViewer pkg; \
	fi
	echo "BUILD_NUMBER=$(PKG_GPL_DIRNAME)" > pkg/BUILD
	echo "BUILD_NUMBER=$(PKG_GPL_DIRNAME)" > pkg/src/ASP/BUILD
	mv pkg $(PKG_GPL_DIRNAME)
	tar --exclude=.svn -czvf $(EXPORT_DIR)/$(PKG_GPL_FILENAME) $(PKG_GPL_DIRNAME) && \
		(cd $(EXPORT_DIR); md5sum $(PKG_GPL_FILENAME) > $(MD5_GPL_FILENAME))
	cp -r $(PKG_GPL_DIRNAME) $(PKG_DIRNAME)
	cp tools/license/*.py .
	python applylicense.py $(PKG_DIRNAME)
	rm ./*.py*
	echo "BUILD_NUMBER=$(PKG_DIRNAME)" > $(PKG_DIRNAME)/BUILD
	echo "BUILD_NUMBER=$(PKG_DIRNAME)" > $(PKG_DIRNAME)/src/ASP/BUILD
	tar --exclude=.svn -czvf $(EXPORT_DIR)/$(PKG_FILENAME) $(PKG_DIRNAME) && \
		(cd $(EXPORT_DIR); md5sum $(PKG_FILENAME) > $(MD5_FILENAME))
	rm -rf $(PKG_DIRNAME)
	mv $(PKG_GPL_DIRNAME) pkg

psp:
	make -C psp PSP_PREFIX=$(PSP_PREFIX) \
		    PSP_SUFFIX=$(PSP_SUFFIX) \
		    BUILD_NUMBER=$(BUILD_NUMBER) \
		    EXPORT_DIR=$(EXPORT_DIR) \
		    pkg

