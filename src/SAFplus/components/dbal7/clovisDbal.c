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
 * File        : clovisDbal.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis Database Abstraction Layer implementation       
 *****************************************************************************/
#include <unistd.h>
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clDbalApi.h>
#include "clovisDbalInternal.h"

/*****************************************************************************/
/* Globals */
DBEngineInfo_t gDBEngineInfo;
extern ClDbalFunctionPtrsT  *gDbalFunctionPtrs;
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
  NULL_CHECK(pDBHandle);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbOpen);
  errorCode = gDbalFunctionPtrs->fpCdbOpen(dbFile, dbName, dbFlag, maxKeySize, maxRecordSize, pDBHandle);
  return(errorCode);
}
/*****************************************************************************/
ClRcT
clDbalClose(ClDBHandleT dbHandle)
{
  ClRcT errorCode = CL_OK;

  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbClose);
  return(gDbalFunctionPtrs->fpCdbClose(dbHandle));
}

/*****************************************************************************/
ClRcT
clDbalSync(ClDBHandleT dbHandle, ClUint32T flags)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbSync);

  return(gDbalFunctionPtrs->fpCdbSync(dbHandle, flags));  
}
/*****************************************************************************/
ClRcT
clDbalRecordInsert(ClDBHandleT      dbHandle,
             ClDBKeyT         dbKey,
             ClUint32T       keySize,
             ClDBRecordT      dbRec,
             ClUint32T       recSize)
{
  ClRcT errorCode = CL_OK;

  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordAdd);

  return(gDbalFunctionPtrs->fpCdbRecordAdd(dbHandle,
                                   dbKey,
                                   keySize,
                                   dbRec,
                                   recSize));

}
/*****************************************************************************/
ClRcT
clDbalRecordReplace(ClDBHandleT      dbHandle,
                 ClDBKeyT         dbKey,
                 ClUint32T       keySize,
                 ClDBRecordT      dbRec,
                 ClUint32T       recSize)
{
  ClRcT errorCode = CL_OK;

  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordReplace);

  return(gDbalFunctionPtrs->fpCdbRecordReplace(dbHandle,
                                       dbKey,
                                       keySize,
                                       dbRec,
                                       recSize));
}
/*****************************************************************************/
ClRcT  
clDbalRecordGet(ClDBHandleT      dbHandle,
             ClDBKeyT         dbKey,
             ClUint32T       keySize,
             ClDBRecordT*     pDBRec,
             ClUint32T*      pRecSize)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordGet);

  return(gDbalFunctionPtrs->fpCdbRecordGet(dbHandle,
                                   dbKey,
                                   keySize,
                                   pDBRec,
                                   pRecSize));
}
/*****************************************************************************/
ClRcT
clDbalRecordDelete(ClDBHandleT      dbHandle,
                ClDBKeyT         dbKey,
                ClUint32T       keySize)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordDelete);
  return(gDbalFunctionPtrs->fpCdbRecordDelete(dbHandle,
                                      dbKey,
                                      keySize));
}
/*****************************************************************************/
ClRcT
clDbalFirstRecordGet(ClDBHandleT      dbHandle,
                  ClDBKeyT*        pDBKey,
                  ClUint32T*      pKeySize,
                  ClDBRecordT*     pDBRec,
                  ClUint32T*      pRecSize)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbFirstRecordGet);
  return(gDbalFunctionPtrs->fpCdbFirstRecordGet(dbHandle,
                                        pDBKey,
                                        pKeySize,
                                        pDBRec,
                                        pRecSize));
}
/*****************************************************************************/
ClRcT
clDbalNextRecordGet(ClDBHandleT      dbHandle,
                 ClDBKeyT         currentKey,
                 ClUint32T       currentKeySize,
                 ClDBKeyT*        pDBNextKey,
                 ClUint32T*      pNextKeySize,
                 ClDBRecordT*     pDBNextRec,
                 ClUint32T*      pNextRecSize)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbNextRecordGet);
  return(gDbalFunctionPtrs->fpCdbNextRecordGet(dbHandle,
                                       currentKey,
                                       currentKeySize,
                                       pDBNextKey,
                                       pNextKeySize,
                                       pDBNextRec,
                                       pNextRecSize));
}
/*****************************************************************************/
ClRcT
clDbalTxnOpen(ClDBFileT    dbFile,
              ClDBNameT    dbName,
              ClDBFlagT    dbFlag,
              ClUint32T   maxKeySize,
              ClUint32T   maxRecordSize,
              ClDBHandleT* pDBHandle)
{
  ClRcT errorCode = CL_OK;
  NULL_CHECK(pDBHandle);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTxnOpen);
  errorCode = gDbalFunctionPtrs->fpCdbTxnOpen(dbFile, dbName, dbFlag, maxKeySize, maxRecordSize, pDBHandle);
  return(errorCode);
}
/*****************************************************************************/
ClRcT
clDbalTransactionBegin(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionBegin);
  return(gDbalFunctionPtrs->fpCdbTransactionBegin(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalTransactionCommit(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionCommit);
  return(gDbalFunctionPtrs->fpCdbTransactionCommit(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalTransactionAbort(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionAbort);
  return(gDbalFunctionPtrs->fpCdbTransactionAbort(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalRecordFree(ClDBHandleT  dbHandle,
                 ClDBRecordT      dbRec)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordFree);
  return(gDbalFunctionPtrs->fpCdbRecordFree(dbHandle,dbRec));
}
/*****************************************************************************/
ClRcT
clDbalKeyFree(ClDBHandleT  dbHandle,
              ClDBKeyT     dbKey)
{
  ClRcT errorCode = CL_OK;
  DBAL_HDL_CHECK(dbHandle);
  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbKeyFree);
  return(gDbalFunctionPtrs->fpCdbKeyFree(dbHandle,dbKey));
}

