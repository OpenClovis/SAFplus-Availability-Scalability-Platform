#include <clDbalBase.hxx>
#include <string.h>
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clCommon.hxx>
#include <clLogIpi.hxx>
//#include <clDbg.hxx>
#include <gdbm.h>

namespace SAFplus {

/*****************************************************************************/
#define CL_GDBM_BLOCK_SIZE   4096     /* Black size for GDBM configuration */
#define CL_GDBM_FILE_MODE    0644     /* Permissions for the database */
#define CL_GDBM_MAX_DEL_CNT  1024     /* every 1024 deletions, reorganize the
                                         database 
                                       */
/*****************************************************************************/
typedef struct GDBMHandle_t {
  ClUint32T      deleteRecCnt;
  GDBM_FILE      gdbmInstance;      /* GDBM database instance handle */
  ClBoolT        syncMode;
} GDBMHandle_t;


class GdbmPlugin: public DbalPlugin
{
public:
  //GdbmPlugin() {}
  virtual ClRcT open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize);  
  virtual ClRcT insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize);
  virtual ClRcT replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize);
  virtual ClRcT getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize);
  virtual ClRcT getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize);
  virtual ClRcT getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize);
  virtual ClRcT deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize);
  virtual ClRcT syncDbal(ClUint32T flags);
  virtual ClRcT openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize);
  virtual ClRcT beginTransaction();
  virtual ClRcT commitTransaction();
  virtual ClRcT abortTransaction();
  virtual ClRcT freeRecord(ClDBRecordT dbRec);
  virtual ClRcT freeKey(ClDBRecordT dbKey);
  virtual ~GdbmPlugin();

protected:  
  virtual ClRcT close();
};

//static GdbmPlugin api;



GdbmPlugin::~GdbmPlugin()
{ 
  close(); 
}

ClRcT GdbmPlugin::open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize)
{
  if (pDBHandle) close();

  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = NULL;
  ClUint32T read_write = 0;
  ClDBTypeT  dbType = CL_DB_TYPE_BTREE;

  //NULL_CHECK(pDBHandle);
  NULL_CHECK(dbName);

  /* Validate the flag */
  if(dbFlag >= CL_DB_MAX_FLAG) {
    errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Flag");
    return(errorCode);
  }
  
  /* Validate database type */
  if(dbType >= CL_DB_MAX_TYPE) {
    errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB Type");
    
    return(errorCode);
  }

  pGDBMHandle = (GDBMHandle_t*)SAFplusHeapAlloc(sizeof(GDBMHandle_t));

  if(NULL == pGDBMHandle) {
    errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Memory allocation failed.");
    
    return(errorCode);
  }

  // Initalize the object that was just created
  pGDBMHandle->deleteRecCnt = 0;
  pGDBMHandle->gdbmInstance = 0;
  pGDBMHandle->syncMode = CL_FALSE;

  if(CL_DB_SYNC & dbFlag)
  {
      dbFlag &= ~(CL_DB_SYNC);
      pGDBMHandle->syncMode = CL_TRUE;
  }

  /* Let the env variable override the coded behaviour */
  if (getenv("ASP_DB_SYNC"))
  {
      if (SAFplus::clParseEnvBoolean("ASP_DB_SYNC"))
      {
          pGDBMHandle->syncMode = CL_TRUE;
      }
      else
      {
          pGDBMHandle->syncMode = CL_FALSE;
      }
  }

  if(CL_DB_CREAT == dbFlag) {
    read_write = GDBM_NEWDB | GDBM_NOLOCK;
  }
  else {
    pGDBMHandle->gdbmInstance = gdbm_open((ClCharT*)dbName, CL_GDBM_BLOCK_SIZE, GDBM_READER, CL_GDBM_FILE_MODE, NULL);
    if ((NULL == pGDBMHandle->gdbmInstance) && (CL_DB_APPEND != dbFlag)) {
        SAFplusHeapFree(pGDBMHandle);
        errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Open failed.");
        
        return(errorCode);
    }
    if (NULL != pGDBMHandle->gdbmInstance)   gdbm_close(pGDBMHandle->gdbmInstance);  
    if (CL_DB_OPEN == dbFlag)
        read_write = GDBM_WRITER;
    else if (CL_DB_APPEND == dbFlag)
        read_write = GDBM_WRCREAT;
    else
    {
        SAFplusHeapFree(pGDBMHandle);
        errorCode = CL_DBAL_RC(CL_ERR_BAD_FLAG);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nInvalid DB flag. GDBM open failed.");
        
        return(errorCode);
    }
  }

  if(CL_TRUE == pGDBMHandle->syncMode)
  {
      read_write |= GDBM_SYNC;
  }
  
  /* Create/Open the GDBM database */
  pGDBMHandle->gdbmInstance = gdbm_open((ClCharT*)dbName, CL_GDBM_BLOCK_SIZE, read_write, CL_GDBM_FILE_MODE, NULL);

  if(NULL == (pGDBMHandle->gdbmInstance)) {
    /* if the creation failed, return error */
    SAFplusHeapFree(pGDBMHandle);
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Open failed.");
    
    return(errorCode);
  }

  /* Return the handle to the created/opened database */  
  pDBHandle = (ClDBHandleT)pGDBMHandle;
  
  return(CL_OK);
}

