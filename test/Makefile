S7 := 1
SAFPLUS_LOG_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../src/mk/preface.mk


all: $(TEST_DIR)/testLog
	echo Nothing to do

test:
	$(TAE_DIR)/tae -S safplusTest.xml

testdebug:
	$(TAE_DIR)/tae -v -v -v -d -S safplusTest.xml



clean:
	rm -rf stamps* *.zip *.pyc log_*/*

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
