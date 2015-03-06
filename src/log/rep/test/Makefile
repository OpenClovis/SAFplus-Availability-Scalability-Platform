S7 := 1
include ../../../../mk/preface.mk

LogWriter:=$(TEST_DIR)/logWriter
LogReader:=$(TEST_DIR)/logReader

all: $(LogWriter) $(LogReader)

$(TEST_DIR)/logWriter:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/rep/test/LogWriter

$(TEST_DIR)/logReader:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/rep/test/LogReader

clean:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/rep/test/LogWriter clean
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/rep/test/LogReader clean
