S7 := 1
SAFPLUS_LOG_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../src/mk/preface.mk


all: $(TEST_DIR)/testLog
	echo Nothing to do

test:
	@rm -f /var/log/testresults.log
	$(TAE_DIR)/tae -S safplusTest1Node.xml

testone:
	$(TAE_DIR)/tae -S safplusTest1Node.xml -M $(MODEL)

testonedebug:
	$(TAE_DIR)/tae -v -v -v -d -S safplusTest1Node.xml -M $(MODEL)

testdebug:
	$(TAE_DIR)/tae -v -v -v -d -S safplusTest1Node.xml

testyocto:
	$(TAE_DIR)/tae -S safplusTest1NodeYocto.xml

testyoctoone:
	$(TAE_DIR)/tae -S safplusTest1NodeYocto.xml -M $(MODEL)

testyoctodebug:
	$(TAE_DIR)/tae -v -v -v -d -S safplusTest1NodeYocto.xml

testyoctodebugone:
	$(TAE_DIR)/tae -v -v -v -d -S safplusTest1NodeYocto.xml -M $(MODEL)



clean:
	rm -rf *.tgz stamps* *.zip *.pyc log_*/* /var/log/testresults.log

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
