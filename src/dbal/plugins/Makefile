S7 := 1
SAFPLUS_DBAL_PLUGIN :=1
include ../../mk/preface.mk

Release all: $(LIB_DIR)/libclBerkeleyDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclSQLiteDB.so

$(LIB_DIR)/libclBerkeleyDB.so:
	make -C berkeley

$(LIB_DIR)/libclGDBM.so:
	make -C gdbm

$(LIB_DIR)/libclSQLiteDB.so:
	make -C sqlite

clean:
	make -C berkeley clean
	make -C gdbm clean
	make -C sqlite clean

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
