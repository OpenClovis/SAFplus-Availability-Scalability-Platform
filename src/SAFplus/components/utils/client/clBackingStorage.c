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
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clLogUtilApi.h>
#include <clBackingStorage.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct ClBackingStorage
{
    ClBackingStorageTypeT type;
    const ClBackingStorageOperationsT *pOperations;
    ClPtrT pPrivateData;
}ClBackingStorageT;

static ClBackingStorageInitT  clBackingStorageInitFile;
static ClBackingStorageWriteT clBackingStorageWriteFile;
static ClBackingStorageReadT  clBackingStorageReadFile;
static ClBackingStorageWriteVectorT clBackingStorageWriteVectorFile;
static ClBackingStorageReadVectorT clBackingStorageReadVectorFile;
static ClBackingStorageReadWalkT clBackingStorageReadWalkFile;

static ClBackingStorageOperationsT gClBackingStorageOperationsFile = 
   {
        .pInit = clBackingStorageInitFile,
        .pWrite = clBackingStorageWriteFile, 
        .pRead = clBackingStorageReadFile,
        .pWriteVector = clBackingStorageWriteVectorFile,
        .pReadVector = clBackingStorageReadVectorFile,
        .pReadWalk = clBackingStorageReadWalkFile,
   };

static ClBackingStorageOperationsT *gpClBackingStorageOperations[CL_BACKING_STORAGE_TYPE_MAX+1] = {
    [CL_BACKING_STORAGE_TYPE_FILE] = &gClBackingStorageOperationsFile,
};

ClRcT clBackingStorageCreate(ClBackingStorageHandleT *pHandle, ClBackingStorageTypeT type, const ClBackingStorageAttributesT *pAttr, const ClBackingStorageOperationsT *pOperations)
{
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
    ClBackingStorageT *pStorage = NULL;

    if(type < 0 || type >= CL_BACKING_STORAGE_TYPE_MAX)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT, "Backing storage create error. Invalid type [%d] specified.", type);
        goto out;
    }

    if(!pHandle || !pAttr)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT_FILE, "Backing storage create error. Invalid args");
        goto out;
    }

    if( !pOperations && !gpClBackingStorageOperations[type])
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT, "Backing storage create error. No handlers found for type [%d]", type);
        goto out;
    }

    rc = CL_BACKING_STORAGE_RC(CL_ERR_NO_MEMORY);
    pStorage = clHeapCalloc(1, sizeof(*pStorage));
    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT, "Backing storage create error. No memory");
        goto out;
    }
    
    if(pOperations)
    {
        pStorage->pOperations = pOperations;
    }
    else
    {
        pStorage->pOperations = gpClBackingStorageOperations[type];
    }

    if(!pStorage->pOperations->pInit)
    {
        rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT, "Backing storage create error. Init undefined");
        goto out_free;
    }

    pStorage->type = type;
    
    rc = pStorage->pOperations->pInit(pAttr, &pStorage->pPrivateData);
    if(rc != CL_OK)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT, "Backing storage init error. rc [%#x]", rc);
        goto out_free;
    }

    *pHandle = (ClBackingStorageHandleT) pStorage;
    rc = CL_OK;
    goto out;
    
    out_free:
    clHeapFree(pStorage);

    out:
    return rc;
}

ClRcT clBackingStorageDelete(ClBackingStorageHandleT *pHandle)
{
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
    ClBackingStorageT *pStorage = NULL;

    if(!pHandle)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_EXIT, "NULL handle");
        goto out;
    }

    if(*pHandle)
    {
        pStorage = (ClBackingStorageT*)*pHandle;
        if(pStorage->pPrivateData)
        {
            clHeapFree(pStorage->pPrivateData);
        }
        clHeapFree(pStorage);
        *pHandle = 0;
    }
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clBackingStorageWrite(ClBackingStorageHandleT handle, ClPtrT pData, ClUint32T size, ClOffsetT offset)
{
    ClBackingStorageT *pStorage = (ClBackingStorageT*)(ClPtrT)handle;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);

    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE, "Invalid parameter");
        goto out;
    }
    
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_IMPLEMENTED);
    if(pStorage->pOperations->pWrite)
    {
        rc = pStorage->pOperations->pWrite(handle, pData, size, offset, pStorage->pPrivateData);
    }

    out:
    return rc;
}

