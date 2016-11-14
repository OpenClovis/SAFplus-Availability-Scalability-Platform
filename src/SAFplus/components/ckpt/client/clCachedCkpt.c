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

#include <sys/mman.h>
#include <math.h>
#include <clCachedCkpt.h>
#include <clLogApi.h>
#include <clHandleApi.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clOsalErrors.h>

extern ClIocNodeAddressT gIocLocalBladeAddress;

/**********************************************************************
  DATABASE SECTION: store the cache (local copy) of the ckpt
 **********************************************************************/

static ClUint32T clCachedCkptShmSizeGet(ClUint32T cachSize)
{
    ClInt32T pageSize = 0;
    ClUint32T shmPages = 0;
    ClUint32T shmSize = 0;

    clOsalPageSizeGet(&pageSize);
    shmPages = (ClUint32T) ceil(cachSize/pageSize);
    shmSize = shmPages * pageSize;

    return shmSize;
}

ClRcT clCachedCkptClientInitialize(ClCachedCkptClientSvcInfoT *serviceInfo,
                                   const ClNameT *ckptName,
                                   ClUint32T cachSize)
{
    ClRcT            rc            = CL_OK;
    ClUint32T        shmSize       = clCachedCkptShmSizeGet(cachSize);
    ClCharT cacheName[CL_MAX_NAME_LENGTH];
    ClUint32T retries = 0;

    serviceInfo->cachSize = shmSize;

    snprintf(cacheName, sizeof(cacheName), "%s_%d", ckptName->value, gIocLocalBladeAddress);

    do
    {
      if (retries > 0)
        usleep(500000); // Waiting semaphore created at SAF services/node representative

      rc = clOsalSemIdGet((ClUint8T*)cacheName, &serviceInfo->cacheSem);
    } while (CL_OK != rc && CL_GET_ERROR_CODE(rc) == CL_OSAL_ERR_OS_ERROR && retries++<5);

    if( CL_OK != rc )
    {
        clLogError("CCK", "INIT_CLIENT", "Failed to get SemId. error code [0x%x].", rc);
        goto out_1;
    }

    retries = 0;
    do
    {
      if (retries > 0)
        usleep(500000); // Waiting shared memory created at SAF services/node representative

      rc = clOsalShmOpen(cacheName , CL_CACHED_CKPT_SHM_OPEN_FLAGS, CL_CACHED_CKPT_SHM_MODE, &serviceInfo->fd);
    } while (CL_OK != rc && CL_GET_ERROR_CODE(rc) == CL_OSAL_ERR_OS_ERROR && retries++<5);

    if( CL_OK != rc )
    {
        clLogError("CCK", "INIT_CLIENT", "Failed to open shared memory for cached checkpoint. error code [0x%x].", rc);
        goto out_1;
    }

    rc = clOsalMmap(0, shmSize, CL_CACHED_CKPT_MMAP_PROT_FLAGS, 
                      CL_CACHED_CKPT_MMAP_FLAGS, serviceInfo->fd, 0, (void **) &serviceInfo->cache);
    if( CL_OK != rc )
    {
        clLogError("CCK", "INIT_CLIENT", "Failed to map shared memory. error code [0x%x].", rc);
        goto out_2;
    }

    return rc;
out_2:
    clOsalShmClose(&serviceInfo->fd);
out_1:
    return rc;
}

ClRcT clCachedCkptClientFinalize(ClCachedCkptClientSvcInfoT *serviceInfo)
{
    ClRcT            rc            = CL_OK;
    ClUint32T        shmSize       = serviceInfo->cachSize;

    rc = clOsalMunmap(serviceInfo->cache, shmSize);
    if(rc != CL_OK)
        clLogError("CCK", "FIN_CLIENT", "clOsalMunmap(): error code [0x%x].", rc);
    serviceInfo->cache = NULL;

    rc = clOsalShmClose(&serviceInfo->fd);
    if(rc != CL_OK)
        clLogError("CCK", "FIN_CLIENT", "clOsalShmClose(): error code [0x%x].", rc);

    return rc;
}

void clCachedCkptClientLookup(ClCachedCkptClientSvcInfoT *serviceInfo,
                              const ClNameT *sectionName,
                              ClCachedCkptDataT **sectionData)
{
    if (!sectionData)
        return;

    *sectionData = NULL;

    if (!serviceInfo->cache)
        return;

    ClUint32T        i             = 0;
    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;

    for (i = 0; i< *numberOfSections; i++)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);

        if ((sectionName->length == pTemp->sectionName.length) 
          && (memcmp(sectionName->value, pTemp->sectionName.value, sectionName->length)==0) )
        {
            *sectionData = pTemp;
            pTemp->data = (ClUint8T *)(pTemp + 1);
            break;
        }

        sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
    }
}

