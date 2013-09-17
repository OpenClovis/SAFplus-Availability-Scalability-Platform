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
 * ModuleName  : amf
 * File        : clAmsEntityUserData.c
 *******************************************************************************/

/*******************************************************************************
 * Description : An interface to store opaque user data to entity config used
 * by the ASP GUI interface
 *******************************************************************************/

#include <clAmsEntityUserData.h>

/*
 * We use a hash table and keep it outside entity config as this wont
 * be used in the normal case. So we dont want to touch the normal config
 * structures and increase the memory footprint irrespective of user data usage.
 */
#define CL_AMS_ENTITY_USER_DATA_TABLE_BITS (10)
#define CL_AMS_ENTITY_USER_DATA_TABLE_SIZE ( 1 << CL_AMS_ENTITY_USER_DATA_TABLE_BITS )
#define CL_AMS_ENTITY_USER_DATA_TABLE_MASK ( CL_AMS_ENTITY_USER_DATA_TABLE_SIZE - 1 )

#define CL_AMS_CHECK_USER_DATA_INITIALIZED() do {                       \
    if(gClAmsEntityUserDataInitialized == CL_FALSE) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED); \
}while(0)
    
typedef struct ClAmsEntityUserData
{
#define CL_AMS_ENTITY_USER_DATA_MAX_KEYS (0x40)
    SaNameT name;
    ClCharT *data;
    ClUint32T len;
    struct hashStruct hash;
    ClListHeadT list;
    ClListHeadT keyValueList;
    ClUint32T numKeyValues;
}ClAmsEntityUserDataT;

typedef struct ClAmsEntityUserDataStorage
{
    SaNameT key;
    ClCharT *data;
    ClUint32T len;
    ClListHeadT list;
}ClAmsEntityUserDataStorageT;

static struct hashStruct *gClAmsEntityUserDataTable[CL_AMS_ENTITY_USER_DATA_TABLE_SIZE];
static CL_LIST_HEAD_DECLARE(gClAmsEntityUserDataList);
static ClOsalMutexT gClAmsEntityUserDataMutex;
static ClUint32T numUserDataEntries;
static ClBoolT gClAmsEntityUserDataInitialized = CL_FALSE;

static __inline__ ClUint32T clAmsEntityUserDataHash(SaNameT *name)
{
    ClUint32T hash = 0;
    ClRcT rc = CL_OK;
    CL_ASSERT(name != NULL);
    rc = clCksm32bitCompute((ClUint8T*)name->value, name->length, &hash);
    CL_ASSERT(rc == CL_OK);
    return hash & CL_AMS_ENTITY_USER_DATA_TABLE_MASK;
}

static __inline__ void clAmsEntityUserDataUnlink(ClAmsEntityUserDataT *userData)
{
    hashDel(&userData->hash);
    clListDel(&userData->list);
    --numUserDataEntries;
}

static __inline__ ClRcT clAmsEntityUserDataLink(ClAmsEntityUserDataT *userData)
{
    ClUint32T key = 0;
    if(!userData) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    key = clAmsEntityUserDataHash(&userData->name);
    hashAdd(gClAmsEntityUserDataTable, key, &userData->hash);
    clListAddTail(&userData->list, &gClAmsEntityUserDataList);
    ++numUserDataEntries;
    return CL_OK;
}

static ClRcT clAmsEntityUserDataFree(ClAmsEntityUserDataT *userData)
{
    /*
     * Delete from hash table.
     */
    clAmsEntityUserDataUnlink(userData);
    while(!CL_LIST_HEAD_EMPTY(&userData->keyValueList))
    {
        ClListHeadT *head = userData->keyValueList.pNext;
        ClAmsEntityUserDataStorageT *storage = CL_LIST_ENTRY(head, ClAmsEntityUserDataStorageT, list);
        clListDel(head);
        if(storage->data) clHeapFree(storage->data);
        clHeapFree(storage);
    }
    if(userData->data) clHeapFree(userData->data);
    userData->data = NULL;
    userData->len = 0;
    return CL_OK;
}

static ClAmsEntityUserDataT *clAmsEntityUserDataFind(SaNameT *name)
{
    register struct hashStruct *iter;
    ClUint32T key = clAmsEntityUserDataHash(name);
    for(iter = gClAmsEntityUserDataTable[key]; iter; iter = iter->pNext)
    {
        ClAmsEntityUserDataT *userData = hashEntry(iter, ClAmsEntityUserDataT, hash);
        if(userData->name.length != name->length) continue;
        if(!memcmp(userData->name.value, name->value, userData->name.length))
            return userData;
    }
    return NULL;
}

ClRcT _clAmsEntityUserDataSet(SaNameT *entity, ClCharT *data, ClUint32T len )
{
    ClAmsEntityUserDataT *userData = NULL;

    CL_AMS_CHECK_USER_DATA_INITIALIZED();

    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);

    userData = clAmsEntityUserDataFind(entity);

    if(userData)
    {
        if(userData->data) clHeapFree(userData->data);
        userData->data = data;
        userData->len = len;
    }
    else
    {
        userData = clHeapCalloc(1, sizeof(*userData));
        CL_ASSERT(userData != NULL);
        CL_LIST_HEAD_INIT(&userData->keyValueList);
        userData->data = data;
        userData->len =  len;
        memcpy(&userData->name, entity, sizeof(*entity));
        clAmsEntityUserDataLink(userData);
    }
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);

    return CL_OK;
}

static ClAmsEntityUserDataStorageT *clAmsEntityUserDataFindKey(ClAmsEntityUserDataT *userData,
                                                               SaNameT *key)
{
    ClListHeadT *iter = NULL;
    CL_LIST_FOR_EACH(iter, &userData->keyValueList)
    {
        ClAmsEntityUserDataStorageT *userDataStorage = CL_LIST_ENTRY(iter, ClAmsEntityUserDataStorageT, list);
        if(userDataStorage->key.length != key->length) continue;
        if(!memcmp(userDataStorage->key.value, key->value,
                   userDataStorage->key.length))
            return userDataStorage;
    }
    return NULL;
}
                                                               
ClRcT _clAmsEntityUserDataSetKey(SaNameT *entity, SaNameT *key, ClCharT *data, ClUint32T len)
{
    ClRcT rc = CL_OK;

    ClAmsEntityUserDataT *userData = NULL;
    ClAmsEntityUserDataStorageT *userDataStorage = NULL;

    CL_AMS_CHECK_USER_DATA_INITIALIZED();

    if(!entity || !key) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);

    userData = clAmsEntityUserDataFind(entity);
    if(userData)
    {
        if((userDataStorage = clAmsEntityUserDataFindKey(userData, key)))
        {
            if(userDataStorage->data) clHeapFree(userDataStorage->data);
            userDataStorage->data = data;
            userDataStorage->len = len;
            goto out_unlock;
        }
    }
    else
    {
        userData = clHeapCalloc(1, sizeof(*userData));
        CL_ASSERT(userData != NULL);
        memcpy(&userData->name, entity, sizeof(userData->name));
        CL_LIST_HEAD_INIT(&userData->keyValueList);
        clAmsEntityUserDataLink(userData);
    }

    if(userData->numKeyValues >= CL_AMS_ENTITY_USER_DATA_MAX_KEYS)
    {
        rc = CL_AMS_RC(CL_ERR_NO_SPACE);
        goto out_unlock;
    }

    userDataStorage = clHeapCalloc(1, sizeof(*userDataStorage));
    CL_ASSERT(userDataStorage != NULL);
    userDataStorage->data = data;
    userDataStorage->len = len;
    memcpy(&userDataStorage->key, key, sizeof(userDataStorage->key));
    clListAddTail(&userDataStorage->list, &userData->keyValueList);
    ++userData->numKeyValues;

    out_unlock:
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);

    return rc;
}

ClRcT _clAmsEntityUserDataGet(SaNameT *entity,
                              ClCharT **data,
                              ClUint32T *length)
{
    ClRcT rc = CL_OK;
    ClAmsEntityUserDataT *userData = NULL;
    
    CL_AMS_CHECK_USER_DATA_INITIALIZED();
    
    if(!entity || !data || !length) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    userData = clAmsEntityUserDataFind(entity);
    if(userData)
    {
        *data = NULL;
        if(userData->len)
        {
            *data = clHeapCalloc(1, userData->len);
            CL_ASSERT(*data != NULL);
            memcpy(*data, userData->data, userData->len);
        }
        *length = userData->len;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
    return rc;
}

ClRcT _clAmsEntityUserDataGetKey(SaNameT *entity,
                                 SaNameT *key,
                                 ClCharT **data,
                                 ClUint32T *length)
{
    ClAmsEntityUserDataT *userData = NULL;
    ClAmsEntityUserDataStorageT *userDataStorage = NULL;
    ClRcT rc = CL_OK;

    CL_AMS_CHECK_USER_DATA_INITIALIZED();

    if(!entity || !key || !data || !length) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    userData = clAmsEntityUserDataFind(entity);
    if(userData)
    {
        userDataStorage = clAmsEntityUserDataFindKey(userData, key);
        if(!userDataStorage)
        {
            rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
            goto out_unlock;
        }
        *data = NULL;
        if(userDataStorage->len)
        {
            *data = clHeapCalloc(1, userDataStorage->len);
            CL_ASSERT(*data != NULL);
            memcpy(*data, userDataStorage->data, userDataStorage->len);
        }
        *length = userDataStorage->len;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    out_unlock:
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
    return rc;
}

/*
 * Delete the key entry for the user data.
 */
ClRcT _clAmsEntityUserDataDeleteKey(SaNameT *entity, SaNameT *key)
{
    ClRcT rc = CL_OK;
    ClAmsEntityUserDataT *userData = NULL;
    ClAmsEntityUserDataStorageT *userDataStorage = NULL;

    CL_AMS_CHECK_USER_DATA_INITIALIZED();

    if(!entity || !key) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    userData = clAmsEntityUserDataFind(entity);
    if(userData)
    {
        userDataStorage = clAmsEntityUserDataFindKey(userData, key);
        if(!userDataStorage)
        {
            rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
            goto out_unlock;
        }
        clListDel(&userDataStorage->list);
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out_unlock;
    }
    
    if(userDataStorage->data) clHeapFree(userDataStorage->data);
    clHeapFree(userDataStorage);

    out_unlock:
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
    
    return rc;
}

ClRcT _clAmsEntityUserDataDelete(SaNameT *entity, ClBoolT clear)
{
    ClRcT rc = CL_OK;
    ClAmsEntityUserDataT *userData = NULL;
    ClPtrT data = NULL;

    CL_AMS_CHECK_USER_DATA_INITIALIZED();

    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    userData = clAmsEntityUserDataFind(entity);
    if(!userData)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out_unlock;
    }
    if(clear)
    {
        clAmsEntityUserDataFree(userData);
        data = userData;
    }
    else
    {
        /*
         * Rip off the default key,data.
         */
        if(userData->data)
        {
            data = userData->data;
            userData->data = NULL;
        }
        userData->len = 0;
    }

    out_unlock:
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);

    if(data) clHeapFree(data);

    return rc;
}

ClRcT clAmsEntityUserDataDestroy(void)
{
    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    while(!CL_LIST_HEAD_EMPTY(&gClAmsEntityUserDataList))
    {
        ClListHeadT *head= gClAmsEntityUserDataList.pNext;
        ClAmsEntityUserDataT *userData = CL_LIST_ENTRY(head, ClAmsEntityUserDataT, list);
        clAmsEntityUserDataFree(userData);
        clHeapFree(userData);
    }
    numUserDataEntries = 0;
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
    return CL_OK;
}

static ClRcT clAmsEntityUserDataPack(ClAmsEntityUserDataT *userData, 
                                     ClBufferHandleT inMsgHdl)
{
    ClListHeadT *iter = NULL;
    /*
     * First dump the data and then the key value list.
     */
    AMS_CALL(clXdrMarshallSaNameT(&userData->name, inMsgHdl, 0));
    AMS_CALL(clXdrMarshallClUint32T(&userData->len, inMsgHdl, 0));
    if(userData->len)
        AMS_CALL(clXdrMarshallArrayClCharT(userData->data, userData->len, inMsgHdl, 0));
    AMS_CALL(clXdrMarshallClUint32T(&userData->numKeyValues, inMsgHdl, 0));
    CL_LIST_FOR_EACH(iter, &userData->keyValueList)
    {
        ClAmsEntityUserDataStorageT *userDataStorage = CL_LIST_ENTRY(iter, ClAmsEntityUserDataStorageT, list);
        AMS_CALL(clXdrMarshallSaNameT(&userDataStorage->key, inMsgHdl, 0));
        AMS_CALL(clXdrMarshallClUint32T(&userDataStorage->len, inMsgHdl, 0));
        if(userDataStorage->len)
            AMS_CALL(clXdrMarshallArrayClCharT(userDataStorage->data, userDataStorage->len, inMsgHdl, 0));
    }
    return CL_OK;
}