ClRcT clBackingStorageWriteVector(ClBackingStorageHandleT handle, struct iovec *pIOVec, ClUint32T numVectors, ClOffsetT  offset)
{
    ClBackingStorageT *pStorage = (ClBackingStorageT*) handle;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);

    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITEV, "Invalid parameter");
        goto out;
    }
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_IMPLEMENTED);
    if(pStorage->pOperations->pWriteVector)
    {
        rc = pStorage->pOperations->pWriteVector(handle, pIOVec, numVectors, offset, pStorage->pPrivateData);
    }

    out:
    return rc;
}

ClRcT clBackingStorageRead(ClBackingStorageHandleT handle, ClPtrT *pData, ClSizeT *pSize, ClOffsetT offset)
{
    ClBackingStorageT *pStorage = (ClBackingStorageT*)handle;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
    
    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ, "Invalid parameter");
        goto out;
    }
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_IMPLEMENTED);
    if(pStorage->pOperations->pRead)
    {
        rc = pStorage->pOperations->pRead(handle, pData, pSize, offset, pStorage->pPrivateData);
    }
    out:
    return rc;
}

ClRcT clBackingStorageReadVector(ClBackingStorageHandleT handle, struct iovec **ppIOVec, ClUint32T *pNumVectors, ClOffsetT offset, ClUint32T readSize)
{
    ClBackingStorageT *pStorage = (ClBackingStorageT*) handle;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);

    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV, "Invalid parameter");
        goto out;
    }
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_IMPLEMENTED);
    if(pStorage->pOperations->pReadVector)
    {
        rc = pStorage->pOperations->pReadVector(handle, ppIOVec, pNumVectors, offset, readSize, pStorage->pPrivateData);
    }
    out:
    return rc;
}

ClRcT clBackingStorageReadWalk(ClBackingStorageHandleT handle, ClBackingStorageCallbackT pCallback, ClPtrT pArg, ClUint32T readSize)
{
    ClBackingStorageT *pStorage = (ClBackingStorageT*)handle;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
    
    if(!pStorage)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW, "Invalid parameter");
        goto out;
    }
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_IMPLEMENTED);
    if(pStorage->pOperations->pReadWalk)
    {
        rc = pStorage->pOperations->pReadWalk(handle, pCallback, pArg,
                                              readSize, pStorage->pPrivateData);
    }
    out:
    return rc;
}

/*
 * The operations for the FILE type backing storage.
 */
ClRcT clBackingStorageInitFile(const ClBackingStorageAttributesT *pAttr, ClPtrT *pPrivateData)
{
    ClBackingStorageAttributesFileT *pFileAttr = NULL;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);
    ClFdT fd = -1;

    fd = open(pAttr->location, pAttr->mode, 0666);
    if(fd < 0)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT_FILE, "open failed with error [%s]", strerror(errno));
        goto out;
    }
    rc = CL_BACKING_STORAGE_RC(CL_ERR_NO_MEMORY);
    pFileAttr = clHeapCalloc(1, sizeof(*pFileAttr));
    if(!pFileAttr)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_INIT_FILE, "No memory");
        goto out;
    }
    memcpy(&pFileAttr->baseAttr, pAttr, sizeof(pFileAttr->baseAttr));
    pFileAttr->fd = fd;
    *pPrivateData = (ClPtrT)pFileAttr;
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clBackingStorageWriteFile(ClBackingStorageHandleT handle, ClPtrT pData, ClUint32T size, ClOffsetT offset, ClPtrT pPrivateData)
{
    ClBackingStorageAttributesFileT *pAttr = pPrivateData;
    ClRcT  rc = CL_OK;
    ClInt32T err = 0;
    ClOffsetT curOffset = 0;
    ClInt32T whence = SEEK_SET;

    rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);
    if( offset != (ClOffsetT)-1)
    {
        curOffset = lseek(pAttr->fd, 0, SEEK_CUR);
        if(curOffset < 0 )
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE_FILE, 
                       "File lseek save error [%s]", strerror(errno));
            goto out;
        }
        if(offset < 0)
        {
            whence = SEEK_END;
            size = -offset;
        }
        else
        {
            size -= offset;
        }
        if(lseek(pAttr->fd, offset, whence) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE_FILE, "File lseek error [%s]", strerror(errno));
            goto out;
        }
    }

    while(size > 0 )
    {
        ClInt32T n = CL_MIN(size, 32768);
        err = write(pAttr->fd, pData, n);
        if(err < 0 )
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE_FILE, "Write error [%s]", strerror(errno));
            if(offset != (ClOffsetT)-1)
            {
                if(lseek(pAttr->fd, curOffset, SEEK_SET) < 0)
                {
                    clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE_FILE, "lseek error [%s] restoring back offset", strerror(errno));
                }
            }
            goto out;
        }
        if(err != n)
        {
            clLogWarning(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITE_FILE, "Write wrote [%d] bytes. Expected [%d] bytes", err, n);
        }
        size -= err;
        pData = (ClPtrT)((ClUint8T*)pData + err);
    }

    rc = CL_OK;
    out:
    return rc;
    
}