/*
 * Append the data chunk to the last section
 */
ClRcT clCacheEntryDataAppend(ClCachedCkptSvcInfoT *serviceInfo,
                             ClPtrT data,
                             ClUint32T dataSize)
{
    if (!serviceInfo->cache)
        return CL_ERR_NOT_INITIALIZED;

    ClUint32T cacheSize = serviceInfo->cachSize;
    ClUint32T *numberOfSections = (ClUint32T*)serviceInfo->cache;
    ClUint32T *sizeOfCache = (ClUint32T*)(numberOfSections + 1);
    ClCachedCkptDataT *ckptData = (ClCachedCkptDataT*) (sizeOfCache + 1);
    ClUint32T offset = 0;
    ClUint32T hdrSize = (ClUint8T*)ckptData - serviceInfo->cache;

    if(!data || !dataSize)
        return CL_ERR_INVALID_PARAMETER;

    if(!numberOfSections || !*numberOfSections)
        return CL_ERR_INVALID_STATE;

    if(*sizeOfCache + hdrSize + dataSize > cacheSize)
        return CL_ERR_NO_SPACE;

    for(ClUint32T i = 0; i < *numberOfSections; ++i)
    {
        ckptData = (ClCachedCkptDataT*) ( (ClUint8T*)ckptData + offset);
        offset += sizeof(*ckptData) + ckptData->dataSize;
    }
    if(!ckptData->data)
        return CL_ERR_INVALID_STATE;
    memcpy(ckptData->data + ckptData->dataSize, data, dataSize);
    ckptData->dataSize += dataSize;
    *sizeOfCache += dataSize;
    return CL_OK;
}

/*
 * Delete a chunk from the last section matching a data chunk of dataSize
 */
ClRcT clCacheEntryDataDelete(ClCachedCkptSvcInfoT *serviceInfo,
                             ClPtrT data,
                             ClUint32T dataSize)
{
    if (!serviceInfo->cache)
        return CL_ERR_NOT_INITIALIZED;

    ClUint32T *numberOfSections = (ClUint32T*)serviceInfo->cache;
    ClUint32T *sizeOfCache = (ClUint32T*)(numberOfSections + 1);
    ClCachedCkptDataT *ckptData = (ClCachedCkptDataT*)(sizeOfCache + 1);
    ClUint32T hdrSize = (ClUint8T*)ckptData - serviceInfo->cache;
    ClUint32T offset = 0;
    ClRcT rc = CL_OK;

    if(!data || !dataSize)
        return CL_ERR_INVALID_PARAMETER;

    if(!numberOfSections || !*numberOfSections)
        return CL_ERR_INVALID_STATE;
    
    if(dataSize > *sizeOfCache + hdrSize)
        return CL_ERR_NO_SPACE;

    for(ClUint32T i = 0; i < *numberOfSections; ++i)
    {
        ckptData = (ClCachedCkptDataT*) ( (ClUint8T*)ckptData + offset );
        offset += sizeof(*ckptData) + ckptData->dataSize;
    }

    if(!ckptData->data || 
       dataSize > ckptData->dataSize)
        return CL_ERR_NO_SPACE;

    ClUint32T dataChunks = ckptData->dataSize/dataSize;
    ClUint32T chunkSize = 0;

    offset = 0;
    for(ClUint32T i = 0; i < dataChunks; ++i)
    {
        chunkSize = CL_MIN(ckptData->dataSize - offset, dataSize);
        if(chunkSize != dataSize) break;
        if(!memcmp(ckptData->data + offset, data, chunkSize))
        {
            ckptData->dataSize -= chunkSize;
            *sizeOfCache -= chunkSize;
            if(offset < ckptData->dataSize)
            {
                memmove(ckptData->data + offset, ckptData->data + offset + chunkSize, 
                        ckptData->dataSize - offset);
            }
            rc = CL_OK;
            goto found;
        }
        offset += chunkSize;
    }
    rc = CL_ERR_NOT_EXIST;

    found:
    return rc;
}

