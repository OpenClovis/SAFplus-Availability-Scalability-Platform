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
 * File        : clovisGDBM.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis Database Abstraction Layer implementation for   
 * GDBM                                                                      
 *****************************************************************************/
#include <stdlib.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <gdbm.h>
#include <clDbalApi.h>
#include <clLogApi.hxx>
#include <clDbg.hxx>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include "clovisDbalInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/*****************************************************************************/
ClRcT clDbalInterface(ClDbalFunctionPtrsT *ptrs);
/*****************************************************************************/

static ClRcT  
cdbGDBMOpen(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT
cdbGDBMClose(ClDBHandleT dbHandle);
/*****************************************************************************/
static ClRcT
cdbGDBMRecordAdd(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT      dbRec,
                 ClUint32T       recSize);
/*****************************************************************************/
static ClRcT
cdbGDBMRecordReplace(ClDBHandleT      dbHandle,
                     ClDBKeyT         dbKey,
                     ClUint32T       keySize,
                     ClDBRecordT      dbRec,
                     ClUint32T       recSize);
/*****************************************************************************/
static ClRcT  
cdbGDBMRecordGet(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT*     pDBRec,
                 ClUint32T*      pRecSize);
/*****************************************************************************/
static ClRcT
cdbGDBMRecordDelete(ClDBHandleT      dbHandle,
                    ClDBKeyT         dbKey,
                    ClUint32T       keySize);
/*****************************************************************************/
static ClRcT
cdbGDBMFirstRecordGet(ClDBHandleT      dbHandle,
                      ClDBKeyT*        pDBKey,
                      ClUint32T*      pKeySize,
                      ClDBRecordT*     pDBRec,
                      ClUint32T*      pRecSize);
/*****************************************************************************/
static ClRcT
cdbGDBMNextRecordGet(ClDBHandleT      dbHandle,
                     ClDBKeyT         currentKey,
                     ClUint32T       currentKeySize,
                     ClDBKeyT*        pDBNextKey,
                     ClUint32T*      pNextKeySize,
                     ClDBRecordT*     pDBNextRec,
                     ClUint32T*      pNextRecSize);
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionBegin(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionCommit(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionAbort(ClDBHandleT  dbHandle);
/*****************************************************************************/
static ClRcT
cdbGDBMRecordFree(ClDBHandleT  dbHandlei,
                  ClDBRecordT  dbRec);
/*****************************************************************************/
static ClRcT
cdbGDBMKeyFree(ClDBHandleT  dbHandle,
               ClDBRecordT  dbKey);
/*****************************************************************************/
ClRcT
clDbalConfigInitialize(void* pDbalConfiguration)
{
    return(CL_OK);
}
/*****************************************************************************/
ClRcT
clDbalEngineFinalize() 
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT  
cdbGDBMOpen(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle)
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = NULL;
  ClUint32T read_write = 0;
  ClDBTypeT  dbType = CL_DB_TYPE_BTREE;

  CL_FUNC_ENTER();
  NULL_CHECK(pDBHandle);

  NULL_CHECK(dbName);

  /* Validate the flag */
  if(dbFlag >= CL_DB_MAX_FLAG) {
    errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid Flag");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  
  /* Validate database type */
  if(dbType >= CL_DB_MAX_TYPE) {
    errorCode = CL_DBAL_RC(CL_ERR_INVALID_PARAMETER);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Invalid DB Type");
    CL_FUNC_EXIT();
    return(errorCode);
  }

  pGDBMHandle = (GDBMHandle_t*)clHeapCalloc(1,sizeof(GDBMHandle_t));

  if(NULL == pGDBMHandle) {
    errorCode = CL_DBAL_RC(CL_ERR_NO_MEMORY);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Memory allocation failed.");
    CL_FUNC_EXIT();
    return(errorCode);
  }

  pGDBMHandle->syncMode = CL_FALSE;

  if(CL_DB_SYNC & dbFlag)
  {
      dbFlag &= ~(CL_DB_SYNC);
      pGDBMHandle->syncMode = CL_TRUE;
  }

  /* Let the env variable override the coded behaviour */
  if (getenv("ASP_DB_SYNC"))
  {
      if (clParseEnvBoolean("ASP_DB_SYNC") == CL_TRUE)
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
        clHeapFree(pGDBMHandle);
        errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Open failed.");
        CL_FUNC_EXIT();
        return(errorCode);
    }
    if (NULL != pGDBMHandle->gdbmInstance)   gdbm_close(pGDBMHandle->gdbmInstance);  
    if (CL_DB_OPEN == dbFlag)
        read_write = GDBM_WRITER;
    else if (CL_DB_APPEND == dbFlag)
        read_write = GDBM_WRCREAT;
    else
    {
        clHeapFree(pGDBMHandle);
        errorCode = CL_DBAL_RC(CL_ERR_BAD_FLAG);
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nInvalid DB flag. GDBM open failed.");
        CL_FUNC_EXIT();
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
    clHeapFree(pGDBMHandle);
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Open failed.");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  pGDBMHandle->deleteRecCnt = 0;

  /* Return the handle to the created/opened database */  
  *pDBHandle = (ClDBHandleT)pGDBMHandle;
  CL_FUNC_EXIT();
  return(CL_OK);

}
/*****************************************************************************/
static ClRcT
cdbGDBMClose(ClDBHandleT dbHandle)  /* Handle to the database */
{
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;

  CL_FUNC_ENTER();
  /* close the GDBM database */
  gdbm_close(pGDBMHandle->gdbmInstance);

  /* make the GDBM handle invalid */
  clHeapFree(pGDBMHandle);

  logInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM closed.");
  CL_FUNC_EXIT();
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbGDBMRecordAdd(ClDBHandleT      dbHandle, /* Handle to the database */
                 ClDBKeyT         dbKey,    /* Handle to the key being added */
                 ClUint32T       keySize,  /* Size of the key being added */
                 ClDBRecordT      dbRec,    /* Handle to the record being added */
                 ClUint32T       recSize)  /* Size of the record being added */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  int returnCode = 0;
  datum key = {NULL, 0};
  datum data = {NULL, 0};
  
  CL_FUNC_ENTER();
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
    CL_FUNC_EXIT();
    return(errorCode);
  }

  if(0 != returnCode) {
    /* If not, some other GDBM error occured. return DB error */
    errorCode = CL_DBAL_RC(CL_DBAL_ERR_DB_ERROR);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM ERROR");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  CL_FUNC_EXIT();
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbGDBMRecordReplace(ClDBHandleT      dbHandle, /* Handle to the database */
                     ClDBKeyT         dbKey,    /* Handle to the key being added */
                     ClUint32T       keySize,  /* Size of the key being added */
                     ClDBRecordT      dbRec,    /* Handle to the record being added */
                     ClUint32T       recSize)  /* Size of the record being added */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  ClUint32T returnCode = 0;
  datum key = {NULL, 0};
  datum data = {NULL, 0};
  
  CL_FUNC_ENTER();
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
    CL_FUNC_EXIT();
    return(errorCode);
  }

  CL_FUNC_EXIT();
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT  
cdbGDBMRecordGet(ClDBHandleT      dbHandle, /* Handle to the database */
                 ClDBKeyT         dbKey,    /* Handle to the key of the record being retrieved */
                 ClUint32T       keySize,  /* Size of the key */
                 ClDBRecordT*     pDBRec,   /* Pointer to handle in which the record handle is returned */
                 ClUint32T*      pRecSize) /* Pointer to size, in which the size of the record is returned */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  datum key = {NULL, 0};
  datum data = {NULL, 0};

  CL_FUNC_ENTER();
  NULL_CHECK(pDBRec);

  NULL_CHECK(pRecSize);

  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  /* retrieve the record from the database */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, key);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM fetch failed");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  
  *pDBRec = (ClDBRecordT)data.dptr;
  *pRecSize = data.dsize;

  CL_FUNC_EXIT();
  return(CL_OK);  
}
/*****************************************************************************/
static ClRcT
cdbGDBMRecordDelete(ClDBHandleT      dbHandle,  /* Handle to the database */
                    ClDBKeyT         dbKey,     /* Handle to the key of the record being deleted */
                    ClUint32T       keySize)   /* Size of the key */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  ClInt32T returnCode = 0;
  datum key = {NULL, 0};

  CL_FUNC_ENTER();
  key.dsize = keySize;
  key.dptr = (ClCharT *)dbKey;

  returnCode = gdbm_delete(pGDBMHandle->gdbmInstance, key);

  if(0 != returnCode) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM delete failed");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  pGDBMHandle->deleteRecCnt++;

  if( CL_GDBM_MAX_DEL_CNT <= pGDBMHandle->deleteRecCnt )
  {
      returnCode = gdbm_reorganize(pGDBMHandle->gdbmInstance);
      if(0 != returnCode) {
          errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
          logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM reorganize failed");
          CL_FUNC_EXIT();
          return(errorCode);
      }
      pGDBMHandle->deleteRecCnt = 0;
  }
  CL_FUNC_EXIT();
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbGDBMFirstRecordGet(ClDBHandleT      dbHandle,    /* Handle to the database */
                      ClDBKeyT*        pDBKey,      /* Pointer to handle in which the key handle is returned */
                      ClUint32T*      pKeySize,    /* Pointer to size, in which the size of the key is returned */
                      ClDBRecordT*     pDBRec,      /* Pointer to handle in which the record handle is returned */
                      ClUint32T*      pRecSize)    /* Pointer to size, in which the size of the record is returned */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  datum key = {NULL, 0};
  datum data = {NULL, 0};

  CL_FUNC_ENTER();
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
    CL_FUNC_EXIT();
    return(errorCode);
  }

  *pDBKey = (ClDBKeyT)key.dptr;
  *pKeySize = key.dsize;

  /* Retrieve the associated record in the database */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, key);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM record fetch failed");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  
  *pDBRec = (ClDBRecordT)data.dptr;
  *pRecSize = data.dsize;

  CL_FUNC_EXIT();
  return(CL_OK);  

}
/*****************************************************************************/
static ClRcT
cdbGDBMNextRecordGet(ClDBHandleT      dbHandle,         /* Handle to the database */
                     ClDBKeyT         currentKey,       /* Handle to the current key */
                     ClUint32T       currentKeySize,   /* Size of the current key */
                     ClDBKeyT*        pDBNextKey,       /* pointer to handle in which the next key is returned */
                     ClUint32T*      pNextKeySize,     /* pointer to size in which the next key's size is returned */
                     ClDBRecordT*     pDBNextRec,       /* pointer to handle in which the next record is returned */
                     ClUint32T*      pNextRecSize)     /* pointer to size in which the next record's size is returned */
{
  ClRcT errorCode = CL_OK;
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;
  datum key = {NULL, 0};
  datum nextKey = {NULL, 0};
  datum data = {NULL, 0};

  CL_FUNC_ENTER();
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
    CL_FUNC_EXIT();
    return(errorCode);
  }

  *pDBNextKey = (ClDBKeyT)nextKey.dptr;
  *pNextKeySize = nextKey.dsize;

  /* retrieve the associated record */
  data = gdbm_fetch(pGDBMHandle->gdbmInstance, nextKey);

  if(NULL == data.dptr) {
    errorCode = CL_DBAL_RC(CL_ERR_NOT_EXIST);
    logTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM fetch record failed");
    CL_FUNC_EXIT();
    return(errorCode);
  }
  
  *pDBNextRec = (ClDBRecordT)data.dptr;
  *pNextRecSize = data.dsize;

  CL_FUNC_EXIT();
  return(CL_OK);  

}
/*****************************************************************************/
static ClRcT  
cdbGDBMSync(ClDBHandleT  dbHandle,
            ClUint32T    flags)
{
  GDBMHandle_t* pGDBMHandle = (GDBMHandle_t*)dbHandle;

  CL_FUNC_ENTER();
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

  CL_FUNC_EXIT();
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT  
cdbGDBMTxnOpen(ClDBFileT    dbFile,
               ClDBNameT    dbName, 
               ClDBFlagT    dbFlag,
               ClUint32T   maxKeySize,
               ClUint32T   maxRecordSize,
               ClDBHandleT* pDBHandle)
{
  ClRcT errorCode = CL_OK;
  CL_FUNC_ENTER();
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  CL_FUNC_EXIT();
  return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionBegin(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  CL_FUNC_ENTER();
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  CL_FUNC_EXIT();
  return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionCommit(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  CL_FUNC_ENTER();
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  CL_FUNC_EXIT();
  return(errorCode);
}
/*****************************************************************************/
static ClRcT
cdbGDBMTransactionAbort(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  CL_FUNC_ENTER();
  /* Transactions are not supported in GDBM. So return CL_ERR_NOT_SUPPORTED */
  errorCode = CL_DBAL_RC(CL_ERR_NOT_SUPPORTED);
  logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nGDBM Transaction not supported");
  CL_FUNC_EXIT();
  return(errorCode);
}
/*****************************************************************************/
static ClRcT  
cdbGDBMRecordFree(ClDBHandleT      dbHandle, /* Handle to the database */
                  ClDBRecordT      dbRec)   /* Pointer to handle in which the record handle is returned */
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  NULL_CHECK(dbRec);
  free(dbRec);
  CL_FUNC_EXIT();
  return(CL_OK);  
}
/*****************************************************************************/
static ClRcT  
cdbGDBMKeyFree(ClDBHandleT      dbHandle, /* Handle to the database */
              ClDBKeyT          dbKey)   /* Pointer to handle in which the record handle is returned */
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  NULL_CHECK(dbKey);
  free(dbKey);
  CL_FUNC_EXIT();
  return(CL_OK);  
}
/*****************************************************************************/
ClRcT clDbalInterface(ClDbalFunctionPtrsT  *funcDbPtr)
{
  ClRcT rc = CL_OK;

  funcDbPtr->fpCdbOpen              = cdbGDBMOpen;
  funcDbPtr->fpCdbClose             = cdbGDBMClose;
  funcDbPtr->fpCdbSync              = cdbGDBMSync;
  funcDbPtr->fpCdbRecordAdd         = cdbGDBMRecordAdd;
  funcDbPtr->fpCdbRecordReplace     = cdbGDBMRecordReplace;
  funcDbPtr->fpCdbRecordGet         = cdbGDBMRecordGet;
  funcDbPtr->fpCdbRecordDelete      = cdbGDBMRecordDelete;
  funcDbPtr->fpCdbFirstRecordGet    = cdbGDBMFirstRecordGet;
  funcDbPtr->fpCdbNextRecordGet     = cdbGDBMNextRecordGet;
  funcDbPtr->fpCdbTxnOpen           = cdbGDBMTxnOpen;
  funcDbPtr->fpCdbTransactionBegin  = cdbGDBMTransactionBegin;
  funcDbPtr->fpCdbTransactionCommit = cdbGDBMTransactionCommit;
  funcDbPtr->fpCdbTransactionAbort  = cdbGDBMTransactionAbort;
  funcDbPtr->fpCdbRecordFree        = cdbGDBMRecordFree;
  funcDbPtr->fpCdbKeyFree           = cdbGDBMKeyFree;
  funcDbPtr->fpCdbFinalize          = clDbalEngineFinalize;
  funcDbPtr->validDatabase          = DATABASE_ID;

  return rc;
}

#ifdef __cplusplus
}
#endif

