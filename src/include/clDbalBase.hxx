#pragma once
#include <clDbalApi.h>
#include <clPluginApi.hxx>

#define NULL_CHECK(X) if(NULL == (X)) { \
        errorCode = CL_RC(CL_CID_DBAL, CL_ERR_NULL_POINTER); \
        /*clDbgCodeError(errorCode, ("Parameter " #X " is NULL!"));*/ \
                        return(errorCode); \
                      }

/* base class for all dbal plugins including sqlite, gdbm, berkeley and even checkpoint */
namespace SAFplus {

enum
 {
    CL_DBAL_PLUGIN_ID = 0x13746813,
    CL_DBAL_PLUGIN_VER = 1
 };

class DbalPlugin: public SAFplus::ClPlugin
{
public:

  const char* type;
 
  virtual ClRcT open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize)=0;  
  virtual ClRcT insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)=0;
  virtual ClRcT replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)=0;
  virtual ClRcT getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)=0;
  virtual ClRcT getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)=0;
  virtual ClRcT getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize)=0;
  virtual ClRcT deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize)=0;
  virtual ClRcT syncDbal(ClUint32T flags)=0;
  virtual ClRcT openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize)=0;
  virtual ClRcT beginTransaction()=0;
  virtual ClRcT commitTransaction()=0;
  virtual ClRcT abortTransaction()=0;
  virtual ClRcT freeRecord(ClDBRecordT dbRec)=0;
  virtual ClRcT freeKey(ClDBRecordT dbKey)=0;
  virtual ~DbalPlugin()=0;

  DbalPlugin(DbalPlugin const&) = delete; 
  DbalPlugin& operator=(DbalPlugin const&) = delete;
  
protected:
  ClDBHandleT pDBHandle;

  virtual ClRcT close()=0; /* destructor of derived class will call this function */
  DbalPlugin() {pDBHandle=NULL;}
};

DbalPlugin::~DbalPlugin() { }

/*
This function loads and creates a new dbal plugin object based on the pluginFile parameter.
The value of pluginFile is the one of the dbal plugin shared libraries (libclSqliteDB.so, libclGDBM.so, libclBerkeleyDB.so or libclCkptDB.so).
Therefore, user can decide which dbal plugin should be loaded and created on his own. 
If this parameter is omitted or set as NULL, the dbal plugin will be determined by the environment variable SAFPLUS_DB_PLUGIN. If environment variable SAFPLUS_DB_PLUGIN is not set, the dbal plugin will be the default one. 
 - Return value: DbalPlugin object pointer. User has to delete it when it's not used to avoid memory leak.
*/
extern DbalPlugin* clDbalObjCreate(const char* pluginFile=NULL);

};
/* specific database classes implement their own functionalities */
/*
class SqlitePlugin: public DbalPlugin
{
};

class GdbmPlugin: public DbalPlugin
{
};

class BerkeleyPlugin: public DbalPlugin
{
};

class CheckpointPlugin: public DbalPlugin
{
};
*/

namespace SAFplusI
{
  extern SAFplus::DbalPlugin* defaultDbalPlugin;
};