ClRcT clCacheEntryAdd (ClCachedCkptSvcInfoT *serviceInfo, 
                       const ClCachedCkptDataT *sectionData)
{
    if (!serviceInfo->cache)
        return CL_ERR_NOT_INITIALIZED;

    ClRcT            rc = CL_OK;
    ClUint32T        shmSize = serviceInfo->cachSize;

    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);

    ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *)(data + *sizeOfCache);

    memcpy (pTemp, sectionData, sizeof(ClCachedCkptDataT));
    pTemp->data = (ClUint8T *)(pTemp + 1);
    memcpy (pTemp->data, sectionData->data, sectionData->dataSize);
    (*numberOfSections)++;
    *sizeOfCache += sizeof(ClCachedCkptDataT) + pTemp->dataSize;

    rc = clOsalMsync(serviceInfo->cache, shmSize, MS_SYNC);
    if (rc != CL_OK)
    {
        clLogError("CCK", "ADD", "clOsalMsync(): error code [0x%x].", rc);
    }

    return rc;
}

ClRcT clCacheEntryDelete (ClCachedCkptSvcInfoT *serviceInfo, 
                          const ClNameT *sectionName)
{
    if (!serviceInfo->cache)
        return CL_ERR_NOT_INITIALIZED;

    ClRcT            rc = CL_OK;
    ClUint32T        shmSize = serviceInfo->cachSize;
    ClUint32T        i = 0;

    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;
    ClBoolT          isDelete = CL_FALSE;

    for (i = 0; i< *numberOfSections; i++)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *)(data + sectionOffset);
        ClUint32T dataSize = pTemp->dataSize;
        ClUint32T nextSectionOffset = sectionOffset + sizeof(ClCachedCkptDataT) + dataSize; 
        ClCachedCkptDataT *pNextRecord = (ClCachedCkptDataT *)(data + nextSectionOffset);

        if ((sectionName->length == pTemp->sectionName.length) 
          && (memcmp(sectionName->value, pTemp->sectionName.value, sectionName->length)==0) )
        {
            memmove(pTemp, pNextRecord, *sizeOfCache - nextSectionOffset);
            (*numberOfSections)--;
            *sizeOfCache -= sizeof(ClCachedCkptDataT) + dataSize;
            isDelete = CL_TRUE;
            break;
        }

        sectionOffset = nextSectionOffset;
    }

    if(isDelete)
    {
        rc = clOsalMsync(serviceInfo->cache, shmSize, MS_SYNC);
        if (rc != CL_OK)
        {
            clLogError("CCK", "DEL", "clOsalMsync(): error code [0x%x].", rc);
            return CL_ERR_ALREADY_EXIST;
        }
    }

    return rc;
}

ClRcT clCacheEntryUpdate (ClCachedCkptSvcInfoT *serviceInfo, 
                                       const ClCachedCkptDataT *sectionData)
{
    ClRcT            rc            = CL_OK;

    rc = clCacheEntryDelete(serviceInfo,&sectionData->sectionName);
    if (rc != CL_OK)
    {
        goto error_out;
    }

    rc = clCacheEntryAdd(serviceInfo, sectionData);

error_out:
    return rc;
}

/**********************************************************************
  CHECKPOINT SECTION 
 **********************************************************************/

/**
 * INIT: The below function initializes the checkpoint service client 
 * and opens a checkpoint to store data.
 */
