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
 * File        : clovisDbalInternal.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis Database Abstraction Layer implementation       
 *****************************************************************************/

#ifndef __INC_DBAL_INTERNAL_H__
#define __INC_DBAL_INTERNAL_H__

#include <clDbg.hxx>

# ifdef __cplusplus
extern "C"
{
# endif

#define DATABASE_ID         0x2411
#define DATABASE_ENGINE_ID  0x1124
/*****************************************************************************/
#define NULL_CHECK(X) if(NULL == (X)) { \
        errorCode = CL_RC(CL_CID_DBAL, CL_ERR_NULL_POINTER); \
        clDbgCodeError(errorCode, ("Parameter " #X " is NULL!")); \
                        return(errorCode); \
                      }
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
typedef struct DBEngineInfo_t {
    ClUint32T  dbEngineType;
    ClDBEngineT engineHandle;
}DBEngineInfo_t;
/*****************************************************************************/
/*****************************************************************************/
typedef ClRcT  
(*FPCdbOpen)(ClDBFileT    dbFile,
        ClDBNameT    dbName, 
        ClDBFlagT    dbFlag,
        ClUint32T   maxKeySize,
        ClUint32T   maxRecordSize,
        ClDBHandleT* pDBHandle);
/*****************************************************************************/
ClRcT dbalGetLibName(ClCharT  *pLibName);
/*****************************************************************************/
typedef ClRcT
(*FPCdbClose)(ClDBHandleT dbHandle);
/*****************************************************************************/
typedef ClRcT
(*FPCdbSync)(ClDBHandleT dbHandle,
             ClUint32T   flags);
/*****************************************************************************/
typedef ClRcT
(*FPCdbRecordAdd)(ClDBHandleT      dbHandle,
                  ClDBKeyT         dbKey,
                  ClUint32T       keySize,
                  ClDBRecordT      dbRec,
                  ClUint32T       recSize);
/*****************************************************************************/
typedef ClRcT
(*FPCdbRecordReplace)(ClDBHandleT      dbHandle,
                      ClDBKeyT         dbKey,
                      ClUint32T       keySize,
                      ClDBRecordT      dbRec,
                      ClUint32T       recSize);
/*****************************************************************************/
typedef ClRcT
(*FPCdbRecordGet)(ClDBHandleT      dbHandle,
                  ClDBKeyT         dbKey,
                  ClUint32T       keySize,
                  ClDBRecordT*     pDBRec,
                  ClUint32T*      pRecSize);
/*****************************************************************************/
typedef ClRcT
(*FPCdbRecordDelete)(ClDBHandleT      dbHandle,
                     ClDBKeyT         dbKey,
                     ClUint32T       keySize);
/*****************************************************************************/
typedef ClRcT
(*FPCdbFirstRecordGet)(ClDBHandleT      dbHandle,
                       ClDBKeyT*        pDBKey,
                       ClUint32T*      pKeySize,
                       ClDBRecordT*     pDBRec,
                       ClUint32T*      pRecSize);
/*****************************************************************************/
typedef ClRcT
(*FPCdbNextRecordGet)(ClDBHandleT      dbHandle,
                      ClDBKeyT         currentKey,
                      ClUint32T       currentKeySize,
                      ClDBKeyT*        pDBNextKey,
                      ClUint32T*      pNextKeySize,
                      ClDBRecordT*     pDBNextRec,
                      ClUint32T*      pNextRecSize);
/*****************************************************************************/
typedef ClRcT  
(*FPCdbTxnOpen)(ClDBFileT    dbFile,
                ClDBNameT    dbName, 
                ClDBFlagT    dbFlag,
                ClUint32T   maxKeySize,
                ClUint32T   maxRecordSize,
                ClDBHandleT* pDBHandle);
/*****************************************************************************/
typedef ClRcT
(*FPCdbTransactionBegin)(ClDBHandleT  dbHandle);
/*****************************************************************************/
typedef ClRcT
(*FPCdbTransactionCommit)(ClDBHandleT  dbHandle);
/*****************************************************************************/
typedef ClRcT
(*FPCdbTransactionAbort)(ClDBHandleT  dbHandle);
/*****************************************************************************/
typedef ClRcT
(*FPCdbRecordFree)(ClDBHandleT  dbHandle,
                   ClDBRecordT  dbRec);
/*****************************************************************************/
typedef ClRcT
(*FPCdbKeyFree)(ClDBHandleT  dbHandle,
                ClDBKeyT     dbKey);
/*****************************************************************************/
typedef ClRcT
(*FPCdbFinalize)();
/*****************************************************************************/
typedef struct ClDbDatabase{
  ClUint32T             validDatabase;
  FPCdbOpen              fpCdbOpen;
  FPCdbClose             fpCdbClose;
  FPCdbSync              fpCdbSync;
  FPCdbRecordAdd         fpCdbRecordAdd;
  FPCdbRecordReplace     fpCdbRecordReplace;
  FPCdbRecordGet         fpCdbRecordGet;
  FPCdbRecordDelete      fpCdbRecordDelete;
  FPCdbFirstRecordGet    fpCdbFirstRecordGet;
  FPCdbNextRecordGet     fpCdbNextRecordGet;
  FPCdbTxnOpen           fpCdbTxnOpen;
  FPCdbTransactionBegin  fpCdbTransactionBegin;
  FPCdbTransactionCommit fpCdbTransactionCommit;
  FPCdbTransactionAbort  fpCdbTransactionAbort;
  FPCdbRecordFree        fpCdbRecordFree;
  FPCdbKeyFree           fpCdbKeyFree;
  FPCdbFinalize          fpCdbFinalize;
} ClDbalFunctionPtrsT;


# ifdef __cplusplus
}
# endif

#endif /* __INC_DBAL_INTERNAL_H__ */
