/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#ifndef _CL_BACKING_STORAGE_H_
#define _CL_BACKING_STORAGE_H_

#include <clCommon.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CL_BACKING_STORAGE_RC(err) CL_RC(CL_CID_BACKING_STORAGE, err)

#define CL_LOG_AREA_BACKING_STORAGE "BCK-STOR"
#define CL_LOG_CONTEXT_READ_FILE "READF"
#define CL_LOG_CONTEXT_WRITE_FILE "WRITEF"
#define CL_LOG_CONTEXT_READV_FILE "READVF"
#define CL_LOG_CONTEXT_WRITEV_FILE "WRITEVF"
#define CL_LOG_CONTEXT_READW_FILE "READWF"
#define CL_LOG_CONTEXT_INIT_FILE "INITF"
#define CL_LOG_CONTEXT_INIT "INIT"
#define CL_LOG_CONTEXT_EXIT "EXIT"
#define CL_LOG_CONTEXT_WRITE "WR"
#define CL_LOG_CONTEXT_READ "READ"
#define CL_LOG_CONTEXT_WRITEV "WRITEV"
#define CL_LOG_CONTEXT_READV "READV"
#define CL_LOG_CONTEXT_READW "READW"

typedef enum ClBackingStorageType
{
    CL_BACKING_STORAGE_TYPE_FILE,
    CL_BACKING_STORAGE_TYPE_MAX,
}ClBackingStorageTypeT;

typedef ClPtrT ClBackingStorageHandleT;

typedef struct ClBackingStorageAttributes
{
    ClCharT location[CL_MAX_NAME_LENGTH];
    ClUint32T mode;
}ClBackingStorageAttributesT;

typedef struct ClBackingStorageAttributesFile
{
    ClBackingStorageAttributesT baseAttr;
    ClFdT fd;
}ClBackingStorageAttributesFileT;

typedef ClRcT (*ClBackingStorageCallbackT)(ClBackingStorageHandleT handle, ClPtrT pData, ClUint32T dataSize, ClPtrT pArg);

typedef ClRcT (ClBackingStorageInitT)(const ClBackingStorageAttributesT *pAttr, ClPtrT *pPrivateData);

typedef ClRcT (ClBackingStorageWriteT)(ClBackingStorageHandleT handle, ClPtrT pData, ClUint32T dataSize, ClOffsetT offset, ClPtrT pPrivateData);

typedef ClRcT (ClBackingStorageWriteVectorT)(ClBackingStorageHandleT handle, struct iovec *pIOVec, ClUint32T numVectors, ClOffsetT offset, ClPtrT pPrivateData);

typedef ClRcT (ClBackingStorageReadT)(ClBackingStorageHandleT handle, ClPtrT *pData, ClSizeT *pDataSize, ClOffsetT offset, ClPtrT pPrivateData);

typedef ClRcT (ClBackingStorageReadVectorT)(ClBackingStorageHandleT, struct iovec **ppIOVec, ClUint32T *pNumVectors, ClOffsetT offset, ClUint32T readSize, ClPtrT pPrivateData);

typedef ClRcT (ClBackingStorageReadWalkT)(ClBackingStorageHandleT handle, ClBackingStorageCallbackT pCallback, ClPtrT pArg, ClUint32T readSize, ClPtrT pPrivateData);

typedef struct ClBackingStorageOperations
{
    ClBackingStorageInitT *pInit;
    ClBackingStorageWriteT *pWrite;
    ClBackingStorageReadT *pRead;
    ClBackingStorageWriteVectorT *pWriteVector;
    ClBackingStorageReadVectorT *pReadVector;
    ClBackingStorageReadWalkT *pReadWalk;
}ClBackingStorageOperationsT;

ClRcT clBackingStorageCreate(ClBackingStorageHandleT *pHandle, ClBackingStorageTypeT type, const ClBackingStorageAttributesT *pAttr, const ClBackingStorageOperationsT *pOperations);

ClRcT clBackingStorageDelete(ClBackingStorageHandleT *pHandle);

ClRcT clBackingStorageWrite(ClBackingStorageHandleT handle, ClPtrT pData, ClUint32T size, ClOffsetT offset);

ClRcT clBackingStorageRead(ClBackingStorageHandleT handle, ClPtrT *pData, ClSizeT *pSize, ClOffsetT offset);

ClRcT clBackingStorageWriteVector(ClBackingStorageHandleT handle, struct iovec *pIOVec, ClUint32T numVectors, ClOffsetT offset);

ClRcT clBackingStorageReadVector(ClBackingStorageHandleT handle, struct iovec **ppIOVec, ClUint32T *pNumVectors, ClOffsetT offset, ClUint32T readSize);

ClRcT clBackingStorageReadWalk(ClBackingStorageHandleT handle, ClBackingStorageCallbackT pCallback, ClPtrT pArg, ClUint32T readSize);

#ifdef __cplusplus
}
#endif

#endif

