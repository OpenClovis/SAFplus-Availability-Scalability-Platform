S7 := 1
SAFPLUS_DIR=/home/thang/Documents/source7.0/SAFplus-Availability-Scalability-Platform/src/ide/../..
BASE_DIR=/home/thang/notificationTest
ifneq ($(wildcard $(SAFPLUS_DIR)/src/mk/preface.mk),) 
    SAFPLUS_MAKE_DIR ?= $(SAFPLUS_DIR)/src/mk/
else
ifneq ($(wildcard ../../../src/mk/preface.mk),)   # within source tree
    SAFPLUS_MAKE_DIR ?= ../../../src/mk/
else
ifneq ($(wildcard /opt/safplus/7.0/sdk),) 
    SAFPLUS_MAKE_DIR ?= /opt/safplus/7.0/sdk/src/mk
else
$(error Cannot find SAFPLUS SDK)
endif
endif
endif

include $(SAFPLUS_MAKE_DIR)/preface.mk

SUBDIRS = $(BASE_DIR)/target/bin/Component1

.PHONY: all $(SUBDIRS)
all: directories $(SUBDIRS)

directories:
	mkdir -p $(BASE_DIR)/target/bin


$(BASE_DIR)/target/bin/Component1:
	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C Component1


images: preMakeImage Node12.tar.gz Node11.tar.gz
preMakeImage:
ifneq ($(wildcard ../images),) 
	@echo "Directory exists.Deleting..."
	rm -rf ../images
else
	@echo "Directory not exist, skip deleting..."
endif

Node12.tar.gz: $(SAFPLUS_SRC_DIR)/mk/safplus_packager.py ${subdirs} $(wildcard $(PLUGIN_DIR)/*.so) $(wildcard $(BIN_DIR)/*) Makefile 
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -a x86_64-linux-gnu -x "(cp $(SAFPLUS_SRC_DIR)/ide/resources/setup ../*.xml {image_dir}/bin)" $@
Node11.tar.gz: $(SAFPLUS_SRC_DIR)/mk/safplus_packager.py ${subdirs} $(wildcard $(PLUGIN_DIR)/*.so) $(wildcard $(BIN_DIR)/*) Makefile 
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -a x86_64-linux-gnu -x "(cp $(SAFPLUS_SRC_DIR)/ide/resources/setup ../*.xml {image_dir}/bin)" $@
	


clean:
	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C Component1 clean 

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk

