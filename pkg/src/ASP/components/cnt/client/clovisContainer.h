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
 * ModuleName  : cnt
 * File        : clovisContainer.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the Container library        
 * implementation.                                                           
 *                                                                           
 *****************************************************************************/
#ifndef _CLOVIS_CONTAINER_H_
#define _CLOVIS_CONTAINER_H_

# ifdef __cplusplus
extern "C"
{
# endif

#define CONTAINER_ID 0xC10715
#define NODE_ID 0x51701C

#define  nullChkRet(ptr)   clDbgIfNullReturn(ptr, CL_CID_CNT)

#define  returnCntError(clErr, errStr)                        \
 do                                                               \
 {                                                                \
     if (clErr != CL_ERR_NOT_EXIST) /* Non-existent node is an expected result */ \
       CL_DEBUG_PRINT(CL_DEBUG_WARN, ("[%s]: return code [0x%x]: str: [%s]",      \
                 __FUNCTION__, CL_RC(CL_CID_CNT, clErr), errStr));        \
     if (clErr == CL_ERR_INVALID_HANDLE) \
       clDbgCodeError(clErr,(errStr)); /* Invalid handle must be a code-style error */\
     return CL_RC(CL_CID_CNT, clErr);                             \
 }while(0)

#define  CL_CNT_LOCK(mutex)                                 \
do                                                          \
{                                                           \
    if( CL_HANDLE_INVALID_VALUE != mutex )                  \
    {                                                       \
        ClRcT  errorCode = CL_OK;                           \
        errorCode = clOsalMutexLock(mutex);                 \
        if( CL_OK != errorCode )                            \
        {                                                    \
            returnCntError(errorCode, "Mutex lock failed");  \
        }                                                    \
    }                                                        \
}while(0)                                                     

#define  CL_CNT_UNLOCK(mutex)                                 \
do                                                          \
{                                                           \
    if( CL_HANDLE_INVALID_VALUE != mutex )                  \
    {                                                       \
        ClRcT  errorCode = CL_OK;                           \
        errorCode = clOsalMutexUnlock(mutex);                 \
        if( CL_OK != errorCode )                            \
        {                                                    \
            returnCntError(errorCode, "Mutex unlock failed");  \
        }                                                    \
    }                                                        \
}while(0)                                                     

#define  CL_CNT_DESTROY(mutex)                                 \
do                                                          \
{                                                           \
    if( CL_HANDLE_INVALID_VALUE != mutex )                  \
    {                                                       \
        ClRcT  errorCode = CL_OK;                           \
        errorCode = clOsalMutexDelete(mutex);                 \
        if( CL_OK != errorCode )                            \
        {                                                    \
            returnCntError(errorCode, "Mutex destroy failed");  \
        }                                                    \
    }                                                        \
}while(0)                                                     

/*******************************************************/
typedef ClRcT
(*fpCclContainerNodeAdd) (ClCntHandleT containerHandle,
                          ClCntKeyHandleT userKey,
                          ClCntDataHandleT userData,
                          ClRuleExprT* pExp);
/*******************************************************/
typedef ClRcT
(*fpCclContainerKeyDelete) (ClCntHandleT containerHandle,
                            ClCntKeyHandleT userKey);
/*******************************************************/
typedef ClRcT
(*fpCclContainerNodeDelete) (ClCntHandleT containerHandle,
                             ClCntNodeHandleT nodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerNodeFind) (ClCntHandleT containerHandle,
                           ClCntKeyHandleT userKey,
                           ClCntNodeHandleT* pNodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerFirstNodeGet) (ClCntHandleT containerHandle,
                               ClCntNodeHandleT* pNodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerLastNodeGet) (ClCntHandleT containerHandle,
                              ClCntNodeHandleT* pNodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerNextNodeGet) (ClCntHandleT containerHandle,
                              ClCntNodeHandleT currentNodeHandle,
                              ClCntNodeHandleT* pNextNodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerPreviousNodeGet) (ClCntHandleT containerHandle,
                                  ClCntNodeHandleT currentNodeHandle,
                                  ClCntNodeHandleT* pPreviousNodeHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerSizeGet) (ClCntHandleT containerHandle,
                          ClUint32T* pSize);
/*******************************************************/
typedef ClRcT
(*fpCclContainerKeySizeGet) (ClCntHandleT containerHandle,
                             ClCntKeyHandleT userKey,
                             ClUint32T* pSize);
/*******************************************************/
typedef ClRcT
(*fpCclContainerDestroy) (ClCntHandleT containerHandle);
/*******************************************************/
typedef ClRcT
(*fpCclContainerUserKeyGet)(ClCntHandleT containerHandle,
                            ClCntNodeHandleT nodeHandle, 
                            ClCntKeyHandleT* pUserKey);
/*******************************************************/						   
typedef ClRcT
(*fpCclContainerUserDataGet)(ClCntHandleT containerHandle,
                             ClCntNodeHandleT nodeHandle, 
                             ClCntDataHandleT* pUserDataHandle);
/*******************************************************/
#if 0
/* HA-aware container Modifications Start: Hari*/
typedef ClAddrT
(*fpCclContainerPack)(ClCntHandleT containerHandle,
                      ClAddrT            bufferStart,
                      charPtrFuncPtr_t     serialiser);
/*******************************************************/                       
typedef ClAddrT
(*fpCclContainerUnPack)(ClCntHandleT containerHandle,
                        ClAddrT            bufferStart,
                        charPtrFuncPtr_t     deSerialiser);
/* HA-aware container Modifications End: Hari*/
#endif
/*******************************************************/                       

ClRcT
cclBaseLinkedListCreate (ClCntHandleT* pContainerHandle);
/*******************************************************/
ClRcT
cclBaseLinkedListContainerNodeAdd (ClCntHandleT containerHandle,
                                   ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData,
                                   ClCntNodeHandleT nodeHandle,
                                   ClRuleExprT* pExp);
/*******************************************************/							   
typedef struct CclContainer_t {
  ClUint32T                       validContainer;
#if 0
  /* HA-aware container Modifications Start: Hari*/
  ClAddrT                        cpsHdl;
  ClUint32T                       haEnabled;
  ClUint32T                       domainId;
  ClUint32T                       containerId;

  fpCclContainerPack               fpFunctionContainerPack;
  fpCclContainerUnPack             fpFunctionContainerUnPack;
  /* HA-aware container Modifications End: Hari*/
#endif
  ClUint32T                       isUniqueFlag;
  fpCclContainerNodeAdd            fpFunctionContainerNodeAdd;
  fpCclContainerKeyDelete          fpFunctionContainerKeyDelete;
  fpCclContainerNodeDelete         fpFunctionContainerNodeDelete;
  fpCclContainerNodeFind           fpFunctionContainerNodeFind;
  fpCclContainerFirstNodeGet       fpFunctionContainerFirstNodeGet;
  fpCclContainerLastNodeGet        fpFunctionContainerLastNodeGet;
  fpCclContainerNextNodeGet        fpFunctionContainerNextNodeGet;
  fpCclContainerPreviousNodeGet    fpFunctionContainerPreviousNodeGet;
  fpCclContainerSizeGet            fpFunctionContainerSizeGet;
  fpCclContainerKeySizeGet         fpFunctionContainerKeySizeGet;
  fpCclContainerUserKeyGet         fpFunctionContainerUserKeyGet;
  fpCclContainerUserDataGet        fpFunctionContainerUserDataGet;
  fpCclContainerDestroy            fpFunctionContainerDestroy;
  ClCntKeyCompareCallbackT         fpFunctionContainerKeyCompare;
  ClCntDeleteCallbackT              fpFunctionContainerDeleteCallBack;
  ClCntDeleteCallbackT              fpFunctionContainerDestroyCallBack;
} CclContainer_t;
/*******************************************************/
#ifdef ORDERED_LIST
extern ClRcT
cclBaseOrderedLinkedListContainerNodeAdd (ClCntHandleT containerHandle,
                                   ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData,
                                   ClCntNodeHandleT nodeHandle,
                                   ClRuleExprT* pExp,
                                   ClCntKeyCompareCallbackT keyCompareFunc);
#endif

/*******************************************************/							   

# ifdef __cplusplus
}
# endif

#endif /* _CLOVIS_CONTAINER_H_ */