ClRcT clCachedCkptInitialize(ClCachedCkptSvcInfoT *serviceInfo, 
                             const SaNameT *ckptName,
                             const SaCkptCheckpointCreationAttributesT *ckptAttributes,
                             SaCkptCheckpointOpenFlagsT openFlags,
                             ClUint32T cachSize)
{
    ClRcT			rc = CL_OK;
    SaCkptHandleT		ckptSvcHandle = 0;
    SaCkptCheckpointHandleT	ckptHandle = 0;
    SaVersionT			ckptVersion = {'B', 0x01, 0x01};
    ClTimerTimeOutT		delay = {.tsSec = 0, .tsMilliSec = 500};
    ClInt32T			tries = 0;
    ClUint32T                   shmSize = clCachedCkptShmSizeGet(cachSize);

    serviceInfo->cachSize = shmSize;

    if(serviceInfo->ckptHandle != CL_HANDLE_INVALID_VALUE)
    {
        clLogWarning("CCK", "INI", "Checkpoint already initialized. Skipping initialization");
        return CL_ERR_ALREADY_EXIST;
    }

    snprintf(serviceInfo->cacheName, sizeof(serviceInfo->cacheName), "%s_%d", ckptName->value, gIocLocalBladeAddress);

    clOsalShmUnlink(serviceInfo->cacheName);
    rc = clOsalSemCreate((ClUint8T*)serviceInfo->cacheName, 1, &serviceInfo->cacheSem);
    if(rc != CL_OK)
    {
        ClOsalSemIdT semId = 0;

        if(clOsalSemIdGet((ClUint8T*)serviceInfo->cacheName, &semId) != CL_OK)
        {
            clLogError("CCK", "INI", "cache semaphore creation error while fetching semaphore id");
            goto out1;
        }

        if(clOsalSemDelete(semId) != CL_OK)
        {
            clLogError("CCK", "INI", "cache semaphore creation error while deleting old semaphore id");
            goto out1;
        }

        rc = clOsalSemCreate((ClUint8T*)serviceInfo->cacheName, 1, &serviceInfo->cacheSem);
    }

    /* Create shm */

    rc = clOsalShmOpen((ClCharT *)serviceInfo->cacheName, CL_CACHED_CKPT_SHM_EXCL_CREATE_FLAGS,
                       CL_CACHED_CKPT_SHM_MODE, &serviceInfo->fd);
    if( CL_ERR_ALREADY_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clOsalShmOpen((ClCharT *)serviceInfo->cacheName, CL_CACHED_CKPT_SHM_OPEN_FLAGS,
                           CL_CACHED_CKPT_SHM_MODE, &serviceInfo->fd);
        if( CL_OK != rc )
        {
            clLogError("CCK", "INI", "Could not open shared memory.");
            goto out4;
        }
    }

    rc = clOsalFtruncate(serviceInfo->fd, shmSize);
    if( CL_OK != rc )
    {
        clLogError("CCK", "INI", "clOsalFtruncate(): error code [0x%x].", rc);
        goto out5;
    }

    rc = clOsalMmap(0, shmSize, CL_CACHED_CKPT_MMAP_PROT_FLAGS, 
                    CL_CACHED_CKPT_MMAP_FLAGS, serviceInfo->fd, 0, (void **) &serviceInfo->cache);
    if( CL_OK != rc )
    {
        clLogError("CCK", "INI", "clOsalMmap(): error code [0x%x].", rc);
        goto out5;
    }

    ClUint32T  *numberOfSections = (ClUint32T *)(serviceInfo->cache);
    ClUint32T  *sizeOfCache = (ClUint32T *)(numberOfSections + 1);
    *numberOfSections = 0;
    *sizeOfCache = 0;

    clOsalMsync(serviceInfo->cache, shmSize, MS_SYNC);

    if(ckptAttributes || openFlags)
    {
        if(serviceInfo->ckptSvcHandle == CL_HANDLE_INVALID_VALUE)
        {
            /* Initialize checkpoint service instance */
            do
            {
                rc = clCkptInitialize(&ckptSvcHandle, NULL, (ClVersionT *)&ckptVersion);
                clLogNotice("CCK", "INI", "Try [%d] of [100] to initialize checkpoint service rc[0x%x]", tries, rc);
            } while(rc != CL_OK && tries++ < 100 && clOsalTaskDelay(delay) == CL_OK);

            if(rc != CL_OK)
            {
                clLogError("CCK", "INI", "Failed to initialize checkpoint service instance. error code [0x%x].", rc);
                goto out2;
            }

            serviceInfo->ckptSvcHandle = ckptSvcHandle;
        }

        tries = 0;
        /* Create the checkpoint for read and write. */
        do
        {
            rc = clCkptCheckpointOpen(serviceInfo->ckptSvcHandle,
                                      (ClNameT *)ckptName,
                                      (ClCkptCheckpointCreationAttributesT *)ckptAttributes,
                                      openFlags,
                                      0L,
                                      &ckptHandle);
            clLogNotice("CCK", "INI", "Try [%d] of [10] to open checkpoint service rc=[0x%x]", tries, rc);
        } while(rc != CL_OK && tries++ < 10 && clOsalTaskDelay(delay) == CL_OK);

        if(rc != CL_OK)
        {
            clLogError("CCK", "INI", "Failed to open checkpoint. error code [0x%x].", rc);
            goto out3;
        }

        serviceInfo->ckptHandle = ckptHandle;
    }
    goto out1;

    out5:
    clOsalShmClose(&serviceInfo->fd);
    out4:
    if(serviceInfo->ckptHandle)
    {
        clCkptCheckpointClose(serviceInfo->ckptHandle);
        serviceInfo->ckptHandle =  CL_HANDLE_INVALID_VALUE;
    }
    out3:
    if(serviceInfo->ckptSvcHandle)
    {
        clCkptFinalize(serviceInfo->ckptSvcHandle);
        serviceInfo->ckptSvcHandle =  CL_HANDLE_INVALID_VALUE;
    }
    out2:
    clOsalShmUnlink(serviceInfo->cacheName);
    clOsalSemDelete(serviceInfo->cacheSem);
    out1:
    return rc;
}

