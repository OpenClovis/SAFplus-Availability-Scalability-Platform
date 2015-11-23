#include <clDbalBase.hxx>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <cltypes.h>
#include <db.h>
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include "clClistApi.hxx"
#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clDbg.hxx>

using namespace SAFplusI;

namespace SAFplus {

#define CL_DB_SYNC_OVERRIDE 2

#define DATABASE_ID         0x2411
#define DATABASE_ENGINE_ID  0x1124

/*****************************************************************************/
#define VALIDITY_CHECK(X) if(DATABASE_ID != (X)) { \
                            errorCode = CL_RC(CL_CID_DBAL, CL_ERR_INVALID_HANDLE); \
                            clDbgCodeError(errorCode, ("DATABASE_ID Validity check failed"));\
                            return(errorCode); \
                          }
/*****************************************************************************/
#define DBAL_HDL_CHECK(dbHandle) \
    do{\
        if(NULL == (dbHandle))\
        { \
            errorCode = CL_RC(CL_CID_DBAL, CL_ERR_INVALID_HANDLE); \
            clDbgCodeError(errorCode, ("Passed DB Handle is invalid (NULL)"));\
            return(errorCode); \
        }\
    }while(0)
/*****************************************************************************/

/*****************************************************************************/
typedef struct BerkeleyDBHandle_t {
    DB_TXN* pCurrentTxn;    
    DB* pDatabase;
    DBC *pCursor;
    ClClistT transactionStackHandle;
    ClUint32T bdbFlags;
}BerkeleyDBHandle_t;
/*****************************************************************************/
typedef struct BerkeleyDBEnvHandle_t {
    DB_ENV* pDBEnv;
    ClUint32T validity;
    ClUint32T isInitialized;
}BerkeleyDBEnvHandle_t;
/*****************************************************************************/

typedef struct DBEngineInfo_t {
    ClUint32T  dbEngineType;
    ClDBEngineT engineHandle;
}DBEngineInfo_t;

class BerkeleyPlugin: public DbalPlugin
{
public:
  BerkeleyPlugin();
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
  virtual ~BerkeleyPlugin();

protected:  
  BerkeleyDBEnvHandle_t dbEnvironment;
  DBEngineInfo_t dbEngineInfo;
  ClRcT cdbBerkeleyDBInitialize();
  ClRcT cdbBerkeleyDBFinalize();

  ClRcT berkeleyDBSync(ClUint32T flags);
  virtual ClRcT close();
};

//static BerkeleyPlugin api;


#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 5 )

static void
cdbBerkeleyLsnReset(DB_ENV *env,
                    const ClCharT *dbFile)
{
    ClInt32T ret;
    struct stat statbuf;
    ret = stat(dbFile, &statbuf);
    if ((ret == -1) && (errno == ENOENT))
        goto failure;
    else if (ret == -1)
    {
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                       "Accessing [%s] failed, error [%s]",
                        dbFile,
                        strerror(errno));
        goto failure;
    }

    env->lsn_reset(env, dbFile, 0);

failure:
    return;
}

#else

#define cdbBerkeleyLsnReset(env, dbFile) do {;}while(0)

#endif

static void myDummyDeleteCallBack(ClClistDataT userData)
{
  return;
}

BerkeleyPlugin::~BerkeleyPlugin()
{ 
  close();   
  cdbBerkeleyDBFinalize();  
}

BerkeleyPlugin::BerkeleyPlugin()
{
  cdbBerkeleyDBInitialize();  
}

ClRcT BerkeleyPlugin::cdbBerkeleyDBInitialize()
{
    ClRcT errorCode = CL_OK;
    ClUint32T rc = 0;

    dbEnvironment.isInitialized = 0;

    rc = db_env_create(&(dbEnvironment.pDBEnv), 0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);
    }
    dbEnvironment.pDBEnv->set_errpfx(dbEnvironment.pDBEnv,"DBAL");
    dbEnvironment.pDBEnv->set_errfile(dbEnvironment.pDBEnv,stderr);

    rc = (dbEnvironment.pDBEnv)->open(dbEnvironment.pDBEnv, NULL, DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_THREAD, 0);

    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        goto error_out;
    }