ClRcT GdbmPlugin::close()
{
  if (!pDBHandle) return CL_OK;

  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  
  /* close the GDBM database */
  gdbm_close(pGDBMHandle->gdbmInstance);

  /* make the GDBM handle invalid */
  SAFplusHeapFree(pGDBMHandle);
  pDBHandle=NULL;

  logInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM closed.");
  
  return(CL_OK);  
}

ClRcT GdbmPlugin::insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  int returnCode = 0;
  datum key = {NULL, 0};
  datum data = {NULL, 0};
  
  
  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  data.dsize = recSize;
  data.dptr = (ClCharT *)dbRec;

  /* Store the key and record into the database */
  returnCode = gdbm_store(pGDBMHandle->gdbmInstance, key, data, GDBM_INSERT);

  if(1 == returnCode) {
    /* GDBM returned duplicate error, so return CL_ERR_DUPLICATE */
    errorCode = CL_DBAL_RC(CL_ERR_DUPLICATE);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nDuplicate key");
    
    return(errorCode);
  }

  if(0 != returnCode) {
    /* If not, some other GDBM error occured. return DB error */
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM ERROR");
    
    return(errorCode);
  }
  
  return(CL_OK);
}

ClRcT GdbmPlugin::replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  ClUint32T returnCode = 0;
  datum key = {NULL, 0};
  datum data = {NULL, 0};
  
  
  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  data.dsize = recSize;
  data.dptr = (ClCharT *)dbRec;

  /* Replace the record in the database */
  returnCode = gdbm_store(pGDBMHandle->gdbmInstance, key, data, GDBM_REPLACE);

  if(0 != returnCode) {
    /* Some GDBM error occured. return DB error */
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM replace failed");
    
    return(errorCode);
  }

  
  return(CL_OK);
}

ClRcT GdbmPlugin::getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  datum key = {NULL, 0};
  datum data = {NULL, 0};

  
  NULL_CHECK(pDBRec);

  NULL_CHECK(pRecSize);

  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  /* retrieve the record from the database */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, key);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM fetch failed");
    
    return(errorCode);
  }
  
  *pDBRec = (ClDBRecordT)data.dptr;
  *pRecSize = data.dsize;

  
  return(CL_OK);
}

ClRcT GdbmPlugin::getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  datum key = {NULL, 0};
  datum data = {NULL, 0};

  
  NULL_CHECK(pDBKey);
  NULL_CHECK(pKeySize);

  NULL_CHECK(pDBRec);
  NULL_CHECK(pRecSize);

  /* Retrieve the first key in the database */  
  key = gdbm_firstkey(pGDBMHandle->gdbmInstance);

  if(NULL == key.dptr) {
    /* The first key does exist. So return error */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM record get failed");
    
    return(errorCode);
  }

  *pDBKey = (ClDBKeyT)key.dptr;
  *pKeySize = key.dsize;

  /* Retrieve the associated record in the database */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, key);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM record fetch failed");
    
    return(errorCode);
  }
  
  *pDBRec = (ClDBRecordT)data.dptr;
  *pRecSize = data.dsize;

  
  return(CL_OK);
}