/*
 * FINALIZE: This function closes the checkpoint and finalizes the 
 * checkpoint service handle. 
 */
ClRcT clCachedCkptFinalize(ClCachedCkptSvcInfoT *serviceInfo)
{
    ClRcT rc;
    ClUint32T shmSize = serviceInfo->cachSize;

    rc = clOsalMunmap(serviceInfo->cache, shmSize);
    if(rc != CL_OK)
        clLogError("CCK", "FIN", "clOsalMunmap(): error code [0x%x].", rc);
    serviceInfo->cache = NULL; // Clear the pointer out after unmap to avoid accessing invalid memory

    rc = clOsalShmClose(&serviceInfo->fd);
    if(rc != CL_OK)
        clLogError("CCK", "FIN", "clOsalShmClose(): error code [0x%x].", rc);

    clOsalShmUnlink(serviceInfo->cacheName);
    rc = clOsalSemDelete(serviceInfo->cacheSem);
    if(rc != CL_OK)
        clLogError("CCK", "FIN", "Failed to destroy cache lock. error code [0x%x].", rc);

    if(serviceInfo->ckptHandle)
    {
        rc = clCkptCheckpointClose(serviceInfo->ckptHandle);
        if (rc != CL_OK)
        {
            clLogError("CCK", "FIN", "Failed to close checkpoint. error code [0x%x].", rc);
        }
        serviceInfo->ckptHandle =  CL_HANDLE_INVALID_VALUE;
    }

    if(serviceInfo->ckptSvcHandle)
    {
        rc = clCkptFinalize(serviceInfo->ckptSvcHandle);
        if (rc != CL_OK)
        {
            clLogError("CCK", "FIN", "Failed to finalize checkpoint service instance. error code [0x%x].", rc);
        }
        serviceInfo->ckptSvcHandle =  CL_HANDLE_INVALID_VALUE;
    }

    return rc;
}

/**
 * CREATE: This function creates a section in the checkpoint.
 */
ClRcT clCachedCkptSectionCreate(ClCachedCkptSvcInfoT *serviceInfo,
                                const ClCachedCkptDataT *sectionData)
{
    ClRcT rc = CL_OK;

    SaCkptSectionIdT ckptSectionId = {        /* Section id for checkpoints   */
        .id = (SaUint8T *) sectionData->sectionName.value,
        .idLen = sectionData->sectionName.length
    };

    SaCkptSectionCreationAttributesT sectionAttrs= {
        .sectionId = &ckptSectionId,
        .expirationTime = CL_TIME_END      /* Setting an infinite time  */
    };

    ClUint8T *ckptedData, *copyData;
    ClSizeT ckptedDataSize = sectionData->dataSize + sizeof(ClIocAddressT);
    ClUint32T network_byte_order;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };

    ckptedData = (ClUint8T *) clHeapAllocate(ckptedDataSize);
    if(ckptedData == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clLogError("CCK", "ADD", "Failed to allocate memory. error code [0x%x].", rc);
        goto out1;
    }

    /* Marshall section data */
    copyData = ckptedData;
    network_byte_order = (ClUint32T) htonl((ClUint32T)sectionData->sectionAddress.iocPhyAddress.nodeAddress);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);
    network_byte_order = (ClUint32T) htonl((ClUint32T)sectionData->sectionAddress.iocPhyAddress.portId);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);
    memcpy(copyData, sectionData->data, sectionData->dataSize);

    /* Try to create a section */