ClRcT clBackingStorageWriteVectorFile(ClBackingStorageHandleT handle, struct iovec *pIOVec, ClUint32T numVectors, ClOffsetT offset, ClPtrT pPrivateData)
{
    ClBackingStorageAttributesFileT *pAttr = pPrivateData;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_NOT_SUPPORTED);
    ClInt32T err = 0;
    ClOffsetT curOffset = 0;
    ClInt32T whence = SEEK_SET;

    rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);
    if(offset != (ClOffsetT)-1)
    {
        curOffset = lseek(pAttr->fd, 0, SEEK_CUR);
        if(curOffset < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITEV_FILE,
                       "lseek save failed with error [%s]", strerror(errno));
            goto out;
        }
        if(offset < 0 )
        {
            whence = SEEK_END;
        }
        if(lseek(pAttr->fd, offset, whence) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITEV_FILE,
                       "lseek error [%s]", strerror(errno));
            goto out;
        }
    }

    err = writev(pAttr->fd, pIOVec, numVectors);
    if(err < 0)
    {
        if(lseek(pAttr->fd, curOffset, SEEK_SET) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITEV_FILE,
                       "lseek failed to restore offset with error [%s]", 
                       strerror(errno));
        }
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_WRITEV_FILE, "writev failed with error [%s]", strerror(errno));
        goto out;
    }
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clBackingStorageReadFile(ClBackingStorageHandleT handle, ClPtrT *pData, ClSizeT *pSize, ClOffsetT offset, ClPtrT pPrivateData)
{
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);
    ClBackingStorageAttributesFileT *pAttr = pPrivateData;
    ClSizeT size = 0, n = 0, ret_size =0;
    struct stat st;
    ClUint8T *data = NULL;
    ClOffsetT curOffset = 0;
    ClInt32T whence = SEEK_SET;

    if(fstat(pAttr->fd, &st))
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ_FILE, "stat error [%s]", strerror(errno));
        goto out;
    }
    data = clHeapAllocate(st.st_size);
    if(!data)
    {
        rc = CL_BACKING_STORAGE_RC(CL_ERR_NO_MEMORY);
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ_FILE, "No memory");
        goto out;
    }
    size = st.st_size;
    
    if(offset != (ClOffsetT)-1)
    {
        curOffset = lseek(pAttr->fd, 0, SEEK_CUR);
        if(curOffset < 0 )
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ_FILE,
                       "lseek save error [%s]", strerror(errno));
            goto out_free;
        }
        if(offset < 0 )
        {
            whence = SEEK_END;
            ret_size = size = -offset;
        }
        else
        {
            size -= offset;
            ret_size = size;
        }
        if(lseek(pAttr->fd, offset, whence) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ_FILE,
                       "lseek error [%s]", strerror(errno));
            goto out_free;
        }
    }

    while(size > 0)
    {
        ClInt32T err = 0;
        err = read(pAttr->fd, data + n, size);
        if(err < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READ_FILE, "read error [%s]", strerror(errno));
            if(lseek(pAttr->fd, curOffset, SEEK_SET) < 0)
            {
                clLogError(CL_LOG_AREA_BACKING_STORAGE,
                           CL_LOG_CONTEXT_READ_FILE, 
                           "lseek error restoring back [%s]", strerror(errno));
            }
            goto out_free;
        }
        size -= err;
        n += err;
    }
    *pData = (ClPtrT) data;
    *pSize = ret_size;

    rc = CL_OK;
    goto out;

    out_free:
    clHeapFree(data);
    out:
    return rc;
    
}

