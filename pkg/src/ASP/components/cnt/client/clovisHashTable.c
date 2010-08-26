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
 * File        : clovisHashTable.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains a simple Hash Table implementation. It will be         
 * extended to make it "very" generic later.                                 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clCntApi.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clRuleApi.h>
#include "clovisContainer.h"
#include "clovisBaseLinkedList.h"
#include "clovisLinkedList.h"
#if HA_ENABLED
#include "cps_main.h"
#endif 
#include <clDebugApi.h>
/*#include <containerCompId.h>*/
/*****************************************************************************/

typedef struct HashTable_t 
{
  CclContainer_t      container;
  ClUint32T           numberOfBuckets;
  ClPtrT*             pHashBuckets; /* allocate this when you create the hash table */
  ClCntHashCallbackT  fpFunctionContainerHash;
  ClOsalMutexIdT      hashMutex;
}HashTable_t;

/*****************************************************************************/
/*                                  PROTO TYPES                              */
/*****************************************************************************/
static ClRcT 
cclHashTableContainerDestroy (ClCntHandleT containerHandle);

static ClRcT 
cclHashTableContainerNodeAdd (ClCntHandleT containerHandle, 
                              ClCntKeyHandleT userKey, 
                              ClCntDataHandleT userData,
                              ClRuleExprT* pExp);

static ClRcT 
cclHashTableContainerNodeFind (ClCntHandleT containerHandle, 
                               ClCntKeyHandleT userKey,
                               ClCntNodeHandleT* pNodeHandle);

static ClRcT
cclHashTableContainerKeyDelete (ClCntHandleT containerHandle, 
                                ClCntKeyHandleT userKey);
static ClRcT
cclHashTableContainerNodeDelete (ClCntHandleT containerHandle,
                                 ClCntNodeHandleT nodeHandle);
static ClRcT
cclHashTableContainerFirstNodeGet (ClCntHandleT containerHandle,
                                   ClCntNodeHandleT* pNodeHandle);

static ClRcT
cclHashTableContainerLastNodeGet (ClCntHandleT containerHandle,
                                  ClCntNodeHandleT* pNodeHandle);

static ClRcT
cclHashTableContainerNextNodeGet (ClCntHandleT containerHandle,
                                  ClCntNodeHandleT nodeHandle,
                                  ClCntNodeHandleT* pNextNodeHandle); 

static ClRcT
cclHashTableContainerPreviousNodeGet (ClCntHandleT containerHandle,
                                      ClCntNodeHandleT nodeHandle,
                                      ClCntNodeHandleT* pPreviousNodeHandle); 
static ClRcT
cclHashTableContainerSizeGet (ClCntHandleT containerHandle,
                              ClUint32T* pSize);
static ClRcT
cclHashTableContainerKeySizeGet (ClCntHandleT containerHandle,
                                 ClCntKeyHandleT userKey,
                                 ClUint32T* pSize);							 
static ClRcT
cclHashTableContainerUserKeyGet(ClCntHandleT containerHandle,
                                ClCntNodeHandleT nodeHandle,
                                ClCntKeyHandleT* pUserKey);							  
static ClRcT
cclHashTableContainerUserDataGet(ClCntHandleT containerHandle,
                                 ClCntNodeHandleT nodeHandle,
                                 ClCntDataHandleT *pUserDataHandle);
/****************************************************************************/


/* HA-aware container Modifications start : Hari*/
#if HA_ENABLED
ClRcT
cclHAHashTableCreate (ClUint32T               numberOfBuckets,
                      ClCntKeyCompareCallbackT fpKeyCompare,
                      ClCntHashCallbackT        fpHashFunction,
                      ClCntDeleteCallbackT      fpUserDeleteCallback,
                      ClCntDeleteCallbackT      fpUserDestroyCallback,
                      haInfo_t                 *pHaInfo,
                      ClCntHandleT     *pContainerHdl)
{
  ClRcT            retCode;
  HashTable_t       *pHashTable;

  /*Create the container */
  if (CL_OK != (retCode = clCntHashtblCreate (numberOfBuckets,
                                                 fpKeyCompare,
                                                 fpHashFunction,
                                                 fpUserDeleteCallback,
                                                 fpUserDestroyCallback,
                                                 CL_CNT_NON_UNIQUE_KEY,
                                                 pContainerHdl)))
  {
     /* Some thing went wrong!!*/
     return retCode;
  }
  pHashTable= (HashTable_t*)(*pContainerHdl);
  pHashTable->container.haEnabled = TRUE;
  pHashTable->container.cpsHdl = pHaInfo->cpsHdl;
  pHashTable->container.containerId = pHaInfo->containerId;
  /* Let CPS know of this container */
  return clCPContainerReg(pHaInfo->cpsHdl,
                          (ClCntHandleT)pContainerHdl,
                          pHaInfo->domainId,
                          pHaInfo->containerId,
                          pHaInfo->frequency,
                          pHaInfo->order,
                          pHaInfo->autoCPOff,
                          pHaInfo->serialiser,
                          pHaInfo->deSerialiser);
}
#endif 
/* HA-aware container Modifications End: Hari*/
/****************************************************************************/
ClRcT 
clCntThreadSafeHashtblCreate (ClUint32T                 numberOfBuckets,
                              ClCntKeyCompareCallbackT  fpKeyCompare,
                              ClCntHashCallbackT        fpHashFunction,
                              ClCntDeleteCallbackT      fpUserDeleteCallback,
                              ClCntDeleteCallbackT      fpUserDestroyCallback,
					          ClCntKeyTypeT             containerKeyType,
                              ClCntHandleT              *pContainerHandle)
{
    ClRcT       errorCode   = CL_OK;
    HashTable_t *pHashTable = NULL;

    errorCode = clCntHashtblCreate(numberOfBuckets, fpKeyCompare, fpHashFunction, 
                            fpUserDestroyCallback, fpUserDestroyCallback, 
                            containerKeyType, pContainerHandle);
    if( CL_OK != errorCode )
    {
        return errorCode;
    }
    pHashTable = (HashTable_t *) *pContainerHandle;
    errorCode = clOsalMutexCreate(&pHashTable->hashMutex);
    if( CL_OK != errorCode )
    {
        pHashTable->hashMutex = CL_HANDLE_INVALID_VALUE;
        clCntDelete(*pContainerHandle);
        returnCntError(errorCode, "Mutex creation failed");
    }

    return CL_OK;
}
    
ClRcT 
clCntHashtblCreate (ClUint32T                 numberOfBuckets,
                    ClCntKeyCompareCallbackT  fpKeyCompare,
                    ClCntHashCallbackT        fpHashFunction,
                    ClCntDeleteCallbackT      fpUserDeleteCallback,
                    ClCntDeleteCallbackT      fpUserDestroyCallback,
					ClCntKeyTypeT             containerKeyType,
                    ClCntHandleT              *pContainerHandle)
{
  HashTable_t   *pHashTable     = NULL;
  ClCntHandleT  containerHandle = CL_HANDLE_INVALID_VALUE;
  ClInt32T      hashKey         = 0;
  ClRcT         errorCode       = CL_OK;

  /* Validate All necessary parameters against NULL */
  nullChkRet(fpKeyCompare);
  nullChkRet(fpHashFunction);
  nullChkRet(pContainerHandle);
  /* Validate the other parameters */
  if(containerKeyType >= CL_CNT_INVALID_KEY_TYPE)
  {
      returnCntError(CL_ERR_INVALID_PARAMETER, 
                     "Container type is not proper");
  }
  if( 0 == numberOfBuckets )
  {
      returnCntError(CL_ERR_INVALID_PARAMETER, 
                      "Num of buckets passed is Zero");
  }
  /* Allocate the Entries table with proper Initialized vaibles */
  pHashTable = (HashTable_t*) clHeapCalloc (1, (ClUint32T)sizeof (HashTable_t));
  if (pHashTable == NULL)
  {
      returnCntError(CL_ERR_NULL_POINTER, "Memory allocation failed");
  }
  pHashTable->numberOfBuckets = numberOfBuckets;
  pHashTable->hashMutex       = CL_HANDLE_INVALID_VALUE;

  /* Allocate handles for all the buckets */
  pHashTable->pHashBuckets = (void**) clHeapCalloc (numberOfBuckets, sizeof (*pHashTable->pHashBuckets));
  if(pHashTable->pHashBuckets == NULL) 
  {
    clHeapFree (pHashTable);
    returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }

  /* Link hash table with containers of linked list */
  for (hashKey = 0; hashKey < (ClInt32T)numberOfBuckets; hashKey++)
  {
      errorCode = clCntLlistCreate(fpKeyCompare, fpUserDeleteCallback,
                                   fpUserDestroyCallback, containerKeyType,
                                   &containerHandle);
      if( CL_OK != errorCode )
      {
          hashKey--;
          for (; hashKey >= 0; hashKey--)
          {
              /* not checking the error code, its cleanup path */
              clCntDelete(pHashTable->pHashBuckets[hashKey]); 
          }
          clHeapFree(pHashTable->pHashBuckets);
          clHeapFree(pHashTable);
          returnCntError(errorCode, "Hash table creation failed");
      }
      pHashTable->pHashBuckets[hashKey] = containerHandle;
  }
  /* Add container functions */
  //pHashTable->container.type            = (ClTypeT *) CL_CNT_HASH_TABLE;
  pHashTable->container.fpFunctionContainerNodeAdd = 
    cclHashTableContainerNodeAdd; 
  pHashTable->container.fpFunctionContainerKeyDelete = 
    cclHashTableContainerKeyDelete;
  pHashTable->container.fpFunctionContainerNodeDelete =
    cclHashTableContainerNodeDelete;
  pHashTable->container.fpFunctionContainerNodeFind = 
    cclHashTableContainerNodeFind;
  pHashTable->container.fpFunctionContainerFirstNodeGet =  
    cclHashTableContainerFirstNodeGet;
  pHashTable->container.fpFunctionContainerLastNodeGet = 
    cclHashTableContainerLastNodeGet;
  pHashTable->container.fpFunctionContainerNextNodeGet =
    cclHashTableContainerNextNodeGet; 
  pHashTable->container.fpFunctionContainerPreviousNodeGet = 
    cclHashTableContainerPreviousNodeGet;
  pHashTable->container.fpFunctionContainerSizeGet = 
    cclHashTableContainerSizeGet;
  pHashTable->container.fpFunctionContainerKeySizeGet = 
    cclHashTableContainerKeySizeGet;
  pHashTable->container.fpFunctionContainerUserKeyGet = 
    cclHashTableContainerUserKeyGet;
  pHashTable->container.fpFunctionContainerUserDataGet = 
    cclHashTableContainerUserDataGet;
  pHashTable->container.fpFunctionContainerKeyCompare =
    fpKeyCompare;
  pHashTable->container.fpFunctionContainerDestroy = 
    cclHashTableContainerDestroy;
  pHashTable->fpFunctionContainerHash = 
    fpHashFunction;	
  pHashTable->container.fpFunctionContainerDeleteCallBack =
    fpUserDeleteCallback;
  pHashTable->container.fpFunctionContainerDestroyCallBack =
    fpUserDestroyCallback;
  if( CL_CNT_UNIQUE_KEY == containerKeyType )
  {
    pHashTable->container.isUniqueFlag = 1;
  }
  else
  {
    pHashTable->container.isUniqueFlag = 0;
  }
	
  pHashTable->container.validContainer = CONTAINER_ID;

  *pContainerHandle = (ClCntHandleT) pHashTable;
  return (CL_OK);
}

/***************************************************************************/
static ClRcT 
cclHashTableContainerDestroy (ClCntHandleT containerHandle)
{
  ClInt32T        hashKey     = 0;
  HashTable_t     *pHashTable = NULL;
  ClOsalMutexIdT  hashMutex   = CL_HANDLE_INVALID_VALUE;

  /* Handle validations */
  pHashTable = (HashTable_t*) containerHandle;
	
  CL_CNT_LOCK(pHashTable->hashMutex);
  hashMutex = pHashTable->hashMutex;
  /* Destroy all the buckets */
  for (hashKey = 0; hashKey < (ClInt32T)pHashTable->numberOfBuckets; hashKey++) 
  {
    /* Just ignore the error message,  proceed the work */
    clCntDelete(pHashTable->pHashBuckets[hashKey]); 
  }
  clHeapFree (pHashTable->pHashBuckets); 
  clHeapFree (pHashTable); 
  /* Unlock the copied mutex */
  CL_CNT_UNLOCK(hashMutex);
  /* Free the mutex */
  CL_CNT_DESTROY(hashMutex);

  return (CL_OK);
}
/****************************************************************************/
static ClRcT 
cclHashTableContainerNodeAdd (ClCntHandleT     containerHandle, 
                              ClCntKeyHandleT  userKey, 
                              ClCntDataHandleT userData,
                              ClRuleExprT      *pExp)
{
  ClInt32T     hashKey     = 0;
  HashTable_t  *pHashTable = NULL;
  ClRcT        errorCode   = CL_OK;

  pHashTable = (HashTable_t*) containerHandle;

  CL_CNT_LOCK(pHashTable->hashMutex);
	
  hashKey = pHashTable->fpFunctionContainerHash(userKey); 
  if( hashKey < 0 )
  {
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      returnCntError(CL_ERR_INVALID_PARAMETER, "Hash key is not proper");
  }
  errorCode = (clCntNodeAdd (pHashTable->pHashBuckets[hashKey], 
                               userKey, 
                               userData,
                               pExp));
  CL_CNT_UNLOCK(pHashTable->hashMutex);

  return errorCode;
}
/****************************************************************************/
static ClRcT 
cclHashTableContainerNodeFind(ClCntHandleT     containerHandle, 
                              ClCntKeyHandleT  userKey,
                              ClCntNodeHandleT *pNodeHandle)
{
    ClInt32T         hashKey        = -1;
    HashTable_t      *pHashTable    = NULL;
    ClCntNodeHandleT tempNodeHandle = CL_HANDLE_INVALID_VALUE;
    ClRcT            errorCode      = CL_OK;

    /* Handle validations */
    pHashTable = (HashTable_t*) containerHandle;

    CL_CNT_LOCK(pHashTable->hashMutex);

    /* Get the hash key */
    hashKey = pHashTable->fpFunctionContainerHash(userKey); 
    if( hashKey < 0 )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(CL_ERR_INVALID_PARAMETER, "Hash Key is not proper");
    }
    /* Find the node */
    errorCode = clCntNodeFind(pHashTable->pHashBuckets[hashKey],
            userKey, &tempNodeHandle);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        /* NodeFind is used for "existence" testing, so this is a "normal" error
          returnCntError(errorCode, "Hash Node doesn't exist"); */
        return errorCode;        
    }
    
    *pNodeHandle = tempNodeHandle;
    CL_CNT_UNLOCK(pHashTable->hashMutex);
    return (CL_OK);
}
/****************************************************************************/
static ClRcT
cclHashTableContainerKeyDelete(ClCntHandleT    containerHandle,
                               ClCntKeyHandleT userKey)
{
  ClInt32T     hashKey     = -1;
  HashTable_t  *pHashTable = NULL;
  ClRcT        errorCode   = CL_OK;
    
  /* Validations */
  pHashTable = (HashTable_t*) containerHandle;
	
  CL_CNT_LOCK(pHashTable->hashMutex);
  /* Find the key */
  hashKey = pHashTable->fpFunctionContainerHash(userKey); 
  if( hashKey < 0 )
  {
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      returnCntError(CL_ERR_INVALID_PARAMETER, "Hash Key is not proper");
  }

  errorCode =  clCntAllNodesForKeyDelete (pHashTable->pHashBuckets[hashKey],
                                 userKey);
  CL_CNT_UNLOCK(pHashTable->hashMutex);

  return errorCode;
}
/****************************************************************************/
static ClRcT
cclHashTableContainerFirstNodeGet (ClCntHandleT      containerHandle,
                                   ClCntNodeHandleT  *pNodeHandle)
{
  ClUint32T        hashKey     = 0;
  HashTable_t      *pHashTable = NULL;
  ClCntNodeHandleT cclNode     = CL_HANDLE_INVALID_VALUE;
  ClRcT            errorCode   = CL_OK;

  /* Hash table validations */
  pHashTable = (HashTable_t*) containerHandle;
	
  CL_CNT_LOCK(pHashTable->hashMutex);
  /* Go thru the hash key and get the first node */
  for (hashKey = 0; hashKey < pHashTable->numberOfBuckets; hashKey++)
  {
    errorCode = clCntFirstNodeGet (pHashTable->pHashBuckets[hashKey], &cclNode);
    if (errorCode == CL_OK) 
    {
      *pNodeHandle = cclNode;
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      return (CL_OK);
    }
  }

  *pNodeHandle = (ClCntNodeHandleT)NULL;
  CL_CNT_UNLOCK(pHashTable->hashMutex);
  return (errorCode);
}
/****************************************************************************/
static ClRcT
cclHashTableContainerLastNodeGet (ClCntHandleT      containerHandle,
                                  ClCntNodeHandleT  *pNodeHandle)
{
  ClInt32T         hashKey     = 0;
  ClCntNodeHandleT cclNode     = CL_HANDLE_INVALID_VALUE;
  HashTable_t      *pHashTable = NULL;
  ClRcT            errorCode   = CL_OK;

  /* Hash table validations */
  pHashTable = (HashTable_t*)containerHandle;
	
  CL_CNT_LOCK(pHashTable->hashMutex);
  for (hashKey = pHashTable->numberOfBuckets - 1; hashKey >= 0; hashKey--)
  {
    errorCode = clCntLastNodeGet (pHashTable->pHashBuckets[hashKey], &cclNode);
    if (errorCode == CL_OK)
    {
      *pNodeHandle = cclNode;
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      return (CL_OK);
    }
  }

  *pNodeHandle = (ClCntNodeHandleT)NULL;
  CL_CNT_UNLOCK(pHashTable->hashMutex);
  return (errorCode);
}
/****************************************************************************/
static ClRcT
cclHashTableContainerNextNodeGet (ClCntHandleT      containerHandle, 
                                  ClCntNodeHandleT  nodeHandle,
                                  ClCntNodeHandleT  *pNextNodeHandle) 
{
  ClInt32T             hashKey     = -1;
  HashTable_t          *pHashTable = NULL;
  ClCntKeyHandleT      userKey     = CL_HANDLE_INVALID_VALUE;
  ClCntNodeHandleT     nextNode    = CL_HANDLE_INVALID_VALUE;
  BaseLinkedListNode_t *pBLlNode   = NULL;
  ClRcT                errorCode   = CL_OK;

  /* Hash handle validations */
  *pNextNodeHandle = CL_HANDLE_INVALID_VALUE;
  pHashTable       = (HashTable_t*)containerHandle;
  
  /* Node handle validations */
  pBLlNode = (BaseLinkedListNode_t *) nodeHandle;
  nullChkRet(pBLlNode);
  if( NODE_ID != pBLlNode->validNode ) 
  {
      returnCntError(CL_ERR_INVALID_HANDLE,
              "The passed node handle is invalid");
  }

  CL_CNT_LOCK(pHashTable->hashMutex);
  /*Getting the corresponding key */
  errorCode = clCntNodeUserKeyGet(containerHandle, nodeHandle, &userKey);
  if( errorCode != CL_OK)
  { 
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      returnCntError(errorCode, "Node doesn't exist");
  }

  /* Get the bucket */
  hashKey = pHashTable->fpFunctionContainerHash(userKey); 
  if( hashKey < 0 )
  {
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      returnCntError(CL_ERR_INVALID_PARAMETER, "Hash key is not proper");
  }
  errorCode = clCntNextNodeGet (pHashTable->pHashBuckets[hashKey], 
                                nodeHandle, &nextNode);
  if( CL_OK != errorCode )
  {
      /* Go thru all the buckets, starting from this bucket */
      hashKey++;
      for (; hashKey < (ClInt32T)pHashTable->numberOfBuckets; hashKey++) 
      {
          errorCode = clCntFirstNodeGet (pHashTable->pHashBuckets[hashKey], &nextNode);
          if (errorCode == CL_OK) 
          {
            *pNextNodeHandle = nextNode;
             CL_CNT_UNLOCK(pHashTable->hashMutex);
             return(CL_OK);
          }	
      }  
      CL_CNT_UNLOCK(pHashTable->hashMutex);
      returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
  }

  *pNextNodeHandle = nextNode;
  CL_CNT_UNLOCK(pHashTable->hashMutex);
  return(CL_OK);
}
/****************************************************************************/
static ClRcT
cclHashTableContainerPreviousNodeGet(ClCntHandleT      containerHandle,
                                     ClCntNodeHandleT  nodeHandle,
                                     ClCntNodeHandleT  *pPreviousNodeHandle) 
{
    ClInt32T             hashKey     = -1; 
    HashTable_t          *pHashTable = NULL;
    ClCntKeyHandleT      userKey     = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT     prevNode    = CL_HANDLE_INVALID_VALUE;
    BaseLinkedListNode_t *pBLlNode   = NULL;
    ClRcT                errorCode   = CL_OK;

    /* Hash handle validations */
    pHashTable = (HashTable_t*) containerHandle;

    /* Node handle validattions */
    pBLlNode = (BaseLinkedListNode_t *) nodeHandle;
    nullChkRet(pBLlNode);
  if( NODE_ID != pBLlNode->validNode ) 
  {
      returnCntError(CL_ERR_INVALID_HANDLE,
              "The passed node handle is invalid");
  }

    CL_CNT_LOCK(pHashTable->hashMutex);
    /* Get the corressponding key for this node */
    errorCode = clCntNodeUserKeyGet(containerHandle, nodeHandle, &userKey);
    if( errorCode != CL_OK)
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(errorCode, "Key doesn't exist for this node");
    }

    /* Get the bucket for the key */
    hashKey = pHashTable->fpFunctionContainerHash(userKey); 
    if( hashKey < 0 )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(CL_ERR_INVALID_PARAMETER, "Hash key is not proper");
    }

    errorCode = clCntPreviousNodeGet(pHashTable->pHashBuckets[hashKey], 
                                     nodeHandle, &prevNode);
    if( CL_OK != errorCode )
    {
        /* if previous node is not available, traverse in hash table */   
        hashKey--; 
        for(; hashKey >= 0; hashKey--)
        {
            errorCode = clCntLastNodeGet (pHashTable->pHashBuckets[hashKey], &prevNode);
            if (errorCode == CL_OK) 
            {
                *pPreviousNodeHandle = prevNode;
                CL_CNT_UNLOCK(pHashTable->hashMutex);
                return CL_OK;
            }
        }
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(CL_ERR_NOT_EXIST, "The Last node doesn't exist");
    }

    *pPreviousNodeHandle = prevNode;
    CL_CNT_UNLOCK(pHashTable->hashMutex);
    return CL_OK;
}