retry:
    rc = clCkptSectionCreate(serviceInfo->ckptHandle,    /* Checkpoint handle  */
                             (ClCkptSectionCreationAttributesT *)&sectionAttrs,       /* Section attributes */
                             ckptedData,          /* Initial data       */
                             ckptedDataSize);     /* Size of data       */
    if (CL_ERR_TRY_AGAIN == CL_GET_ERROR_CODE(rc))
    {
        if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
        {
            goto retry;
        }
    }
    if ((CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST))
    {
        rc = CL_ERR_ALREADY_EXIST;
        goto out2;
    }
    else if (rc != CL_OK)
    {
        clLogError("CCK", "ADD", "CkptSectionCreate failed with rc [0x%x].",rc);
        goto out2;
    }

    /* Add the section data to cache */
    rc = clCacheEntryAdd(serviceInfo, sectionData);

    if (rc != CL_OK)
    {
        clLogError("CCK", "ADD", "CacheEntryAdd failed with rc [0x%x].",rc);
        goto out3;
    }

    goto out2;

out3:
    saCkptSectionDelete(serviceInfo->ckptHandle, &ckptSectionId);
out2:
    clHeapFree(ckptedData);
out1:
    return rc;
}

ClRcT clCkptEntryUpdate(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClCachedCkptDataT *sectionData)
{
    ClRcT rc;

    SaCkptSectionIdT ckptSectionId = {        /* Section id for checkpoints   */
        .id = (SaUint8T *) sectionData->sectionName.value,
        .idLen = sectionData->sectionName.length
    };

    ClUint8T *ckptedData, *copyData;
    ClSizeT ckptedDataSize = sectionData->dataSize + sizeof(ClIocAddressT);
    ClUint32T network_byte_order;
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };

    ckptedData = (ClUint8T *) clHeapAllocate(ckptedDataSize);
    if(ckptedData == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clLogError("CCK", "UPD", "Failed to allocate memory. error code [0x%x].", rc);
        goto out1;
    }

    /* Marshall section data*/
    copyData = ckptedData;
    network_byte_order = (ClUint32T) htonl((ClUint32T)sectionData->sectionAddress.iocPhyAddress.nodeAddress);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);
    network_byte_order = (ClUint32T) htonl((ClUint32T)sectionData->sectionAddress.iocPhyAddress.portId);
    memcpy(copyData, &network_byte_order, sizeof(ClUint32T));
    copyData = copyData + sizeof(ClUint32T);
    memcpy(copyData, sectionData->data, sectionData->dataSize);

    /* Try to update the section */
retry:
    rc = clCkptSectionOverwrite(serviceInfo->ckptHandle,         /* Checkpoint handle  */
                             (ClCkptSectionIdT *)&ckptSectionId,         /* Section ID         */
                             ckptedData,             /* Initial data       */
                             ckptedDataSize);        /* Size of data       */
    if (CL_ERR_TRY_AGAIN == CL_GET_ERROR_CODE(rc))
    {
        if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
        {
            goto retry;
        }
    }
    if (rc != CL_OK)
    {
        clLogError("CCK", "UPD", "CkptSectionUpdate failed with rc [0x%x].",rc);
        goto out2;
    }

out2:
    clHeapFree(ckptedData);
out1:
    return rc;
}

/**
 * UPDATE: This function updates the checkpoint section data 
 */
ClRcT clCachedCkptSectionUpdate(ClCachedCkptSvcInfoT *serviceInfo,
                                const ClCachedCkptDataT *sectionData)
{
    ClRcT rc;

    rc = clCkptEntryUpdate(serviceInfo, sectionData);
    if (rc != CL_OK)
    {
        goto error_out;
    }

    clCacheEntryUpdate(serviceInfo, sectionData);

error_out:
    return rc;
}

#if 0
static ClBoolT clCkptEntryExist(ClCachedCkptSvcInfoT *serviceInfo, const ClNameT *sectionName, ClRcT *ret)
{
    ClBoolT                    retVal         = CL_FALSE;
    ClRcT                      rc;
    ClHandleT                  hSecIter       = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionDescriptorT   secDescriptor  = {{0}};

    rc = clCkptSectionIterationInitialize(serviceInfo->ckptHandle,
                                          CL_CKPT_SECTIONS_ANY,
                                          CL_TIME_END, &hSecIter);
    if( CL_OK != rc )
    {
        clLogError("CCK", "EXIST", "clCkptSectionIterationInitialize(): rc [0x%x].",rc);
        *ret = rc;
        goto out;
    }

    do
    {
        rc = clCkptSectionIterationNext(hSecIter, &secDescriptor);
        if( CL_OK != rc)
        {
            break;
        }

        if ((sectionName->length == secDescriptor.sectionId.idLen) 
            && (memcmp(sectionName->value, secDescriptor.sectionId.id, sectionName->length)==0) )
        {
            clHeapFree(secDescriptor.sectionId.id);
            secDescriptor.sectionId.idLen = 0;
            retVal = CL_TRUE;
            goto out_free;
        }
        clHeapFree(secDescriptor.sectionId.id);
        secDescriptor.sectionId.id = NULL;
        secDescriptor.sectionId.idLen = 0;
    }while( (rc == CL_OK) );

    out_free:
    if (!retVal)
      *ret = rc;

    if(hSecIter)
    {
        rc = clCkptSectionIterationFinalize(hSecIter);
        if( CL_OK != rc )
        {
            clLogError("CCK", "EXIST", "clCkptSectionIterationFinalize(): rc [0x%x].",rc);
        }
    }

    out:
    return retVal;
}
#endif