#if 0 
   /* Stone: the DB_DSYNC operation seems to do a write and sync in a single 
      system call, if available, as opposed to a separate flush system call.
      This functionality adjusts precisely when a sync happens, not whether
      or not it happens at all!

      From the docs:
B_DSYNC_DB
    Configure Berkeley DB to flush database writes to the backing disk before returning from the write system call, rather than flushing database writes explicitly in a separate system call, as necessary.
   */
    ClCharT *syncEnvVar = NULL;
    gDBEnvironment.syncMode = CL_FALSE;

    syncEnvVar = getenv("ASP_DB_SYNC");
    if(syncEnvVar)
      {
        if (0 == strcasecmp(syncEnvVar, "TRUE"))
          {
            rc = (gDBEnvironment.pDBEnv)->set_flags(gDBEnvironment.pDBEnv, 
                                                DB_DSYNC_DB | DB_DSYNC_LOG, 1);
            if(0 != rc) 
              {
                errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
                goto error_out;
              }
            gDBEnvironment.syncMode = CL_TRUE;
          }
        else if (0 == strcasecmp(syncEnvVar, "FALSE"))
          {
            /* Nothing to do */
          }
        else
          {
            logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid value [%s] for environment variable [ASP_DB_SYNC].  Expecting 'TRUE' or 'FALSE'.",syncEnvVar);
          }
      }
#endif

    dbEnvironment.validity = DATABASE_ENGINE_ID;
    dbEnvironment.isInitialized = 1;

    //*pEngineHandle = (ClDBEngineT)&gDBEnvironment;
    dbEngineInfo.engineHandle = (ClDBEngineT)&dbEnvironment;
    dbEngineInfo.dbEngineType = 0;
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL initialize : DONE ");
    
    return (CL_OK);

error_out:
    (dbEnvironment.pDBEnv)->err(dbEnvironment.pDBEnv, rc, "environment open");
    (dbEnvironment.pDBEnv)->close(dbEnvironment.pDBEnv, 0);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL initialize : NOT DONE");
    
    return(errorCode);
}

ClRcT BerkeleyPlugin::cdbBerkeleyDBFinalize()
{
    ClRcT errorCode = CL_OK;
    ClUint32T rc = 0;
    

    if(dbEnvironment.isInitialized == 0) 
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB not initialized");
        
        return(errorCode);
    }
   
    rc = (dbEnvironment.pDBEnv)->close(dbEnvironment.pDBEnv, 0);
    if(0 != rc) 
    {
         errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
         logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley Shutdown failed.");
       
         return(errorCode);
    }
   

    dbEnvironment.isInitialized = 0;
    dbEnvironment.validity = 0;
    
    
    return(CL_OK);
}