/****************************************************************************/
static ClRcT
cclHashTableContainerSizeGet (ClCntHandleT containerHandle,
                              ClUint32T    *pSize)
{
  HashTable_t  *pHashTable   = NULL;
  ClUint32T    numberOfNodes = 0, tempNumber = 0;
  ClInt32T     hashKey       = 0;

  /* Hash handle validations */
  pHashTable = (HashTable_t*) containerHandle;
	
  CL_CNT_LOCK(pHashTable->hashMutex);
  /* Walk thru the handle and get the num of nodes */
  for (hashKey = 0; hashKey < (ClInt32T)pHashTable->numberOfBuckets; hashKey++)
  {
    tempNumber = 0;
    /* ignoring the error */
    clCntSizeGet (pHashTable->pHashBuckets[hashKey], &tempNumber);
    numberOfNodes += tempNumber;
  }

  /* return the size */
  *pSize = numberOfNodes;  
  CL_CNT_UNLOCK(pHashTable->hashMutex);
  return (CL_OK); 
}
/***********************************************************************/
static ClRcT
cclHashTableContainerUserKeyGet(ClCntHandleT     containerHandle,
                                ClCntNodeHandleT nodeHandle,
                                ClCntKeyHandleT  *pUserKey)
{
    BaseLinkedListNode_t *pTemp   = NULL;
    LinkedListNode_t     *pLlNode = NULL;

    /* Node handle validations */
    pTemp = (BaseLinkedListNode_t*)nodeHandle;
    nullChkRet(pTemp);
    if( NODE_ID != pTemp->validNode ) 
    {
        returnCntError(CL_ERR_INVALID_HANDLE,
                "The passed node handle is invalid");
    }

    /* Go to the parent and get the key */
    pLlNode = (LinkedListNode_t *) pTemp->parentNodeHandle;
    if( NULL == pLlNode )
    {
        returnCntError(CL_ERR_INVALID_STATE, "Hash table got corrupted");
    }
    *pUserKey = pLlNode->userKey;

    return (CL_OK);
}