ClRcT clCkptEntryDelete(ClCachedCkptSvcInfoT *serviceInfo, const ClNameT *sectionName)
{
    ClRcT rc = CL_OK;
#if 0
    ClBoolT isCkptExist = CL_FALSE;
#endif

    SaCkptSectionIdT ckptSectionId = {        /* Section id for checkpoints   */
        .id = (SaUint8T *) sectionName->value,
        .idLen = sectionName->length
    };
    ClInt32T tries = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };
#if 0
retryCheck:
    isCkptExist = clCkptEntryExist(serviceInfo, sectionName, &rc);

    /* Delete section from the ckpt */
  if (isCkptExist == CL_TRUE)
#endif
    {
      tries = 0;
retry:
        rc = clCkptSectionDelete(serviceInfo->ckptHandle, (ClCkptSectionIdT *)&ckptSectionId);
        if (CL_ERR_TRY_AGAIN == CL_GET_ERROR_CODE(rc) || CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE
            || CL_IOC_ERR_HOST_UNREACHABLE == CL_GET_ERROR_CODE(rc) || (CL_GET_CID(rc) == CL_CID_IOC && CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST))
        {
            if ((++tries < 5) && (clOsalTaskDelay(delay) == CL_OK))
            {
                goto retry;
            }
        }
    }
#if 0
  else if ((CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) || (CL_IOC_ERR_HOST_UNREACHABLE == CL_GET_ERROR_CODE(rc)))
    {
      if (tries++ >= 3)
        return rc;
  
      if (tries > 1)
        clOsalTaskDelay(delay);
  
      goto retryCheck;
    }
#endif
    return rc;
}

/**
 * DELETE: This function deletes a section in the checkpoint.
 */
ClRcT clCachedCkptSectionDelete(ClCachedCkptSvcInfoT *serviceInfo,
                                const ClNameT *sectionName)
{
    ClRcT rc = CL_OK;

    /* Delete section from the cache */
    rc = clCacheEntryDelete(serviceInfo, sectionName);
    if (rc != CL_OK)
    {
        clLogError("CCK", "DEL", "CacheEntryDel failed with rc [0x%x].",rc);
    }

    /* Delete section from the ckpt */
    rc = clCkptEntryDelete(serviceInfo, sectionName);
    if (rc != CL_OK)
    {
        clLogError("CCK", "DEL", "CkptSectionDelete failed with rc [0x%x].",rc);
    }

    return rc;
}

/**
 * READ: This function reads a single section and stores in the 
 * ClCachedCkptDataT record
 */
void clCachedCkptSectionRead(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClNameT *sectionName,
                                       ClCachedCkptDataT **sectionData)
{
    *sectionData = NULL;

    if (!serviceInfo->cache)
        return;

    ClUint32T        i = 0;

    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);
    ClUint32T        sectionOffset = 0;

    for (i = 0; i< *numberOfSections; i++)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + sectionOffset);

        if ((sectionName->length == pTemp->sectionName.length) 
          && (memcmp(sectionName->value, pTemp->sectionName.value, sectionName->length)==0) )
        {
            *sectionData = pTemp;
            pTemp->data = (ClUint8T *)(pTemp + 1);
            break;
        }

        sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
    }
}

void clCachedCkptSectionGetFirst(ClCachedCkptSvcInfoT *serviceInfo,
                                 ClCachedCkptDataT **sectionData,
                                 ClUint32T        *sectionOffset)
{
    *sectionData = NULL;

    if (!serviceInfo->cache)
        return;

    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);

    if (*numberOfSections > 0)
    {
        *sectionData = (ClCachedCkptDataT *) (sizeOfCache + 1);
        (*sectionData)->data = (ClUint8T *)(*sectionData + 1);
        *sectionOffset += sizeof(ClCachedCkptDataT) + (*sectionData)->dataSize;
    }
}