ClRcT BerkeleyPlugin::open(ClDBFileT dbFile, ClDBNameT dbName, ClDBFlagT dbFlag, ClUint32T maxKeySize, ClUint32T maxRecordSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBTYPE berkeleyDBType = DB_BTREE;

    
    if(dbEnvironment.isInitialized == 0) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB NOT initialized.");
        
        return(errorCode);
    }


    if(dbFlag >= CL_DB_MAX_FLAG) {
        errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: Invalid flag.");
        
        return(errorCode);
    }

    pBerkeleyHandle = (BerkeleyDBHandle_t*) pDBHandle;
    if (!pBerkeleyHandle) 
    {
        pBerkeleyHandle = (BerkeleyDBHandle_t *)SAFplusHeapAlloc(sizeof(BerkeleyDBHandle_t));
        if(NULL == pBerkeleyHandle) {
            errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
            logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: No Memory.");
        
            return(errorCode);
        }

        errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
        if(CL_OK != errorCode) 
        {
            goto err_cleaup;
        }

        pBerkeleyHandle->pCurrentTxn = NULL;

        cdbBerkeleyLsnReset(dbEnvironment.pDBEnv, dbFile);

        rc = db_create(&(pBerkeleyHandle->pDatabase), dbEnvironment.pDBEnv, 0);
        if(0 != rc) 
        {        
            goto err_cleaup;
        }
        
        /* DB_SYNC is the only flag that we care about*/
        pBerkeleyHandle->bdbFlags = dbFlag & CL_DB_SYNC;       
        dbFlag &= ~CL_DB_SYNC; /* Remove the Sync flag since the rest of this routine assumes it does not exist */

        if (1) /* Let the environment variable override the coded behavior */
        {
            if (getenv("ASP_DB_SYNC"))
            {
                if (SAFplus::clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
                {
                    pBerkeleyHandle->bdbFlags |= CL_DB_SYNC | CL_DB_SYNC_OVERRIDE;
                }
                else
                {
                    pBerkeleyHandle->bdbFlags &= ~CL_DB_SYNC;
                    pBerkeleyHandle->bdbFlags |= CL_DB_SYNC_OVERRIDE;
                }
             }
         }
         pDBHandle = pBerkeleyHandle;
    }
    
    if (dbFlag == CL_DB_CREAT)
    {
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile, 
                (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_EXCL, 0);    
#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, 
                (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_EXCL, 0);
#endif

        if(0 == rc)
            goto Open_done;

        if(EEXIST == rc)
        {
            close();
            
            rc = dbEnvironment.pDBEnv->dbremove(dbEnvironment.pDBEnv, NULL, (ClCharT*)dbFile, (ClCharT*)dbName, 0);
            if (0 != rc)
            {
                goto err_cleaup;
            }            
            errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
            if(CL_OK != errorCode) 
            {
                goto err_cleaup;
            }

            cdbBerkeleyLsnReset(dbEnvironment.pDBEnv, dbFile);
            rc = db_create(&(pBerkeleyHandle->pDatabase), dbEnvironment.pDBEnv, 0);
            if(0 != rc)
            {
                goto err_cleaup;
            }

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile,
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);    
#else
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, 
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);
#endif
            if(0 != rc) {
                goto err_open_failed;
            }
        }
        else
        {
            goto err_open_failed;
        }

    }

    else if(dbFlag == CL_DB_OPEN)
    {
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                0, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                0, 0);
#endif
        if(0 != rc) 
        {
            pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "Database opened failed: %s", dbName);
            goto err_open_failed;
        }
    }
    else if(dbFlag == CL_DB_APPEND)
    {

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                DB_CREATE, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, 
                DB_CREATE, 0);
#endif

        if(0 != rc) 
        {
            goto err_open_failed;
        }
    }
    else
    {
        logError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB flag [0x%x].  Cannot open database [%s].",dbFlag,(ClCharT*)dbFile);
        goto err_cleaup;
    }

Open_done:

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_open_failed;
    }    

    //pDBHandle = pBerkeleyHandle;
    
    return(CL_OK);

err_open_failed:    
    pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase,rc,"%d\n",rc);
    pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
err_cleaup:    
    SAFplusHeapFree(pBerkeleyHandle);
    pDBHandle = NULL;
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB open failed.");
    
    return(errorCode);
}

ClRcT BerkeleyPlugin::close()
{
    logTrace("BERK", "CLOSE", "Enter [%s]", __FUNCTION__);
     
    ClRcT errorCode = CL_OK;

    if (!pDBHandle) return errorCode;

    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;

    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;  

    errorCode = clClistDelete(&(pBerkeleyHandle->transactionStackHandle));
    if(CL_OK != errorCode)
    {
        goto err_out;
    }
    if (pBerkeleyHandle->pCursor)
    {
       rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
       if(0 != rc)
       {
           goto err_out;
       }
    }
    rc = pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
    if(rc != 0)
    {
        goto err_out;
    }

    
    SAFplusHeapFree(pBerkeleyHandle);
    pDBHandle = NULL;

    logInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nBerkeley closed.");
    return (CL_OK);    

err_out:    
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB close failed.");
    
    return(errorCode);
}