/***********************************************************************/
static ClRcT
cclHashTableContainerUserDataGet(ClCntHandleT      containerHandle,
                                 ClCntNodeHandleT  nodeHandle,
                                 ClCntDataHandleT  *pUserDataHandle)
{
    BaseLinkedListNode_t *pTemp = NULL;

    /* Node handle validations */
    pTemp = (BaseLinkedListNode_t*) nodeHandle;
    nullChkRet(pTemp);
    if( NODE_ID != pTemp->validNode ) 
    {
        returnCntError(CL_ERR_INVALID_HANDLE,
                "The passed node handle is invalid");
    }

    /* returning the data */
    *pUserDataHandle = pTemp->userData;

    return (CL_OK);
}

/***********************************************************************/
static ClRcT
cclHashTableContainerNodeDelete (ClCntHandleT     containerHandle,
                                 ClCntNodeHandleT nodeHandle)
{
    HashTable_t           *pHashTable = NULL;
    BaseLinkedListNode_t  *pLlNode    = NULL;
    LinkedListNode_t      *tempLlNode = NULL;
    ClUint32T             hashKey     = 0;
    ClRcT                 errorCode   = CL_OK;

    /* Hash handle validation */
    pHashTable = (HashTable_t*) containerHandle;

    /* Node handle validations */
    pLlNode = (BaseLinkedListNode_t *) nodeHandle;
    nullChkRet(pLlNode);
    if( NODE_ID != pLlNode->validNode ) 
    {
        returnCntError(CL_ERR_INVALID_HANDLE,
                "The passed node handle is invalid");
    }
 
    CL_CNT_LOCK(pHashTable->hashMutex);
    /* Got the parent node */
    tempLlNode = (LinkedListNode_t*) pLlNode->parentNodeHandle;

    /*Get the hash key from hash function */
    hashKey = (ClUint32T)pHashTable->fpFunctionContainerHash(tempLlNode->userKey);
    errorCode = clCntNodeDelete(pHashTable->pHashBuckets[hashKey], nodeHandle);
    CL_CNT_UNLOCK(pHashTable->hashMutex);
    return errorCode;
}