ClRcT clBackingStorageReadVectorFile(ClBackingStorageHandleT handle, struct iovec **ppIOVec, ClUint32T *pNumVectors, ClOffsetT offset,  ClUint32T readSize, ClPtrT pPrivateData)
{
    ClBackingStorageAttributesFileT *pAttr = pPrivateData;
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);
    struct iovec *pIOVec = NULL;
    ClUint32T numVectors = 0;
    ClSizeT size = 0, n = 0;
    ClInt32T err =0 , i = 0;
    struct stat st;
    ClCharT **ppDataBlocks = NULL;
    ClOffsetT curOffset = 0;
    ClInt32T whence = SEEK_SET;
    ClInt32T lastBatchStart = 0;

    *ppIOVec = NULL;
    *pNumVectors = 0;

    if(readSize > 0x7fff)
    {
        rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, 
                   "Invalid readSize [%d]", readSize);
        goto out;
    }

    if(fstat(pAttr->fd, &st))
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, "stat error [%s]", strerror(errno));
        goto out;
    }

    size = st.st_size;
    if(!size)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, 
                   "File [%s] empty", pAttr->baseAttr.location);
        goto out;
    }
    if(offset != (ClOffsetT)-1)
    {
        curOffset = lseek(pAttr->fd, 0, SEEK_CUR);
        if(curOffset < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE,
                       "lseek save error [%s]", strerror(errno));
            goto out;
        }
        if(offset < 0 )
        {
            size = -offset;
            whence = SEEK_END;
        }
        else
        {
            size -= offset;
        }
        if(lseek(pAttr->fd, offset, whence) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE,
                       "lseek error [%s]", strerror(errno));
            goto out;
        }
    }

    if(readSize && (size % readSize))
    {
        rc = CL_BACKING_STORAGE_RC(CL_ERR_INVALID_PARAMETER);
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, 
                   "File size [%lld] not a multiple of readsize [%d]",
                   size, readSize);
        goto out_restore;
    }

    if(!readSize)
    {
        readSize =  0x3fff+1;
    }

    rc = CL_BACKING_STORAGE_RC(CL_ERR_NO_MEMORY);
    numVectors = size/readSize;
    if(( size % readSize))
        ++numVectors;

    pIOVec = clHeapCalloc(numVectors, sizeof(*pIOVec));
    if(!pIOVec)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, "No memory");
        goto out_restore;
    }
    ppDataBlocks = clHeapCalloc(numVectors, sizeof(*ppDataBlocks));
    if(!ppDataBlocks)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, "No memory");
        goto out_restore;
    }

    for(i = 0; i < numVectors && size > 0;++i)
    {
        ClUint32T bytes = CL_MIN(readSize, size);
        ppDataBlocks[i] = clHeapAllocate(bytes);
        if(!ppDataBlocks[i])
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE, "No memory");
            goto out_free;
        }
        pIOVec[i].iov_base = ppDataBlocks[i];
        pIOVec[i].iov_len = bytes;
        size -= readSize;
        n += readSize;
        if(i+1 >= numVectors
           ||
           readSize >=  0x3fff
           ||
           n >= 0x3fff
           )
        {
            if(n >= 0x3fff)
            {
                n = 0;
            }
            err = readv(pAttr->fd, &pIOVec[lastBatchStart], i-lastBatchStart+1);
            if(err < 0)
            {
                clLogError(CL_LOG_AREA_BACKING_STORAGE, 
                           CL_LOG_CONTEXT_READV_FILE,
                           "readv error [%s]", strerror(errno));
                goto out_restore;
            }
            lastBatchStart = i+1;
        }
    }

    *ppIOVec = pIOVec;
    *pNumVectors = numVectors;
    rc = CL_OK;
    goto out;

    out_restore:
    if(offset != (ClOffsetT)-1)
    {
        if(lseek(pAttr->fd, curOffset, SEEK_SET) < 0)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READV_FILE,
                       "lseek error  restoring back [%s]", strerror(errno));
        }
    }
    out_free:
    {
        ClInt32T j;
        for(j = 0; j < i; ++j)
            clHeapFree(ppDataBlocks[j]);
    }
    if(pIOVec)
        clHeapFree(pIOVec);
    if(ppDataBlocks)
        clHeapFree(ppDataBlocks);

    out:
    return rc;
}

