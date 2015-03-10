S7 := 1
SAFPLUS_DBAL_PLUGIN :=1
include ../../mk/preface.mk

Release all: $(PLUGIN_DIR)/libclBerkeleyDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclSQLiteDB.so

$(PLUGIN_DIR)/libclBerkeleyDB.so:
	make -C berkeley

$(PLUGIN_DIR)/libclGDBM.so:
	make -C gdbm

$(PLUGIN_DIR)/libclSQLiteDB.so:
	make -C sqlite

clean:
	make -C berkeley clean
	make -C gdbm clean
	make -C sqlite clean

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