ClRcT BerkeleyPlugin::insertRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;
    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);
    NULL_CHECK(pBerkeleyHandle);

    memset(&key, 0, sizeof(key));
	memset(&record, 0, sizeof(record));

    key.data = (void *)dbKey;
    key.size = keySize;

    record.data = (void *)dbRec;
    record.size = recSize;

    rc = pBerkeleyHandle->pDatabase->put(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, &record, DB_NOOVERWRITE);
    
    if(DB_KEYEXIST == rc)
    {
        /* Berkeley returned duplicate error, so return CL_ERR_DUPLICATE */
        errorCode = CL_DBAL_RC(CL_ERR_DUPLICATE);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Duplicate key");
        
        return(errorCode);
    }

    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record add failed.");
        pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "insertRecord:");
        return(errorCode);
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(0);
  
    return(CL_OK);
}

ClRcT BerkeleyPlugin::replaceRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT dbRec, ClUint32T recSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;
    
    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);
    NULL_CHECK(pBerkeleyHandle);

    memset(&key, 0, sizeof(key));
    memset(&record, 0, sizeof(record));

    key.data = (void *)dbKey;
    key.size = keySize;

    record.data = (void *)dbRec;
    record.size = recSize;

    rc = pBerkeleyHandle->pDatabase->put(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, &record, 0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record replace failed.");
        pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "replaceRecord:");
        return(errorCode);
    }  

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(0);

    
    return (CL_OK);
}

ClRcT BerkeleyPlugin::getRecord(ClDBKeyT dbKey, ClUint32T keySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    
    NULL_CHECK(dbKey);
    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    NULL_CHECK(pBerkeleyHandle);

    memset(&key, 0, sizeof(key));
    memset(&record, 0, sizeof(record));
	
    key.data = (void *)dbKey;
    key.size = keySize;
    record.flags = DB_DBT_MALLOC;
    rc = pBerkeleyHandle->pDatabase->get(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, &record,0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record get failed.");
        pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "getRecord:");
        return(errorCode);
    }

    *pDBRec = (ClDBRecordT) SAFplusHeapAlloc(record.size);
    if(NULL == *pDBRec) {
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record get failed.");
        
        return(errorCode);
    }

    memcpy(*pDBRec, record.data, record.size);
    *pRecSize = (ClUint32T)record.size;
    free(record.data);
    
    return (CL_OK);
}

ClRcT BerkeleyPlugin::getFirstRecord(ClDBKeyT* pDBKey, ClUint32T* pKeySize, ClDBRecordT* pDBRec, ClUint32T* pRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;    

    
    NULL_CHECK(pDBKey);
    NULL_CHECK(pKeySize);

    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    NULL_CHECK(pBerkeleyHandle);

    memset(&key, 0, sizeof(key));
	memset(&record, 0, sizeof(record));
    key.flags = DB_DBT_MALLOC;
    record.flags = DB_DBT_MALLOC;
    rc = pBerkeleyHandle->pCursor->c_get(pBerkeleyHandle->pCursor, &key, &record, DB_FIRST);
    if(0 != rc) {
        if(DB_NOTFOUND == rc) {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
            pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "getFirstRecord:");
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        
        return(errorCode);
    }

    *pDBKey = (ClDBKeyT) SAFplusHeapAlloc(key.size);
    if(NULL == *pDBKey) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        
        return(errorCode);
    }

    memcpy(*pDBKey, key.data, key.size);    
    *pKeySize = (ClUint32T)key.size;

    *pDBRec = (ClDBRecordT) SAFplusHeapAlloc(record.size);
    if(NULL == *pDBRec) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        
        return(errorCode);
    }

    memcpy(*pDBRec, record.data, record.size);
    *pRecSize = (ClUint32T)record.size;
    free(key.data);
    free(record.data);
    
    return (CL_OK);
}

