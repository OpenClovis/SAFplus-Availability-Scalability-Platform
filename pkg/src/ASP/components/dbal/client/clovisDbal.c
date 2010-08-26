/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
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
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDbalApi.h>
#include <clOsalApi.h>
#include "clovisDbalInternal.h"
#include <clDebugApi.h>
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

  CL_FUNC_ENTER();
  NULL_CHECK(pDBHandle);

  NULL_CHECK(gDbalFunctionPtrs->fpCdbOpen);
  CL_FUNC_EXIT();
  errorCode = gDbalFunctionPtrs->fpCdbOpen(dbFile, dbName, dbFlag, maxKeySize, maxRecordSize, pDBHandle);
  return(errorCode);
}
/*****************************************************************************/
ClRcT
clDbalClose(ClDBHandleT dbHandle)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbClose);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbClose(dbHandle));  

}
/*****************************************************************************/
ClRcT
clDbalSync(ClDBHandleT dbHandle, ClUint32T flags)
{
  ClRcT errorCode = CL_OK;
  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbSync);

  CL_FUNC_EXIT();
  
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordAdd);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordReplace);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordGet);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordDelete);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbFirstRecordGet);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbNextRecordGet);

  CL_FUNC_EXIT();
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

  CL_FUNC_ENTER();
  NULL_CHECK(pDBHandle);

  NULL_CHECK(gDbalFunctionPtrs->fpCdbTxnOpen);
  CL_FUNC_EXIT();
  errorCode = gDbalFunctionPtrs->fpCdbTxnOpen(dbFile, dbName, dbFlag, maxKeySize, maxRecordSize, pDBHandle);
  return(errorCode);
}
/*****************************************************************************/
ClRcT
clDbalTransactionBegin(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionBegin);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbTransactionBegin(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalTransactionCommit(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionCommit);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbTransactionCommit(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalTransactionAbort(ClDBHandleT  dbHandle)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbTransactionAbort);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbTransactionAbort(dbHandle));
}
/*****************************************************************************/
ClRcT
clDbalRecordFree(ClDBHandleT  dbHandle,
                 ClDBRecordT      dbRec)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbRecordFree);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbRecordFree(dbHandle,dbRec));
}
/*****************************************************************************/
ClRcT
clDbalKeyFree(ClDBHandleT  dbHandle,
              ClDBKeyT     dbKey)
{
  ClRcT errorCode = CL_OK;

  CL_FUNC_ENTER();
  DBAL_HDL_CHECK(dbHandle);

  NULL_CHECK(gDbalFunctionPtrs);
  VALIDITY_CHECK(gDbalFunctionPtrs->validDatabase);
  NULL_CHECK(gDbalFunctionPtrs->fpCdbKeyFree);

  CL_FUNC_EXIT();
  return(gDbalFunctionPtrs->fpCdbKeyFree(dbHandle,dbKey));
}

