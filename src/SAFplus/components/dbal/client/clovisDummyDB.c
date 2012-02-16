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
 * File        : clovisDummyDB.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis Database Abstraction Layer template.            
 *****************************************************************************/
#include <stdlib.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDbalApi.h>
#include <clOsalApi.h>
#include "clovisDbalInternal.h"

/*****************************************************************************/
typedef struct DummyHandle_t {
  CdbDatabase_t  database;          /* List of function pointers for the undelying DB functions */
} DummyHandle_t;
/*****************************************************************************/
ClRcT  
cdbDummyOpen(ClDBNameT    dbName,
            ClDBTypeT    dbType,
            ClDBFlagT    dbFlag,
            ClDBHandleT* pDBHandle);
/*****************************************************************************/
static ClRcT
cdbDummyClose(ClDBHandleT dbHandle)
{
    DummyHandle_t* pDummyHandle = (DummyHandle_t*)dbHandle;

    /* make the Dummy DB handle invalid */
    pDummyHandle->database.validDatabase = 0;
    clHeapFree(pDummyHandle);

    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyRecordAdd(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT      dbRec,
                 ClUint32T       recSize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyRecordReplace(ClDBHandleT      dbHandle,
                     ClDBKeyT         dbKey,
                     ClUint32T       keySize,
                     ClDBRecordT      dbRec,
                     ClUint32T       recSize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT  
cdbDummyRecordGet(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT*     pDBRec,
                 ClUint32T*      pRecSize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyRecordDelete(ClDBHandleT      dbHandle,
                    ClDBKeyT         dbKey,
                    ClUint32T       keySize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyFirstRecordGet(ClDBHandleT      dbHandle,
                      ClDBKeyT*        pDBKey,
                      ClUint32T*      pKeySize,
                      ClDBRecordT*     pDBRec,
                      ClUint32T*      pRecSize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyNextRecordGet(ClDBHandleT      dbHandle,
                     ClDBKeyT         currentKey,
                     ClUint32T       currentKeySize,
                     ClDBKeyT*        pDBNextKey,
                     ClUint32T*      pNextKeySize,
                     ClDBRecordT*     pDBNextRec,
                     ClUint32T*      pNextRecSize)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyTransactionBegin(ClDBHandleT  dbHandle)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyTransactionCommit(ClDBHandleT  dbHandle)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyTransactionAbort(ClDBHandleT  dbHandle)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyRecordFree(ClDBHandleT  dbHandle,
                   ClDBRecordT  dbRec)
{
    return(CL_OK);
}
/*****************************************************************************/
static ClRcT
cdbDummyKeyFree(ClDBHandleT  dbHandle,
                ClDBKeyT  dbKey)
{
    return(CL_OK);
}
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
ClRcT  
clDbalOpen(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle)
{
    ClRcT errorCode = CL_OK;

    errorCode = cdbDummyOpen(dbName, CL_DB_TYPE_BTREE, dbFlag, pDBHandle);
    if(CL_OK != errorCode) {
        return(errorCode);
    }
    
    return(CL_OK);
}
/*****************************************************************************/
ClRcT  
cdbDummyOpen(ClDBNameT    dbName,        /* Name of the database */
            ClDBTypeT    dbType,        /* Database type : HASH or BTREE */
            ClDBFlagT    dbFlag,        /* Flag specifying create/open */
            ClDBHandleT* pDBHandle)     /* handle being returned */
{
  ClRcT errorCode = CL_OK;
  DummyHandle_t* pDummyHandle = NULL;
  
  NULL_CHECK(pDBHandle);

  NULL_CHECK(dbName);

  /* Validate the flag */
  if(dbFlag >= CL_DB_MAX_FLAG) {
    errorCode = CL_RC(CL_CID_DBAL,CL_ERR_INVALID_PARAMETER);
    return(errorCode);
  }
  
  /* Validate database type */
  if(dbType >= CL_DB_MAX_TYPE) {
    errorCode = CL_RC(CL_CID_DBAL,CL_ERR_INVALID_PARAMETER);
    return(errorCode);
  }

  pDummyHandle = (DummyHandle_t*)clHeapAllocate(sizeof(DummyHandle_t));

  if(NULL == pDummyHandle) {
    errorCode = CL_RC(CL_CID_DBAL, CL_ERR_NO_MEMORY);
    return(errorCode);
  }


  /* Initialize the list of function pointers to point to Dummy functions */
  pDummyHandle->database.fpCdbClose = cdbDummyClose;
  pDummyHandle->database.fpCdbRecordAdd = cdbDummyRecordAdd;
  pDummyHandle->database.fpCdbRecordReplace = cdbDummyRecordReplace;
  pDummyHandle->database.fpCdbRecordGet = cdbDummyRecordGet;
  pDummyHandle->database.fpCdbRecordDelete = cdbDummyRecordDelete;
  pDummyHandle->database.fpCdbFirstRecordGet = cdbDummyFirstRecordGet;
  pDummyHandle->database.fpCdbNextRecordGet = cdbDummyNextRecordGet;
  pDummyHandle->database.fpCdbTransactionBegin = cdbDummyTransactionBegin;
  pDummyHandle->database.fpCdbTransactionCommit = cdbDummyTransactionCommit;
  pDummyHandle->database.fpCdbTransactionAbort = cdbDummyTransactionAbort;
  pDummyHandle->database.fpCdbRecordFree  = cdbDummyRecordFree;
  pDummyHandle->database.fpCdbKeyFree  = cdbDummyKeyFree;
  pDummyHandle->database.validDatabase = DATABASE_ID;

  /* Return the handle to the created/opened database */  
  *pDBHandle = (ClDBHandleT)pDummyHandle;
  return(CL_OK);

}