ClRcT clAmsEntityUserDataPackAll(ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;
    ClListHeadT *iter = NULL;

    clOsalMutexLock(&gClAmsEntityUserDataMutex);
    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrMarshallClUint32T(&numUserDataEntries, inMsgHdl, 0),
                                        &gClAmsEntityUserDataMutex);
    CL_LIST_FOR_EACH(iter, &gClAmsEntityUserDataList)
    {
        ClAmsEntityUserDataT *userData = CL_LIST_ENTRY(iter, ClAmsEntityUserDataT, list);
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clAmsEntityUserDataPack(userData, inMsgHdl),
                                            &gClAmsEntityUserDataMutex);
    }
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);

    exitfn:
    return rc;
}

ClRcT clAmsEntityUserDataUnpackAll(ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;
    ClUint32T numEntries = 0;
    ClUint32T i = 0;

    clAmsEntityUserDataDestroy();

    clOsalMutexLock(&gClAmsEntityUserDataMutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallClUint32T(inMsgHdl, &numEntries),
                                        &gClAmsEntityUserDataMutex);

    for(i = 0; i < numEntries; ++i)
    {
        ClUint32T numKeyValues = 0;
        SaNameT name = {0};
        ClCharT *data = NULL;
        ClUint32T len = 0;
        ClUint32T j = 0;
        ClAmsEntityUserDataT *userData = NULL;

        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallSaNameT(inMsgHdl, &name),
                                            &gClAmsEntityUserDataMutex);

        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallClUint32T(inMsgHdl, &len),
                                            &gClAmsEntityUserDataMutex);

        if(len)
        {
            data = clHeapCalloc(1, len);
            CL_ASSERT(data != NULL);
            rc = clXdrUnmarshallArrayClCharT(inMsgHdl, data, len);
            if(rc != CL_OK)
            {
                clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
                clHeapFree(data);
                goto exitfn;
            }
        }
        userData = clHeapCalloc(1, sizeof(*userData));

        CL_ASSERT(userData != NULL);

        CL_LIST_HEAD_INIT(&userData->keyValueList);

        memcpy(&userData->name, &name, sizeof(userData->name));
        userData->data = data;
        userData->len = len;
        clAmsEntityUserDataLink(userData);
        
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallClUint32T(inMsgHdl,
                                                                     &numKeyValues),
                                            &gClAmsEntityUserDataMutex);
        for(j = 0; j < numKeyValues; ++j)
        {
            ClAmsEntityUserDataStorageT *userDataStorage = clHeapCalloc(1, sizeof(*userDataStorage));
            ClCharT *data = NULL;
            ClUint32T len = 0;
            CL_ASSERT(userDataStorage != NULL);
            clListAddTail(&userDataStorage->list, &userData->keyValueList);
            ++userData->numKeyValues;
            AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallSaNameT(inMsgHdl,
                                                                       &userDataStorage->key),
                                                &gClAmsEntityUserDataMutex);
            AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clXdrUnmarshallClUint32T(inMsgHdl, &len),
                                                &gClAmsEntityUserDataMutex);
            if(len)
            {
                data = clHeapCalloc(1, len);
                CL_ASSERT(data != NULL);
                rc = clXdrUnmarshallArrayClCharT(inMsgHdl, data, len);
                if(rc != CL_OK)
                {
                    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
                    clHeapFree(data);
                    goto exitfn;
                }
            }
            userDataStorage->data = data;
            userDataStorage->len = len;
        }
    }
    clOsalMutexUnlock(&gClAmsEntityUserDataMutex);
    return CL_OK;

    exitfn:
    clAmsEntityUserDataDestroy();

    return rc;
}

ClRcT clAmsEntityUserDataInitialize(void)
{
    ClRcT rc;
    if(gClAmsEntityUserDataInitialized == CL_TRUE) return CL_OK;
    rc = clOsalMutexInit(&gClAmsEntityUserDataMutex);    
    CL_ASSERT(rc == CL_OK );
    gClAmsEntityUserDataInitialized = CL_TRUE;
    return CL_OK;
}

ClRcT clAmsEntityUserDataFinalize(void)
{
    if(gClAmsEntityUserDataInitialized == CL_FALSE) return CL_OK;
    gClAmsEntityUserDataInitialized = CL_FALSE;
    clAmsEntityUserDataDestroy();
    return CL_OK;
}