/***********************************************************************/
static ClRcT
cclHashTableContainerKeySizeGet (ClCntHandleT     containerHandle,
                                 ClCntKeyHandleT  userKey,
                                 ClUint32T        *pSize)
{
    ClInt32T             hashKey        = -1 ;
    HashTable_t          *pHashTable    = NULL;
    ClCntNodeHandleT     tempNodeHandle = CL_HANDLE_INVALID_VALUE;
    BaseLinkedListNode_t *pBaseLlNode   = NULL;
    LinkedListNode_t     *pLlNode       = NULL;
    ClRcT                errorCode      = CL_OK;
    ClUint32T            tempSize       = 0;

    /* Do handle validations */
    pHashTable = (HashTable_t*) containerHandle;

    CL_CNT_LOCK(pHashTable->hashMutex);
    /*Get the hash key */
    hashKey = pHashTable->fpFunctionContainerHash(userKey);  
    if( hashKey < 0 )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(CL_ERR_INVALID_PARAMETER, "Hash key is not proper");
    }
    *pSize = 0;

    /* Find the proper node */
    errorCode = clCntNodeFind (pHashTable->pHashBuckets[hashKey],
            userKey, &tempNodeHandle);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(errorCode, "Corresponding node doesn't exist");
    }

    /* Get the size of the node */
    pBaseLlNode         = (BaseLinkedListNode_t *) tempNodeHandle;
    pLlNode             = (LinkedListNode_t *) pBaseLlNode->parentNodeHandle;
    errorCode = clCntSizeGet((ClCntHandleT) pLlNode->containerHandle, &tempSize);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pHashTable->hashMutex);
        returnCntError(errorCode, "Container node doesn't exist");
    }

    *pSize = tempSize;
    CL_CNT_UNLOCK(pHashTable->hashMutex);
    return (CL_OK);
}
/*******************************END*************************************/