void clCachedCkptSectionGetNext(ClCachedCkptSvcInfoT *serviceInfo,
                                ClCachedCkptDataT **sectionData,
                                ClUint32T        *sectionOffset)
{
    *sectionData = NULL;

    if (!serviceInfo->cache)
        return;

    ClUint32T        *numberOfSections = (ClUint32T *) serviceInfo->cache;
    ClUint32T        *sizeOfCache = (ClUint32T *) (numberOfSections + 1);
    ClUint8T         *data = (ClUint8T *) (sizeOfCache + 1);

    if (*sectionOffset < *sizeOfCache)
    {
        ClCachedCkptDataT *pTemp = (ClCachedCkptDataT *) (data + *sectionOffset);
        *sectionData = pTemp;
        pTemp->data = (ClUint8T *)(pTemp + 1);
        *sectionOffset += sizeof(ClCachedCkptDataT) + pTemp->dataSize;
    }
}

ClRcT clCachedCkptSynch(ClCachedCkptSvcInfoT *serviceInfo, ClBoolT isEmpty)
{
    ClRcT                             rc             = CL_OK;
    ClHandleT                         hSecIter       = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionDescriptorT          secDescriptor  = {{0}};
    ClCkptIOVectorElementT            ioVector       = {{0}};
    ClUint32T                         errIndex       = 0;

    rc = clCkptSectionIterationInitialize(serviceInfo->ckptHandle,
                                          CL_CKPT_SECTIONS_ANY,
                                          CL_TIME_END, &hSecIter);
    if( CL_OK != rc )
    {
        clLogError("CCK", "SYNC", "clCkptSectionIterationInitialize(): rc [0x%x].",rc);
        return rc;
    }

    do
    {
        rc = clCkptSectionIterationNext(hSecIter, &secDescriptor);
        if( CL_OK != rc)
        {
            break;
        }

        ioVector.sectionId  = secDescriptor.sectionId;
        ioVector.dataBuffer = NULL;
        ioVector.dataSize   = 0;
        ioVector.readSize   = 0;
        ioVector.dataOffset = 0;

        rc = clCkptCheckpointRead(serviceInfo->ckptHandle, &ioVector, 1,
                                          &errIndex);
        if( CL_OK == rc )
        {
            ClUint8T *ckptedData, *copyData;
            ClUint32T network_byte_order;
            ClCachedCkptDataT sectionData;

            ckptedData = ioVector.dataBuffer;
            copyData = ckptedData;
            
            memset(&sectionData.sectionName, 0, sizeof (ClNameT));
            sectionData.sectionName.length = ioVector.sectionId.idLen;
            memcpy(sectionData.sectionName.value, ioVector.sectionId.id, sectionData.sectionName.length);

            memcpy(&network_byte_order, copyData, sizeof(ClUint32T));
            sectionData.sectionAddress.iocPhyAddress.nodeAddress = (ClUint32T) ntohl((ClUint32T)network_byte_order);
            copyData = copyData + sizeof(ClUint32T);

            memcpy(&network_byte_order, copyData, sizeof(ClUint32T));
            sectionData.sectionAddress.iocPhyAddress.portId = (ClUint32T) ntohl((ClUint32T)network_byte_order);
            copyData = copyData + sizeof(ClUint32T);

            sectionData.data = copyData;
            sectionData.dataSize = ioVector.readSize - sizeof(ClIocAddressT);

            clLogDebug("CCK", "SYNC", "Cache ckpt section [%s] replicated successfully.", sectionData.sectionName.value);
            if (isEmpty)
                clCacheEntryAdd(serviceInfo, (ClCachedCkptDataT *)&sectionData);
            else
                clCacheEntryUpdate(serviceInfo, (ClCachedCkptDataT *)&sectionData);

            clHeapFree(ioVector.dataBuffer);
        }

        clHeapFree(secDescriptor.sectionId.id);
    }while( (rc == CL_OK) );

    rc = clCkptSectionIterationFinalize(hSecIter);
    if( CL_OK != rc )
    {
        clLogError("CCK", "SYNC", "clCkptSectionIterationFinalize(): rc [0x%x].",rc);
        return rc;
    }

    return CL_OK;
}