ClRcT clBackingStorageReadWalkFile(ClBackingStorageHandleT handle,
                                   ClBackingStorageCallbackT pCallback,
                                   ClPtrT pArg,
                                   ClUint32T readSize,
                                   ClPtrT pPrivateData)
{
    ClBackingStorageAttributesFileT *pAttr = pPrivateData;
    ClCharT data[0xffff+1];
    ClSizeT size = 0;
    ClOffsetT curOffset = 0;
    struct stat st = {0};
    ClRcT rc = CL_BACKING_STORAGE_RC(CL_ERR_LIBRARY);

    if(fstat(pAttr->fd, &st) < 0)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                   "stat error [%s]", strerror(errno));
        goto out;
    }
    if(!st.st_size)
    {
        rc = CL_OK;
        clLogWarning(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                     "File [%s] size is empty", pAttr->baseAttr.location);
        goto out;
    }
    curOffset = lseek(pAttr->fd, 0, SEEK_CUR);
    if(curOffset < 0)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                   "lseek error [%s]", strerror(errno));
        goto out;
    }
    if(lseek(pAttr->fd, 0, SEEK_SET) < 0)
    {
        clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                   "lseek error [%s]", strerror(errno));
        goto out;
    }

    size = st.st_size;
    while(size > 0)
    {
        ClInt32T err = 0;
        ClCharT *pData = data;
        ClInt32T chunkSize = CL_MIN((ClInt32T)sizeof(data), size);
        ClUint32T bytes = 0;

        if(readSize)
        {
            if(size < readSize)
            {
                clLogError(CL_LOG_AREA_BACKING_STORAGE,
                           CL_LOG_CONTEXT_READW_FILE,
                           "Read walk error. Chunksize doesnt match readsize [%d]",
                           chunkSize);
                goto out_restore;
            }
            chunkSize = readSize;
            pData = clHeapAllocate(chunkSize);
            if(!pData)
            {
                clLogError(CL_LOG_AREA_BACKING_STORAGE, 
                           CL_LOG_CONTEXT_READW_FILE,
                           "No memory");
                goto out_restore;
            }
        }
        while(chunkSize > 0)
        {
            err = read(pAttr->fd, pData+bytes, chunkSize);
            if(err < 0)
            {
                clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                           "read error [%s]", strerror(errno));
                if(pData != data)
                {
                    clHeapFree(pData);
                }
                goto out_restore;
            }
            bytes += err;
            chunkSize -= err;
        }

        rc = pCallback(handle, (ClPtrT)pData, bytes, pArg);
        if(rc != CL_OK)
        {
            clLogError(CL_LOG_AREA_BACKING_STORAGE, CL_LOG_CONTEXT_READW_FILE,
                       "readwalk callback error [%#x]", rc);
            if(pData != data)
            {
                clHeapFree(pData);
            }
            goto out_restore;
        }
        size -= bytes;
    }
    rc = CL_OK;

    out_restore:
    lseek(pAttr->fd, curOffset, SEEK_SET);

    out:
    return rc;
}