ClRcT GdbmPlugin::getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  datum key = {NULL, 0};
  datum nextKey = {NULL, 0};
  datum data = {NULL, 0};

  
  NULL_CHECK(pDBNextKey);
  NULL_CHECK(pNextKeySize);

  NULL_CHECK(pDBNextRec);
  NULL_CHECK(pNextRecSize);

  key.dsize = currentKeySize;
  key.dptr = (ClCharT *)currentKey;

  /* Retrieve the next key */
  nextKey = gdbm_nextkey(pGDBMHandle->gdbmInstance, key);

  if(NULL == nextKey.dptr) {
    /* The next key does not exist. So return error */
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM get next key failed");
    
    return(errorCode);
  }

  *pDBNextKey = (ClDBKeyT)nextKey.dptr;
  *pNextKeySize = nextKey.dsize;

  /* retrieve the associated record */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, nextKey);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM fetch record failed");
    
    return(errorCode);
  }
  
  *pDBNextRec = (ClDBRecordT)data.dptr;
  *pNextRecSize = data.dsize;

  
  return(CL_OK);
}

ClRcT GdbmPlugin::deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;
  ClInt32T returnCode = 0;
  datum key = {NULL, 0};

  
  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  returnCode = gdbm_delete(pGDBMHandle->gdbmInstance, key);

  if(0 != returnCode) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM delete failed");
    
    return(errorCode);
  }
  pGDBMHandle->deleteRecCnt++;

  if( CL_GDBM_MAX_DEL_CNT <= pGDBMHandle->deleteRecCnt )
  {
      returnCode = gdbm_reorganize(pGDBMHandle->gdbmInstance);
      if(0 != returnCode) {
          errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
          logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM reorganize failed");
          
          return(errorCode);
      }
      pGDBMHandle->deleteRecCnt = 0;
  }
  
  return(CL_OK);
}

ClRcT GdbmPlugin::syncDbal(ClUint32T flags)
{
   GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)pDBHandle;

  
  /**
   * Calling gdbm_sync() only if GDBM DB is not opened in 
   * automatic SYNC mode.
   */
  if(CL_FALSE == pGDBMHandle->syncMode)
  {
      /**
       * Explicit DB sync - database will be completely updated 
       * with all changes to the current time 
       */
      gdbm_sync(pGDBMHandle->gdbmInstance);
  }

  
  return(CL_OK);
}

ClRcT GdbmPlugin::openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize)
{
  ClRcT errorCode = CL_OK;
  
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  
  return(errorCode);
}

ClRcT GdbmPlugin::beginTransaction()
{
  ClRcT errorCode = CL_OK;
  
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  
  return(errorCode);
}

ClRcT GdbmPlugin::commitTransaction()
{
  ClRcT errorCode = CL_OK;
  
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  
  return(errorCode);
}

ClRcT GdbmPlugin::abortTransaction()
{
  ClRcT errorCode = CL_OK;
  
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  
  return(errorCode);
}

ClRcT GdbmPlugin::freeRecord(ClDBRecordT dbRec)
{
  ClRcT errorCode = CL_OK;

  NULL_CHECK(dbRec);
  SAFplusHeapFree(dbRec);
  return(CL_OK);
}

ClRcT GdbmPlugin::freeKey(ClDBRecordT dbKey)
{
  ClRcT errorCode = CL_OK;
  NULL_CHECK(dbKey);
  SAFplusHeapFree(dbKey);
  return(CL_OK);
}

};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::GdbmPlugin* api = new SAFplus::GdbmPlugin();
  api->pluginId         = SAFplus::CL_DBAL_PLUGIN_ID;
  api->pluginVersion    = SAFplus::CL_DBAL_PLUGIN_VER;
  api->type = "Gdbm";

  // return it
  return (SAFplus::ClPlugin*) api;
}