ClRcT BerkeleyPlugin::getNextRecord(ClDBKeyT currentKey, ClUint32T currentKeySize, ClDBKeyT* pDBNextKey, ClUint32T* pNextKeySize, ClDBRecordT* pDBNextRec, ClUint32T* pNextRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    
    NULL_CHECK(currentKey);
    NULL_CHECK(pDBNextKey);
    NULL_CHECK(pNextKeySize);

    NULL_CHECK(pDBNextRec);
    NULL_CHECK(pNextRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;
    NULL_CHECK(pBerkeleyHandle);

    memset(&key, 0, sizeof(key));
	memset(&record, 0, sizeof(record));
	
    key.data = (void *)currentKey;
    key.size = currentKeySize;
    key.flags = DB_DBT_MALLOC;
    record.flags = DB_DBT_MALLOC;

    rc = pBerkeleyHandle->pCursor->c_get(pBerkeleyHandle->pCursor, &key, &record, DB_NEXT);
    if(0 != rc) {
        if(DB_NOTFOUND == rc) {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
            pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "getNextRecord:");
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        
        return(errorCode);
    }

    *pDBNextKey = (ClDBKeyT) SAFplusHeapAlloc(key.size);
    if(NULL == *pDBNextKey) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        
        return(errorCode);
    }

    memcpy(*pDBNextKey, key.data, key.size);    
    *pNextKeySize = (ClUint32T)key.size;

    *pDBNextRec = (ClDBRecordT) SAFplusHeapAlloc(record.size);
    if(NULL == *pDBNextRec) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        
        return(errorCode);
    }

    memcpy(*pDBNextRec, record.data, record.size);
    *pNextRecSize = (ClUint32T)record.size;
    free(key.data);
    free(record.data);
    
    return (CL_OK);
}

ClRcT BerkeleyPlugin::deleteRecord(ClDBKeyHandleT dbKey, ClUint32T keySize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;

    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    NULL_CHECK(pBerkeleyHandle);
    NULL_CHECK(dbKey);
    
    memset(&key, 0, sizeof(key));

    key.data = (void *)dbKey;
    key.size = keySize;

    rc = pBerkeleyHandle->pDatabase->del(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, 0);
    if(0 != rc) {
        if(DB_NOTFOUND == rc) {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record delete failed:Record not found.");
            pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase, rc, "deleteRecord:");
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record delete failed.");
        
        return(errorCode);
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(0);

    
    return (CL_OK);
}

ClRcT BerkeleyPlugin::syncDbal(ClUint32T flags)
{
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;

    

    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;
    if(!pBerkeleyHandle) return CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);

    if(!(pBerkeleyHandle->bdbFlags & CL_DB_SYNC))
        return berkeleyDBSync(0);
    
    return CL_OK;
}

ClRcT BerkeleyPlugin::openTransaction(ClDBFileT dbFile, ClDBNameT  dbName, ClDBFlagT dbFlag, ClUint32T  maxKeySize, ClUint32T  maxRecordSize)
{
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )

    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBTYPE berkeleyDBType = DB_BTREE;

    
    if(dbEnvironment.isInitialized == 0) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB NOT initialized.");
        
        return(errorCode);
    }


    if(dbFlag >= CL_DB_MAX_FLAG) {
        errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: Invalid flag.");
        
        return(errorCode);
    }

    pBerkeleyHandle = (BerkeleyDBHandle_t*) pDBHandle;
    if (!pBerkeleyHandle) 
    {
        pBerkeleyHandle = (BerkeleyDBHandle_t *)SAFplusHeapAlloc(sizeof(BerkeleyDBHandle_t));
        if(NULL == pBerkeleyHandle) {
            errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
            logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: No Memory.");
        
            return(errorCode);
        }

        errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
        if(CL_OK != errorCode) 
        {
            goto err_cleaup;
        }

        pBerkeleyHandle->pCurrentTxn = NULL;

        cdbBerkeleyLsnReset(dbEnvironment.pDBEnv, dbFile);

        rc = db_create(&(pBerkeleyHandle->pDatabase), dbEnvironment.pDBEnv, 0);
        if(0 != rc) 
        {        
            goto err_cleaup;
        }

        /* DB_SYNC is the only flag that we care about*/
        pBerkeleyHandle->bdbFlags = dbFlag & CL_DB_SYNC;       
        dbFlag &= ~CL_DB_SYNC; /* Remove the Sync flag since the rest of this routine assumes it does not exist */

        if (1) /* Let the environment variable override the coded behavior */
        {
            if (getenv("ASP_DB_SYNC"))
            {
                if (SAFplus::clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
                {
                    pBerkeleyHandle->bdbFlags |= CL_DB_SYNC | CL_DB_SYNC_OVERRIDE;
                }
                else
                {
                    pBerkeleyHandle->bdbFlags &= ~CL_DB_SYNC;
                    pBerkeleyHandle->bdbFlags |= CL_DB_SYNC_OVERRIDE;
                }
             }
         }
         pDBHandle = pBerkeleyHandle;
    }
    
    if (dbFlag == CL_DB_CREAT)
    {
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile, 
                (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_EXCL | DB_AUTO_COMMIT, 0);    
#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, 
                (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_EXCL | DB_AUTO_COMMIT, 0);
#endif

        if(0 == rc)
            goto txnOpen_done;

        if(EEXIST == rc)
        {
            close();
            
            rc = dbEnvironment.pDBEnv->dbremove(dbEnvironment.pDBEnv, NULL, (ClCharT*)dbFile, (ClCharT*)dbName, 0);
            if (0 != rc)
            {
                goto err_cleaup;
            }            
            errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
            if(CL_OK != errorCode) 
            {
                goto err_cleaup;
            }

            cdbBerkeleyLsnReset(dbEnvironment.pDBEnv, dbFile);
            rc = db_create(&(pBerkeleyHandle->pDatabase), dbEnvironment.pDBEnv, 0);
            if(0 != rc)
            {
                goto err_cleaup;
            }

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile,
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_AUTO_COMMIT, 0);    
#else
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, 
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE | DB_AUTO_COMMIT, 0);
#endif
            if(0 != rc) {
                goto err_open_failed;
            }
        }
        else
        {
            goto err_open_failed;
        }

    }

    else if(dbFlag == CL_DB_OPEN)
    {
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                DB_AUTO_COMMIT, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                DB_AUTO_COMMIT, 0);
#endif
        if(0 != rc) 
        {
            goto err_open_failed;
        }
    }
    else if(dbFlag == CL_DB_APPEND)
    {

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType,
                DB_CREATE | DB_AUTO_COMMIT, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase,
                (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, 
                DB_CREATE | DB_AUTO_COMMIT, 0);
#endif

        if(0 != rc) 
        {
            goto err_open_failed;
        }
    }
    else
    {
        logError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB flag [0x%x].  Cannot open database [%s].",dbFlag,(ClCharT*)dbFile);
        goto err_cleaup;
    }

txnOpen_done:

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_open_failed;
    }    

    //pDBHandle = pBerkeleyHandle;
    
    return(CL_OK);

err_open_failed:    
    pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase,rc,"%d\n",rc);
    pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
err_cleaup:    
    SAFplusHeapFree(pBerkeleyHandle);
    pDBHandle = NULL;
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB open failed.");
    
    return(errorCode);
#else
    return CL_OK;
#endif
}

ClRcT BerkeleyPlugin::beginTransaction()
{
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    ClUint32T flags = 0;
    DB_TXN* pParentTxn = NULL;

    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    pParentTxn = pBerkeleyHandle->pCurrentTxn;

    rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
    if(0 != rc) 
    {
        goto err_out;
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) flags = DB_TXN_SYNC;
    else flags = DB_TXN_NOSYNC;

    rc = (dbEnvironment.pDBEnv)->txn_begin(dbEnvironment.pDBEnv, pParentTxn, &(pBerkeleyHandle->pCurrentTxn), flags);

    if(0 != rc) 
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_out;
    }

    errorCode = clClistLastNodeAdd(pBerkeleyHandle->transactionStackHandle, (ClClistDataT)pParentTxn);
    if(CL_OK != errorCode)
    {
        goto err_out;
    }

    
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction begin failed.");
    
    return(errorCode);
#else
    return CL_OK;
#endif
}

