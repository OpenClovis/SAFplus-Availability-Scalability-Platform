S7 := 1
${safplusDir}
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

SUBDIRS = ${subdirs}

.PHONY: all $(SUBDIRS)
all: $(SUBDIRS)

${labelApps}

image: preMakeImage ${imagetgz}
preMakeImage:
ifneq ($(wildcard ../images),) 
	@echo "Directory exists.Deleting..."
	rm -r ../images
else
	@echo "Directory not exist, skip deleting..."
endif

${imageTgz}
	


clean:
${cleanupApps}

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk

