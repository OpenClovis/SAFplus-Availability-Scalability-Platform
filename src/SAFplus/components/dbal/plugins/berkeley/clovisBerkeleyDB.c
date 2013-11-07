/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : dbal
 * File        : clovisBerkeleyDB.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis Database Abstraction Layer implementation for   
 * Berkeley database.                                                       
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <db.h>
#include <sys/stat.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clDbalApi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clClistApi.h>
#include "clovisDbalInternal.h"
#include <clDbalCfg.h>


#define CL_DB_SYNC_OVERRIDE 2

/*****************************************************************************/
extern DBEngineInfo_t gDBEngineInfo;
extern ClDbalFunctionPtrsT *gDbalFunctionPtrs;
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
ClRcT clDbalInterface(ClDbalFunctionPtrsT *);
/*****************************************************************************/
static ClRcT  
cdbBerkeleyDBOpen(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBClose(ClDBHandleT dbHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBSync(ClDBHandleT dbHandle,
                  ClUint32T   flags);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordAdd(ClDBHandleT      dbHandle,
                       ClDBKeyT         dbKey,
                       ClUint32T       keySize,
                       ClDBRecordT      dbRec,
                       ClUint32T       recSize);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordReplace(ClDBHandleT      dbHandle,
                           ClDBKeyT         dbKey,
                           ClUint32T       keySize,
                           ClDBRecordT      dbRec,
                           ClUint32T       recSize);
/*****************************************************************************/
static ClRcT  
cdbBerkeleyDBRecordGet(ClDBHandleT      dbHandle,
                       ClDBKeyT         dbKey,
                       ClUint32T       keySize,
                       ClDBRecordT*     pDBRec,
                       ClUint32T*      pRecSize);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordDelete(ClDBHandleT      dbHandle,
                          ClDBKeyT         dbKey,
                          ClUint32T       keySize);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBFirstRecordGet(ClDBHandleT      dbHandle,
                            ClDBKeyT*        pDBKey,
                            ClUint32T*      pKeySize,
                            ClDBRecordT*     pDBRec,
                            ClUint32T*      pRecSize);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBNextRecordGet(ClDBHandleT      dbHandle,
                           ClDBKeyT         currentKey,
                           ClUint32T       currentKeySize,
                           ClDBKeyT*        pDBNextKey,
                           ClUint32T*      pNextKeySize,
                           ClDBRecordT*     pDBNextRec,
                           ClUint32T*      pNextRecSize);
/*****************************************************************************/
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
static ClRcT  
cdbBerkeleyDBTxnOpen(ClDBFileT    dbFile,
                     ClDBNameT    dbName, 
                     ClDBFlagT    dbFlag,
                     ClUint32T   maxKeySize,
                     ClUint32T   maxRecordSize,
                     ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBTransactionBegin(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBTransactionCommit(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBTransactionAbort(ClDBHandleT  dbHandle);
/*****************************************************************************/
#else
static ClRcT  
cdbBerkeleyDummyTxnOpen(ClDBFileT    dbFile,
                        ClDBNameT    dbName, 
                        ClDBFlagT    dbFlag,
                        ClUint32T   maxKeySize,
                        ClUint32T   maxRecordSize,
                        ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionBegin(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionCommit(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionAbort(ClDBHandleT  dbHandle);
#endif
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordFree(ClDBHandleT  dbHandle,
                        ClDBRecordT  dbRec );
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBKeyFree(ClDBHandleT  dbHandle,
                     ClDBKeyT     dbKey );
/*****************************************************************************/
static void
myDummyDeleteCallBack(ClClistDataT userData)
{
return;
}

/*****************************************************************************/
/* Globals */
BerkeleyDBEnvHandle_t gDBEnvironment;
/*****************************************************************************/

ClRcT
cdbBerkeleyDBInitialize(ClDBFileT    dbEnvFile,
                        ClDBEngineT* pEngineHandle)
{
    ClRcT errorCode = CL_OK;
    ClUint32T rc = 0;

    CL_FUNC_ENTER();
    if(gDBEnvironment.isInitialized == 1) {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL already initialized");
        CL_FUNC_EXIT();
        return (CL_OK);
    }
    
    NULL_CHECK(pEngineHandle);

    rc = db_env_create(&(gDBEnvironment.pDBEnv), 0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        return(errorCode);
    }
    gDBEnvironment.pDBEnv->set_errpfx(gDBEnvironment.pDBEnv,"DBAL");
    gDBEnvironment.pDBEnv->set_errfile(gDBEnvironment.pDBEnv,stderr);

    rc = (gDBEnvironment.pDBEnv)->open(gDBEnvironment.pDBEnv, NULL, DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE | DB_THREAD, 0);

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
            clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid value [%s] for environment variable [ASP_DB_SYNC].  Expecting 'TRUE' or 'FALSE'.",syncEnvVar);
          }
      }
#endif

    gDBEnvironment.validity = DATABASE_ENGINE_ID;
    gDBEnvironment.isInitialized = 1;

    *pEngineHandle = (ClDBEngineT)&gDBEnvironment;
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL initialize : DONE ");
    CL_FUNC_EXIT();
    return (CL_OK);

error_out:
    (gDBEnvironment.pDBEnv)->err(gDBEnvironment.pDBEnv, rc, "environment open");
    (gDBEnvironment.pDBEnv)->close(gDBEnvironment.pDBEnv, 0);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL initialize : NOT DONE");
    CL_FUNC_EXIT();
    return(errorCode);
}
/*****************************************************************************/
ClRcT
cdbBerkeleyDBShutdown(ClDBEngineT engineHandle)
{
    ClRcT errorCode = CL_OK;
    ClUint32T rc = 0;
    CL_FUNC_ENTER();

    if(gDBEnvironment.isInitialized == 0) 
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB not initialized");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    rc = (gDBEnvironment.pDBEnv)->close(gDBEnvironment.pDBEnv, 0);
    if(0 != rc) 
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley Shutdown failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    gDBEnvironment.isInitialized = 0;
    gDBEnvironment.validity = 0;
    
    CL_FUNC_EXIT();
    return(CL_OK);
}

ClRcT
clDbalConfigInitialize(void* pDbalConfiguration)
{
    ClRcT errorCode = CL_OK;
    ClDbalConfigurationT* pConfig = NULL;

    CL_FUNC_ENTER();
    pConfig = (ClDbalConfigurationT *)pDbalConfiguration;

    NULL_CHECK(pConfig);

    errorCode = cdbBerkeleyDBInitialize((ClDBFileT)pConfig->Database.berkeleyConfig.engineEnvironmentPath, &gDBEngineInfo.engineHandle);
    if(CL_OK != errorCode) 
    {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley Initialize failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    gDBEngineInfo.dbEngineType = 0;
    CL_FUNC_EXIT();
    return(CL_OK);
}
/*****************************************************************************/
ClRcT
clDbalEngineFinalize() 
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();
    errorCode = cdbBerkeleyDBShutdown(gDBEngineInfo.engineHandle);
    if(CL_OK != errorCode) 
    {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Engine Shutdown failed.");
        CL_FUNC_EXIT();
        return(errorCode);                    
    }
    CL_FUNC_EXIT();
    return(CL_OK);
}
/*****************************************************************************/
/*****************************************************************************/

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
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
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

static ClRcT  
cdbBerkeleyDBOpen(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBTYPE berkeleyDBType = DB_BTREE;

    CL_FUNC_ENTER();
    if(gDBEnvironment.isInitialized == 0) 
    {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB NOT initialized.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pDBHandle);

    if(dbFlag >= CL_DB_MAX_FLAG) 
    {
        errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: Invalid flag.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    pBerkeleyHandle = (BerkeleyDBHandle_t *)clHeapAllocate(sizeof(BerkeleyDBHandle_t));
    if(NULL == pBerkeleyHandle) 
    {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: No Memory.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
    if(CL_OK != errorCode)
    {
        goto err_cleanup;
    }

    pBerkeleyHandle->pCurrentTxn = NULL;
    
    cdbBerkeleyLsnReset(gDBEnvironment.pDBEnv, dbFile);

    rc = db_create(&(pBerkeleyHandle->pDatabase), gDBEnvironment.pDBEnv, 0);
    if(0 != rc)
    {        
        goto err_cleanup;
    }

    /* DB_SYNC is the only flag that we care about*/
    pBerkeleyHandle->bdbFlags = dbFlag & CL_DB_SYNC;
    dbFlag &= ~CL_DB_SYNC; /* Remove the Sync flag since the rest of this routine assumes it does not exist */

    if (1) /* Let the environment variable override the coded behavior */
    {
        if (getenv("ASP_DB_SYNC"))
        {
            if (clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
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
            goto open_done;

        if(EEXIST == rc)
        {
            rc = pBerkeleyHandle->pDatabase->remove(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, (ClCharT*)dbName, 0);
            if (0 != rc)
            {
                goto err_cleanup;
            }

            rc = db_create(&(pBerkeleyHandle->pDatabase), gDBEnvironment.pDBEnv, 0);
            if(0 != rc) 
            {
                goto err_cleanup;
            }

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile,
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);    
#else
            rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, 
                    (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);
#endif
            if(0 != rc) 
            {
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
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, 0, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, 0, 0);
#endif
        if(0 != rc) 
        {
            goto err_open_failed;
        }
    }
    else if(dbFlag == CL_DB_APPEND)
    {

#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1 )
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, NULL, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);    

#else
        rc = pBerkeleyHandle->pDatabase->open(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, (ClCharT*)dbName, berkeleyDBType, DB_CREATE, 0);
#endif

        if(0 != rc) 
        {
            goto err_open_failed;
        }
    }
    else
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB flag [0x%x].  Cannot open database [%s].",dbFlag,(ClCharT*)dbFile);
        goto err_cleanup;
    }

open_done:    

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_open_failed;
    }    

    *pDBHandle = pBerkeleyHandle;

    CL_FUNC_EXIT();
    return(CL_OK);

err_open_failed:   
    pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase,rc,"%d\n",rc);
    pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
err_cleanup:
    clHeapFree(pBerkeleyHandle);
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB open failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBClose(ClDBHandleT dbHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;  

    errorCode = clClistDelete(&(pBerkeleyHandle->transactionStackHandle));
    if(CL_OK != errorCode)
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
    if(0 != rc)
    {
        goto err_out;
    }

    rc = pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
    if(rc != 0)
    {
        goto err_out;
    }

    clHeapFree(pBerkeleyHandle);
    CL_FUNC_EXIT();
    return (CL_OK);    

err_out:    
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB close failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}

static ClRcT
berkeleyDBSync(BerkeleyDBHandle_t *pBerkeleyHandle, ClUint32T flags)
{
    ClRcT rc = CL_OK;
    ClRcT errorCode = CL_OK;

    if(!pBerkeleyHandle) return CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    rc = pBerkeleyHandle->pDatabase->sync(pBerkeleyHandle->pDatabase, 0);
    if(0 != rc) 
    {
        if(EINVAL == rc)
        {
            errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
            clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DB sync failed : Passed invalid flag.");
            CL_FUNC_EXIT();
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB sync failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }  

    rc = (gDBEnvironment.pDBEnv)->log_flush(gDBEnvironment.pDBEnv, NULL);
    if (rc == EINVAL) /* dont return a sync error because the database was synced... */
    {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DB log sync failed");    
    }

    CL_FUNC_EXIT();
    return (CL_OK);

}

/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordAdd(ClDBHandleT      dbHandle,
                       ClDBKeyT         dbKey,
                       ClUint32T       keySize,
                       ClDBRecordT      dbRec,
                       ClUint32T       recSize)
{   
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;
    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

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
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Duplicate key");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record add failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(pBerkeleyHandle, 0);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/*****************************************************************************/
static ClRcT
cdbBerkeleyDBSync(ClDBHandleT      dbHandle,
                  ClUint32T        flags)
{
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;

    CL_FUNC_ENTER();

    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;
    if(!pBerkeleyHandle) return CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);

    if(!(pBerkeleyHandle->bdbFlags & CL_DB_SYNC))
        return berkeleyDBSync(pBerkeleyHandle, 0);
    
    return CL_OK;
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordReplace(ClDBHandleT      dbHandle,
                     ClDBKeyT         dbKey,
                     ClUint32T       keySize,
                     ClDBRecordT      dbRec,
                     ClUint32T       recSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;
    
    NULL_CHECK(dbKey);
    NULL_CHECK(dbRec);

    memset(&key, 0, sizeof(key));
	memset(&record, 0, sizeof(record));

    key.data = (void *)dbKey;
    key.size = keySize;

    record.data = (void *)dbRec;
    record.size = recSize;

    rc = pBerkeleyHandle->pDatabase->put(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, &record, 0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record replace failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }  

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(pBerkeleyHandle, 0);

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*****************************************************************************/
static ClRcT  
cdbBerkeleyDBRecordGet(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT*     pDBRec,
                 ClUint32T*      pRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    CL_FUNC_ENTER();
    NULL_CHECK(dbKey);
    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

    memset(&key, 0, sizeof(key));
    memset(&record, 0, sizeof(record));
	
    key.data = (void *)dbKey;
    key.size = keySize;
    record.flags = DB_DBT_MALLOC;
    rc = pBerkeleyHandle->pDatabase->get(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, &record,0);
    if(0 != rc) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    *pDBRec = (ClDBRecordT) clHeapAllocate(record.size);
    if(NULL == *pDBRec) {
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    memcpy(*pDBRec, record.data, record.size);
    *pRecSize = (ClUint32T)record.size;
    free(record.data);
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordDelete(ClDBHandleT      dbHandle,
                    ClDBKeyT         dbKey,
                    ClUint32T       keySize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;

    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

    NULL_CHECK(dbKey);
    
    memset(&key, 0, sizeof(key));

    key.data = (void *)dbKey;
    key.size = keySize;

    rc = pBerkeleyHandle->pDatabase->del(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &key, 0);
    if(0 != rc) {
        if(DB_NOTFOUND == rc) {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record delete failed:Record not found.");
            CL_FUNC_EXIT();
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB record delete failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) berkeleyDBSync(pBerkeleyHandle,0);

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBFirstRecordGet(ClDBHandleT      dbHandle,
                      ClDBKeyT*        pDBKey,
                      ClUint32T*      pKeySize,
                      ClDBRecordT*     pDBRec,
                      ClUint32T*      pRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;    

    CL_FUNC_ENTER();
    NULL_CHECK(pDBKey);
    NULL_CHECK(pKeySize);

    NULL_CHECK(pDBRec);
    NULL_CHECK(pRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

    memset(&key, 0, sizeof(key));
	memset(&record, 0, sizeof(record));
    key.flags = DB_DBT_MALLOC;
    record.flags = DB_DBT_MALLOC;
    rc = pBerkeleyHandle->pCursor->c_get(pBerkeleyHandle->pCursor, &key, &record, DB_FIRST);
    if(0 != rc) {
        if(DB_NOTFOUND == rc) {
            errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
            clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
            CL_FUNC_EXIT();
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    *pDBKey = (ClDBKeyT) clHeapAllocate(key.size);
    if(NULL == *pDBKey) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    memcpy(*pDBKey, key.data, key.size);    
    *pKeySize = (ClUint32T)key.size;

    *pDBRec = (ClDBRecordT) clHeapAllocate(record.size);
    if(NULL == *pDBRec) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB first record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    memcpy(*pDBRec, record.data, record.size);
    *pRecSize = (ClUint32T)record.size;
    free(key.data);
    free(record.data);
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBNextRecordGet(ClDBHandleT      dbHandle,
                     ClDBKeyT         currentKey,
                     ClUint32T       currentKeySize,
                     ClDBKeyT*        pDBNextKey,
                     ClUint32T*      pNextKeySize,
                     ClDBRecordT*     pDBNextRec,
                     ClUint32T*      pNextRecSize)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBT key;
    DBT record;

    CL_FUNC_ENTER();
    NULL_CHECK(currentKey);
    NULL_CHECK(pDBNextKey);
    NULL_CHECK(pNextKeySize);

    NULL_CHECK(pDBNextRec);
    NULL_CHECK(pNextRecSize);

    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

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
            clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
            CL_FUNC_EXIT();
            return(errorCode);
        }
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    *pDBNextKey = (ClDBKeyT) clHeapAllocate(key.size);
    if(NULL == *pDBNextKey) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    memcpy(*pDBNextKey, key.data, key.size);    
    *pNextKeySize = (ClUint32T)key.size;

    *pDBNextRec = (ClDBRecordT) clHeapAllocate(record.size);
    if(NULL == *pDBNextRec) {
        free(key.data);
        free(record.data);
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB next record get failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    memcpy(*pDBNextRec, record.data, record.size);
    *pNextRecSize = (ClUint32T)record.size;
    free(key.data);
    free(record.data);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*****************************************************************************/
static ClRcT  
cdbBerkeleyDBTxnOpen(ClDBFileT    dbFile,
                     ClDBNameT    dbName, 
                     ClDBFlagT    dbFlag,
                     ClUint32T   maxKeySize,
                     ClUint32T   maxRecordSize,
                     ClDBHandleT* pDBHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DBTYPE berkeleyDBType = DB_BTREE;

    CL_FUNC_ENTER();
    if(gDBEnvironment.isInitialized == 0) {
        errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB NOT initialized.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pDBHandle);

    if(dbFlag >= CL_DB_MAX_FLAG) {
        errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: Invalid flag.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    pBerkeleyHandle = (BerkeleyDBHandle_t *)clHeapAllocate(sizeof(BerkeleyDBHandle_t));
    if(NULL == pBerkeleyHandle) {
        errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB Open failed: No Memory.");
        CL_FUNC_EXIT();
        return(errorCode);
    }

    errorCode = clClistCreate(0, CL_NO_DROP, myDummyDeleteCallBack, myDummyDeleteCallBack, &(pBerkeleyHandle->transactionStackHandle));
    if(CL_OK != errorCode) 
    {
        goto err_cleaup;
    }

    pBerkeleyHandle->pCurrentTxn = NULL;

    cdbBerkeleyLsnReset(gDBEnvironment.pDBEnv, dbFile);

    rc = db_create(&(pBerkeleyHandle->pDatabase), gDBEnvironment.pDBEnv, 0);
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
            if (clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
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
            rc = pBerkeleyHandle->pDatabase->remove(pBerkeleyHandle->pDatabase, (ClCharT*)dbFile, (ClCharT*)dbName, 0);
            if (0 != rc)
            {
                goto err_cleaup;
            }

            rc = db_create(&(pBerkeleyHandle->pDatabase), gDBEnvironment.pDBEnv, 0);
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
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB flag [0x%x].  Cannot open database [%s].",dbFlag,(ClCharT*)dbFile);
        goto err_cleaup;
    }

txnOpen_done:

    rc = pBerkeleyHandle->pDatabase->cursor(pBerkeleyHandle->pDatabase, pBerkeleyHandle->pCurrentTxn, &pBerkeleyHandle->pCursor, 0);
    if(0 != rc) 
    {
        goto err_open_failed;
    }    

    *pDBHandle = pBerkeleyHandle;
    CL_FUNC_EXIT();
    return(CL_OK);

err_open_failed:    
    pBerkeleyHandle->pDatabase->err(pBerkeleyHandle->pDatabase,rc,"%d\n",rc);
    pBerkeleyHandle->pDatabase->close(pBerkeleyHandle->pDatabase, 0);
err_cleaup:    
    clHeapFree(pBerkeleyHandle);
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB open failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}

/*****************************************************************************/
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
static ClRcT
cdbBerkeleyDBTransactionBegin(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    ClUint32T flags = 0;
    DB_TXN* pParentTxn = NULL;

    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

    pParentTxn = pBerkeleyHandle->pCurrentTxn;

    rc = pBerkeleyHandle->pCursor->c_close(pBerkeleyHandle->pCursor);
    if(0 != rc) 
    {
        goto err_out;
    }

    if (pBerkeleyHandle->bdbFlags & CL_DB_SYNC) flags = DB_TXN_SYNC;
    else flags = DB_TXN_NOSYNC;

    rc = (gDBEnvironment.pDBEnv)->txn_begin(gDBEnvironment.pDBEnv, pParentTxn, &(pBerkeleyHandle->pCurrentTxn), flags);

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

    CL_FUNC_EXIT();
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction begin failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBTransactionCommit(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    ClUint32T flags = 0;
    DB_TXN* pParentTxn = NULL;
    ClClistNodeT nodeHandle = 0;

    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

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

    CL_FUNC_EXIT();
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_COMMIT_FAILED);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction commit failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBTransactionAbort(ClDBHandleT  dbHandle)
{
    ClRcT errorCode = CL_OK;
    BerkeleyDBHandle_t* pBerkeleyHandle = NULL;
    ClUint32T rc = 0;
    DB_TXN* pParentTxn = NULL;
    ClClistNodeT nodeHandle = 0;

    CL_FUNC_ENTER();
    pBerkeleyHandle = (BerkeleyDBHandle_t *)dbHandle;

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

    CL_FUNC_EXIT();
    return (CL_OK);

err_out:
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_ABORT_FAILED);
    clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Berkeley DB transaction abort failed.");
    CL_FUNC_EXIT();
    return(errorCode);
}
#else
static ClRcT  
cdbBerkeleyDummyTxnOpen(ClDBFileT    dbFile,
                        ClDBNameT    dbName, 
                        ClDBFlagT    dbFlag,
                        ClUint32T   maxKeySize,
                        ClUint32T   maxRecordSize,
                        ClDBHandleT* pDBHandle)
{
    return CL_OK;
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionBegin(ClDBHandleT  dbHandle)
{
    return CL_OK;
}

/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionCommit(ClDBHandleT  dbHandle)
{
    return CL_OK;
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDummyTransactionAbort(ClDBHandleT  dbHandle)
{
    return CL_OK;
}
#endif

/*****************************************************************************/
static ClRcT
cdbBerkeleyDBRecordFree(ClDBHandleT  dbHandle,
                        ClDBRecordT  dbRec)
{
    ClRcT errorCode = CL_OK;
    
    CL_FUNC_ENTER();
    NULL_CHECK(dbRec);
    clHeapFree(dbRec);
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbBerkeleyDBKeyFree(ClDBHandleT  dbHandle,
                     ClDBKeyT     dbKey)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();
    NULL_CHECK(dbKey);
    clHeapFree(dbKey);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*****************************************************************************/
ClRcT clDbalInterface(ClDbalFunctionPtrsT  *funcDbPtr)
{
    ClDbalConfigT* pDbalConfiguration = NULL;
    ClRcT          rc = CL_OK;
    
    pDbalConfiguration = (ClDbalConfigT*)clHeapAllocate(sizeof(ClDbalConfigT));
    if ( NULL == pDbalConfiguration )
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL init Failed\n");
        return CL_DBAL_RC(CL_ERR_NO_MEMORY);
    }

    memset(pDbalConfiguration, '\0', sizeof(ClDbalConfigT));  
    strcpy((ClCharT*)pDbalConfiguration->Database.berkeleyConfig.engineEnvironmentPath, CL_DBAL_BERKELEY_ENV_PATH);
    if ((rc = clDbalConfigInitialize((void *)pDbalConfiguration)) != CL_OK )
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"DBAL init Failed\n");
        return rc;
    }

    funcDbPtr->fpCdbOpen              = cdbBerkeleyDBOpen;
    funcDbPtr->fpCdbClose             = cdbBerkeleyDBClose;
    funcDbPtr->fpCdbSync              = cdbBerkeleyDBSync;
    funcDbPtr->fpCdbRecordAdd         = cdbBerkeleyDBRecordAdd;
    funcDbPtr->fpCdbRecordReplace     = cdbBerkeleyDBRecordReplace;
    funcDbPtr->fpCdbRecordGet         = cdbBerkeleyDBRecordGet;
    funcDbPtr->fpCdbRecordDelete      = cdbBerkeleyDBRecordDelete;
    funcDbPtr->fpCdbFirstRecordGet    = cdbBerkeleyDBFirstRecordGet;
    funcDbPtr->fpCdbNextRecordGet     = cdbBerkeleyDBNextRecordGet;
#if ( DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 0 )
    funcDbPtr->fpCdbTxnOpen           = cdbBerkeleyDBTxnOpen;
    funcDbPtr->fpCdbTransactionBegin  = cdbBerkeleyDBTransactionBegin;
    funcDbPtr->fpCdbTransactionCommit = cdbBerkeleyDBTransactionCommit;
    funcDbPtr->fpCdbTransactionAbort  = cdbBerkeleyDBTransactionAbort;
#else
    funcDbPtr->fpCdbTxnOpen           = cdbBerkeleyDummyTxnOpen;
    funcDbPtr->fpCdbTransactionBegin  = cdbBerkeleyDummyTransactionBegin;
    funcDbPtr->fpCdbTransactionCommit = cdbBerkeleyDummyTransactionCommit;
    funcDbPtr->fpCdbTransactionAbort  = cdbBerkeleyDummyTransactionAbort;
#endif
    funcDbPtr->fpCdbRecordFree        = cdbBerkeleyDBRecordFree;
    funcDbPtr->fpCdbKeyFree           = cdbBerkeleyDBKeyFree;
    funcDbPtr->fpCdbFinalize          = clDbalEngineFinalize;
    funcDbPtr->validDatabase          = DATABASE_ID;

    clHeapFree(pDbalConfiguration);

    return rc; 
}