ClRcT BerkeleyPlugin::commitTransaction()
{
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    ClUint32T flags = 0;
    DB_TXN* pParentTxn = NULL;
    ClClistNodeT nodeHandle = 0;

    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    errorCode = clClistLastNodeGet(pBerkeleyHandle->transactionStackHandle, &nodeHandle);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    errorCode = clClistDataGet(pBerkeleyHandle->transactionStackHandle, nodeHandle, (ClClistDataT *)&pParentTxn);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    errorCode = clClistNodeDelete(pBerkeleyHandle->transactionStackHandle, nodeHandle);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
    if(0 != rc) 
    {
        goto err_out;
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) flags = DB_TXN_SYNC;
    else flags = DB_TXN_NOSYNC;

    rc = pBerkeleyHandle->pCurrentTxn->commit(pBerkeleyHandle->pCurrentTxn, flags);
    if(0 != rc) 
    {
        goto err_out;
    }

    pBerkeleyHandle->pCurrentTxn = pParentTxn;    

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_out;
    }

    
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_COMMIT_FAILED);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction commit failed.");
    
    return(errorCode);
#else
    return CL_OK;
#endif
}

ClRcT BerkeleyPlugin::abortTransaction()
{
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DB_TXN* pParentTxn = NULL;
    ClClistNodeT nodeHandle = 0;

    
    pBerkeleyHandle = (BerkeleyDBHandle_t *)pDBHandle;

    errorCode = clClistLastNodeGet(pBerkeleyHandle->transactionStackHandle, &nodeHandle);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    errorCode = clClistDataGet(pBerkeleyHandle->transactionStackHandle, nodeHandle, (ClClistDataT *)&pParentTxn);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    errorCode = clClistNodeDelete(pBerkeleyHandle->transactionStackHandle, nodeHandle);
    if(CL_OK != errorCode) 
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
    if(0 != rc) 
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pCurrentTxn->abort(pBerkeleyHandle->pCurrentTxn);
    if(0 != rc) 
    {
        goto err_out;
    }

    pBerkeleyHandle->pCurrentTxn = pParentTxn;

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_out;
    }

    
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_ABORT_FAILED);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction abort failed.");
    
    return(errorCode);
#else
    return CL_OK;
#endif
}

ClRcT BerkeleyPlugin::freeRecord(ClDBRecordT dbRec)
{
     ClRcT errorCode = CL_OK;
    
    
     NULL_CHECK(dbRec);
     SAFplusHeapFree(dbRec);
    
     return (CL_OK);  
}

ClRcT BerkeleyPlugin::freeKey(ClDBRecordT dbKey)
{
     ClRcT errorCode = CL_OK;

    
     NULL_CHECK(dbKey);
     SAFplusHeapFree(dbKey);
    
     return (CL_OK);
}

ClRcT BerkeleyPlugin::berkeleyDBSync(ClUint32T flags)
{
    ClRcT rc = CL_OK;
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = (BerkeleyDBHandle_t*)pDBHandle;

    if(!pBerkeleyHandle) return CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    rc = pBerkeleyHandle->pDatabase->sync(pBerkeleyHandle->pDatabase, 0);
    if(0 != rc) 
    {
        if(EINVAL == rc)
        {
            errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
            logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DB sync failed : Passed invalid flag.");
            
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB sync failed.");
        
        return(errorCode);
    }  

    rc = (dbEnvironment.pDBEnv)->log_flush(dbEnvironment.pDBEnv, NULL);
    if (rc == EINVAL) /* dont return a sync error because the database was synced... */
    {
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DB log sync failed");    
    }

    
    return (CL_OK);

}

};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.
  SAFplus::BerkeleyPlugin* api = new SAFplus::BerkeleyPlugin();
  // Initialize the pluginData structure
  api->pluginId         = SAFplus::CL_DBAL_PLUGIN_ID;
  api->pluginVersion    = SAFplus::CL_DBAL_PLUGIN_VER;
  api->type = "Berkeley";

  // return it
  return (SAFplus::ClPlugin*) api;
}
