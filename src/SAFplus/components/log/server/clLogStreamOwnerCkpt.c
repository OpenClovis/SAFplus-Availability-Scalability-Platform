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
#include <string.h>
#include <clCkptExtApi.h>
#include <clCkptErrors.h>
#include <clHandleApi.h>
#include <clCkptIpi.h>
#include <clLogCommon.h>
#include <clLogStreamOwnerCkpt.h>
#include <xdrClLogCompKeyT.h>
#include <xdrClLogSOCompDataT.h>
#include <xdrClLogStreamOwnerDataIDLT.h>

#define CL_STREAMOWNER_CKPT_RETENTION_DURATION (CL_TIME_END)

const ClCharT  soSecPrefix[] = "cl_log_so_section_";    

const ClNameT 
gSOLocalCkptName  = {
                     sizeof("clLogStreamOwnerLocalCkpt") - 1,
                     "clLogStreamOwnerLocalCkpt"
                    };

const ClNameT 
gSOSvrCkptName     = {
                      sizeof("clLogStreamOwnerGlobalCkpt") - 1,
                      "clLogStreamOwnerGlobalCkpt"
                     };

ClRcT
clLogDsIdMapUnpack(ClBufferHandleT  msg,
                   ClLogSOEoDataT   *pSoEoEntry)
{
    ClRcT                   rc     = CL_OK;
    ClUint32T  count = 0;
    ClUint32T  dsId  = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint32T(msg, &pSoEoEntry->dsIdCnt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]",
                    rc));
        return rc;
    }
    for( count = 2; count < pSoEoEntry->dsIdCnt ; count++)
    {
        rc = clXdrUnmarshallClUint32T(msg, &dsId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]",
                        rc));
            return rc;
        }
        rc = clBitmapBitSet(pSoEoEntry->hDsIdMap, dsId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
            return rc;
    }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSODsIdMapRecreate(ClUint32T  dsId,
                       ClAddrT    pBuffer,
                       ClUint32T  size,
                           ClPtrT      cookie)
{
    ClRcT                   rc        = CL_OK;
    ClBufferHandleT        msg            = CL_HANDLE_INVALID_VALUE;
    ClLogSOEoDataT         *pSoEoEntry    = NULL; 
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((CL_LOG_SO_DSID_START != dsId),
                     CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((NULL == pBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, (ClUint8T *) pBuffer, size); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg, &pSoEoEntry->dsIdCnt);
    if( CL_OK != rc )
    {
       CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
       CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }    
    CL_LOG_DEBUG_VERBOSE(("dsIdCnt : %d", pSoEoEntry->dsIdCnt));

    rc = clLogBitmapUnpack(msg, pSoEoEntry->hDsIdMap);

    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}    

/*
 * Function - clLogCkptSectionRead()
 *  - Global Checkpoint Read.
 */

ClRcT
clLogStreamOwnerGlobalEntryRecover(ClLogSOEoDataT            *pSoEoEntry,
                                   ClCkptSectionDescriptorT  *pSecDescriptor)
{
    ClRcT                   rc     = CL_OK;
    ClCkptIOVectorElementT  ioVector       = {{0}};
    ClUint32T               errIndex       = 0;
    ClLogSvrCommonEoDataT   *pCommonEoData = NULL;
    ClUint32T               versionCode = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    ioVector.sectionId  = pSecDescriptor->sectionId; 
    ioVector.dataBuffer = NULL;
    ioVector.dataSize   = 0;
    ioVector.readSize   = 0;
    ioVector.dataOffset = 0;
    rc = clCkptCheckpointRead(pSoEoEntry->hCkpt, &ioVector, 1, &errIndex);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptCheckpointRead(): rc[0x %x]", rc));
        return rc;
    }

    if(ioVector.readSize <= sizeof(versionCode))
    {
        CL_LOG_DEBUG_ERROR(("Global stream recovery failed because of inconsistent ckpt "
                            "data of size [%lld]",
                            ioVector.readSize));
        clHeapFree(ioVector.dataBuffer);
        return CL_LOG_RC(CL_ERR_INVALID_STATE);
    }

    ioVector.readSize -= sizeof(versionCode);
    versionCode = ntohl(*(ClUint32T*)ioVector.dataBuffer);
    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION):
        {
            rc = clLogSOStreamEntryRecreate(0, 
                                            (ClAddrT)( (ClUint8T*)ioVector.dataBuffer + sizeof(versionCode)),
                                            ioVector.readSize,
                                            CL_HANDLE_INVALID_VALUE);
        }
        break;
    default:
        rc = CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
        clLogError("GLOBAL", "RECOVER", "Version unsupported [%d.%d.%d]",
                   CL_VERSION_RELEASE(versionCode), CL_VERSION_MAJOR(versionCode),
                   CL_VERSION_MINOR(versionCode));
        break;
    }

    clHeapFree(ioVector.dataBuffer);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogSOCkptRead()
 *  Walk thru the list of sections.
 *  Read the checkpoint data.
 *  Recreate the entry 
 */
ClRcT
clLogStreamOwnerGlobalStateRecover(ClIocNodeAddressT  masterAddr, ClBoolT switchover)
{
    ClRcT                     rc            = CL_OK;
    ClHandleT                 hSecIter      = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionDescriptorT  secDescriptor = {{0}};
    ClLogSOEoDataT            *pSoEoEntry   = NULL;
    ClIocNodeAddressT         localAddr     = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    localAddr = clIocLocalAddressGet();
    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    if(switchover)
    {
        rc = clCkptActiveReplicaSetSwitchOver(pSoEoEntry->hCkpt);
    }
    else
    {
        rc = clCkptActiveReplicaSet(pSoEoEntry->hCkpt);
    }

    if (CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clCkptActiveReplicaSet(): rc[%#x], switchover flag [%d]", rc, switchover));
        return rc;
    }
    rc = clCkptSectionIterationInitialize(pSoEoEntry->hCkpt,
                                          CL_CKPT_SECTIONS_ANY, 
                                          CL_TIME_END, &hSecIter);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptSectionIterationInitialize(): rc[0x %x]",
                            rc));
        return rc;
    }    
    do
    {    
        rc = clCkptSectionIterationNext(hSecIter, &secDescriptor);
        if( (rc != CL_OK))
        {
            if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc)
                ||
                CL_CKPT_ERR_NO_SECTIONS == CL_GET_ERROR_CODE(rc))
            {
                rc = CL_OK;
            }
            break;
        }
        /* 
         * Create the entries as many as we can, so explicitly
         * not checking the rc.
         */
        if( localAddr == masterAddr )
        {
            clLogStreamOwnerGlobalEntryRecover(pSoEoEntry, &secDescriptor);
        }
        clHeapFree(secDescriptor.sectionId.id);
    }while((rc == CL_OK));

    CL_LOG_CLEANUP(clCkptSectionIterationFinalize(hSecIter), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSODsIdMapPack(ClUint32T  dsId,
                   ClAddrT    *pBuffer,
                   ClUint32T  *pSize,
                   ClPtrT     cookie)
{
    ClRcT                   rc     = CL_OK;
    ClLogSOEoDataT  *pSoEoEntry = NULL;
    ClBufferHandleT  msg        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK( (CL_LOG_DSID_START != dsId),
            CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK( (NULL == pBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK( (NULL == pSize), CL_LOG_RC(CL_ERR_NULL_POINTER));
        
    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClUint32T(&pSoEoEntry->dsIdCnt, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    rc = clLogBitmapPack(pSoEoEntry->hDsIdMap, msg);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }    
    rc = clBufferLengthGet(msg, pSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    *pBuffer = clHeapCalloc(*pSize, sizeof(ClInt8T));
    if( NULL == *pBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }    
    rc = clBufferNBytesRead(msg, (ClUint8T *) *pBuffer, pSize); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        clHeapFree(*pBuffer);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerCkptDelete(ClLogSOEoDataT    *pSoEoEntry, 
                           ClCkptSectionIdT  *pSecId,
                           ClUint32T         dsId)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_INVALID_DSID != dsId )
    {
        rc = clLogStreamOwnerCkptDsIdDelete(pSoEoEntry, dsId);
    }
    else 
    {
        if( (0 != pSecId->idLen) && (NULL != pSecId->id) )
        {
           rc = clCkptSectionDelete(pSoEoEntry->hCkpt, 
                                    pSecId); 
           if( CL_OK != rc )
           {
               CL_LOG_DEBUG_TRACE(("clCkptSectionDelete(): rc[0x %x]", rc));
           }
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerEntrySerialiser(ClUint32T  dsId,
                                ClAddrT    *pBuffer,
                                ClUint32T  *pSize,
                                ClPtrT     cookie)
{
    ClRcT      rc          = CL_OK;
    ClBufferHandleT  msg = (ClBufferHandleT) cookie;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferLengthGet(msg, pSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        return rc;
    }
    *pBuffer = clHeapCalloc(*pSize, sizeof(ClUint8T));
    if( NULL == *pBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return rc;
    }    
    rc = clBufferNBytesRead(msg, (ClUint8T *) *pBuffer, pSize); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        clHeapFree(*pBuffer);
        return rc;
    }
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogCompEntryUnpackNAdd(ClBufferHandleT        msg,
                         ClLogStreamOwnerDataT  *pStreamOwnerData)
{
    ClRcT             rc        = CL_OK;
    ClLogCompKeyT     compKey   = {0};
    ClLogCompKeyT     *pCompKey = NULL;
    ClLogSOCompDataT  compData  = {0};
    ClLogSOCompDataT  *pData    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = VDECL_VER(clXdrUnmarshallClLogCompKeyT, 4, 0, 0)(msg, &compKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrUnmarshallClLogCompKeyT, 4, 0, 0)(): rc[0x %x]", rc));
        return rc;
    }
    if((compKey.nodeAddr < CL_IOC_MIN_NODE_ADDRESS) || (compKey.nodeAddr > CL_IOC_MAX_NODE_ADDRESS))
    {
        CL_LOG_DEBUG_WARN(("Error : Invalid Address [Node 0x%x : CompID 0x%x] passed.\n", compKey.nodeAddr, compKey.compId));
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }

    rc = VDECL_VER(clXdrUnmarshallClLogSOCompDataT, 4, 0, 0)(msg, &compData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrUnmarshallClLogSOCompDataT, 4, 0, 0)(): rc[0x %x]", rc));    
        return rc;
    }

    pCompKey = clHeapCalloc(1, sizeof(ClLogCompKeyT));
    if( NULL == pCompKey )
    {
        CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }    
    *pCompKey = compKey;
    pCompKey->hash = (pCompKey->nodeAddr)%(gLogMaxComponents);
    CL_LOG_DEBUG_VERBOSE(("compKey.nodeAddress: %u", compKey.nodeAddr));
    CL_LOG_DEBUG_VERBOSE(("compKey.compId     : %u", compKey.compId));

    pData = clHeapCalloc(1, sizeof(ClLogSOCompDataT));
    if( NULL == pData )
    {
        CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
        clHeapFree(pCompKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }    
    *pData = compData;
    CL_LOG_DEBUG_VERBOSE(("compData.refCount    : %u", pData->refCount));
    CL_LOG_DEBUG_VERBOSE(("compData.ackerCnt    : %u", pData->ackerCnt));
    CL_LOG_DEBUG_VERBOSE(("compData.nonAckerCnt : %u", pData->nonAckerCnt));

    rc = clCntNodeAdd(pStreamOwnerData->hCompTable, 
                      (ClCntKeyHandleT) pCompKey,
                      (ClCntDataHandleT) pData, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCntNodeAdd(): rc[0x %x]", rc));
        clHeapFree(pData);
        clHeapFree(pCompKey);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogSOStreamEntryUnpackNAdd(ClLogSvrCommonEoDataT  *pCommonEoData, 
                             ClBufferHandleT        msg)
{
    ClRcT             rc         = CL_OK;
    ClUint32T                 size              = 0;
    ClUint32T         count      = 0;
    ClNameT                   streamName        = {0};
    ClNameT                   streamScopeNode   = {0};
    ClLogStreamOwnerDataIDLT  soData            = {0};
    ClLogStreamKeyT           *pStreamKey       = NULL;
    ClCntHandleT              hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogStreamOwnerDataT     *pStreamOwnerData = NULL;
    ClCntNodeHandleT          hStreamOwnerNode  = CL_HANDLE_INVALID_VALUE;
    ClLogStreamScopeT         streamScope       = 0;
    ClLogSOEoDataT            *pSoEoEntry       = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clXdrUnmarshallClNameT(msg, &streamName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }    
    CL_LOG_DEBUG_TRACE(("streamName: %*s", streamName.length,
                        streamName.value));
    
    rc = clXdrUnmarshallClNameT(msg, &streamScopeNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }    
    CL_LOG_DEBUG_TRACE(("streamScopeNode: %*s", streamScopeNode.length,
                        streamScopeNode.value));
    
    rc = clLogStreamScopeGet(&streamScopeNode, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }    
    CL_LOG_DEBUG_TRACE(("streamScope: %d \n", streamScope));

    rc = VDECL_VER(clXdrUnmarshallClLogStreamOwnerDataIDLT, 4, 0, 0)(msg, &soData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clLogStreamOwnerStreamTableGet(&pSoEoEntry, streamScope);
    if( CL_OK != rc )
    {
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }

    rc = clLogSOLock(pSoEoEntry, streamScope);
    if( CL_OK != rc )
    {
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }
    rc = clLogStreamKeyCreate(&streamName, &streamScopeNode, 
                              pCommonEoData->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }
   
    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
        ? pSoEoEntry->hGStreamOwnerTable 
        : pSoEoEntry->hLStreamOwnerTable ;
    rc = clLogStreamOwnerEntryAdd(hStreamTable, streamScope, pStreamKey,
                                  &hStreamOwnerNode); 
    if( CL_OK != rc )
    {
        clHeapFree(pStreamKey);
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }

    rc = clCntNodeUserDataGet(hStreamTable, hStreamOwnerNode,
                              (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clCntNodeDelete(hStreamTable, hStreamOwnerNode), CL_OK);
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    pStreamOwnerData->nodeStatus      = CL_LOG_NODE_STATUS_INIT;
    pStreamOwnerData->streamId        = soData.streamId;
    pStreamOwnerData->streamMcastAddr = soData.streamMcastAddr;
    pStreamOwnerData->dsId            = soData.dsId;
    pStreamOwnerData->isNewStream     = soData.isNewStream;
    pStreamOwnerData->openCnt         = soData.openCnt;
    pStreamOwnerData->ackerCnt        = soData.ackerCnt;
    pStreamOwnerData->nonAckerCnt     = soData.nonAckerCnt;
    clLogStreamAttributesCopy(&soData.streamAttr, &pStreamOwnerData->streamAttr, CL_TRUE);
    CL_LOG_DEBUG_TRACE(("streamId : %hu", soData.streamId));
    CL_LOG_DEBUG_TRACE(("streamMcastAddr: %llu", soData.streamMcastAddr));

    rc = clLogStreamOwnerFilterInfoUnpack(msg, pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clCntNodeDelete(hStreamTable, hStreamOwnerNode), CL_OK);
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    clHeapFree(soData.streamAttr.fileName.pValue);
    clHeapFree(soData.streamAttr.fileLocation.pValue);

    rc = clXdrUnmarshallClUint32T(msg, &size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntNodeDelete(hStreamTable, hStreamOwnerNode), CL_OK);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    CL_LOG_DEBUG_VERBOSE(("compSize : %u", size));

    for(count = 0; count < size; count++)
    {    
        rc = clLogCompEntryUnpackNAdd(msg, pStreamOwnerData);
        if( CL_OK != rc )
        {
            if((CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE) || (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST))
            {
                rc = CL_OK;
            }
            else
            {
                CL_LOG_CLEANUP(clCntNodeDelete(hStreamTable, hStreamOwnerNode), CL_OK);
                CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
                return rc;
            }
        }    
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogSOStreamEntryRecreate(ClUint32T  dsId,
                           ClAddrT    pBuffer,
                           ClUint32T  size,
                           ClPtrT     cookie)
{
    ClRcT                  rc              = CL_OK;
    ClBufferHandleT        msg            = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate()"));
        return rc;
    }    
    rc = clBufferNBytesWrite(msg, (ClUint8T *) pBuffer, size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        clBufferDelete(&msg);
        return rc;
    }    
    rc = clLogSOStreamEntryUnpackNAdd(pCommonEoData, msg);

    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
        rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogSOLocalStreamEntryRecover(ClBitmapHandleT  hDsIdMap,
                               ClUint32T        dsId,
                               void             *pCookie)
{
    ClRcT                  rc             = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;
    ClBoolT dataSetExists = CL_TRUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    
    /* internally calls the deserializer, clLogSOStreamEntryRecreate() */
    rc = clCkptLibraryDoesDatasetExist(pCommonEoData->hLibCkpt,
                                       (ClNameT *) &gSOLocalCkptName,
                                       dsId, &dataSetExists);
    if(rc != CL_OK || !dataSetExists)
    {
        CL_LOG_DEBUG_ERROR(("Dataset [%u] doesnt exist. Continuing", dsId));
        return CL_OK;
    }

    rc = clCkptLibraryCkptDataSetCreate(pCommonEoData->hLibCkpt, 
                                        (ClNameT *) &gSOLocalCkptName,
                                        dsId, 0, 0, 
                                        clLogStreamOwnerEntrySerialiser,
                                        clLogSOStreamEntryRecreate);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetCreate(): rc[0x %x]",
                    rc));
        return rc;
    }    

    rc = clCkptLibraryCkptDataSetRead(pCommonEoData->hLibCkpt, 
                                      (ClNameT *) &gSOLocalCkptName,
                                      dsId, CL_HANDLE_INVALID_VALUE);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetRead(): rc[0x %x]", rc));
        CL_LOG_DEBUG_ERROR(("Unable to recover data from this dsId: %u."
                             " Continuing", dsId));
        return CL_OK;
    }
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogSOLocalStateRecover(ClHandleT       hLibCkpt, 
                         ClLogSOEoDataT  *pSoEoEntry)
{
    ClRcT                  rc              = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    /* internally calls the deserializer, clLogSODsIdMapRecreate() */
    CL_LOG_DEBUG_TRACE(("clLogSODsIdMapRecreate: %s  %d\n",
                gSOLocalCkptName.value, CL_LOG_DSID_START));
    rc = clCkptLibraryCkptDataSetRead(hLibCkpt, 
                                      (ClNameT *) &gSOLocalCkptName,
                                      CL_LOG_DSID_START, 
                                      CL_HANDLE_INVALID_VALUE);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        return CL_OK;
    }    
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetRead(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clBitmapWalkUnlocked(pSoEoEntry->hDsIdMap, 
                              clLogSOLocalStreamEntryRecover, NULL);
    if( CL_OK != rc )
    {
        /* What do we do, revert back or proceed */
        CL_LOG_DEBUG_ERROR(("clBitmapWalkUnlocked(): rc[0x %x]", rc));
    }    
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerLocalCkptCreate(ClCkptSvcHdlT   hLibInit) 
{    
    ClRcT    rc       = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCkptLibraryCkptCreate(hLibInit, (ClNameT *) &gSOLocalCkptName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCreate(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clCkptLibraryCkptDataSetCreate(hLibInit, 
                                        (ClNameT *) &gSOLocalCkptName, 
                                        CL_LOG_DSID_START, 0, 0,
                                        clLogSODsIdMapPack,
                                        clLogSODsIdMapRecreate);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCkptLibraryCkptDelete(hLibInit, 
                                               (ClNameT *) &gSOLocalCkptName),
                       CL_OK);
    }    
        
    CL_LOG_DEBUG_TRACE(("Checkpoint is created: %s  %d \n",
                gSOLocalCkptName.value, CL_LOG_DSID_START));
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogSOLocalCkptGet(ClLogSOEoDataT  *pSoEoEntry)
{
    ClRcT             rc           = CL_OK; 
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;
    ClBoolT                ckptExist      = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clCkptLibraryDoesCkptExist(pCommonEoData->hLibCkpt, 
                                    (ClNameT *) &gSOLocalCkptName, 
                                    &ckptExist);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryDoesCkptExist(): rc[0x %x]", rc));
        return rc;
    }   
    
    rc = clLogStreamOwnerLocalCkptCreate(pCommonEoData->hLibCkpt);
    
    if(CL_OK != rc)
    {
        return rc;
    }

    if( CL_TRUE == ckptExist )
    {
        rc = clLogSOLocalStateRecover(pCommonEoData->hLibCkpt, pSoEoEntry);
    }    

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}    

ClRcT
clLogStreamOwnerCkptDsIdDelete(ClLogSOEoDataT  *pSoEoEntry,
                               ClUint32T       dsId) 
{
    ClRcT           rc      = CL_OK; 
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clBitmapBitClear(pSoEoEntry->hDsIdMap, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]", rc));
        return rc;
    }    
    CL_LOG_CLEANUP(clCkptLibraryCkptDataSetDelete(pCommonEoData->hLibCkpt,
                                                  (ClNameT *) &gSOLocalCkptName, dsId),
                  CL_OK);
    CL_LOG_CLEANUP(clCkptLibraryCkptDataSetWrite(pCommonEoData->hLibCkpt, 
                                                 (ClNameT *) &gSOLocalCkptName,
                                                 CL_LOG_SO_DSID_START,
                                                 pSoEoEntry),
                 CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerCheckpointCreate(ClLogSOEoDataT  *pSoEoEntry,
                                 ClNameT         *pCkptName,
                                 ClHandleT       *phCkpt)
{
    ClRcT         rc     = CL_OK;
    ClCkptCheckpointCreationAttributesT  creationAtt    = {0};
    ClCkptOpenFlagsT                     openFlags      = 0;
    ClLogSvrCommonEoDataT                *pCommonEoData = NULL;
    ClUint32T                            sectionSize    = 0;
    ClInt32T                             tries = 0;
    ClIocNodeAddressT                    localAddr = clIocLocalAddressGet();
    static ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 100};

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }

    creationAtt.creationFlags     = CL_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_ALL_OPEN_ARE_REPLICAS;
    creationAtt.checkpointSize    = 
        pCommonEoData->maxStreams * CL_LOG_SO_SEC_SIZE; 
    creationAtt.retentionDuration = CL_STREAMOWNER_CKPT_RETENTION_DURATION;
    creationAtt.maxSections       = pCommonEoData->maxStreams;
    sectionSize                   = pCommonEoData->maxComponents * 
        sizeof(ClLogCompKeyT); 
    creationAtt.maxSectionSize    = CL_LOG_SO_SEC_SIZE + sectionSize;
    creationAtt.maxSectionIdSize  = CL_LOG_SO_SEC_ID_SIZE;

    openFlags = CL_CKPT_CHECKPOINT_CREATE | CL_CKPT_CHECKPOINT_WRITE |
        CL_CKPT_CHECKPOINT_READ;

    reopen:
    rc = clCkptCheckpointOpen(pCommonEoData->hSvrCkpt, 
                              pCkptName, &creationAtt, openFlags, 5000L,
                              &pSoEoEntry->hCkpt);
    if( rc != CL_OK )
    {
        /*
         * No replica found and we are the only master.
         * Delete and try re-opening the checkpoint
         */
        if (((CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST) || (CL_GET_ERROR_CODE(rc) == CL_ERR_NO_RESOURCE))
            && pCommonEoData->masterAddr == localAddr)
        {
            if(tries++ < 1)
            {
                clLogNotice("CKP", "GET", "No replica for log checkpoint."
                            "Deleting ckpt [%.*s] and retrying the ckpt open",
                            pCkptName->length, pCkptName->value);
                clCkptCheckpointDelete(pCommonEoData->hSvrCkpt, pCkptName);
                clOsalTaskDelay(delay);
                goto reopen;
            }
        }
        CL_LOG_DEBUG_ERROR(("clCkptCheckpointOpen(): rc[0x %x]", rc));
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    
                        
ClRcT
clLogStreamOwnerCkptDsIdGet(ClLogSOEoDataT  *pSoEoEntry, 
                            ClUint32T       *pDsId)
{
    ClRcT             rc         = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;
    ClUint32T              dsId           = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    

    dsId = pSoEoEntry->dsIdCnt;
    dsId++;
    rc = clBitmapBitSet(pSoEoEntry->hDsIdMap, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clCkptLibraryCkptDataSetCreate(pCommonEoData->hLibCkpt, 
                                        (ClNameT *) &gSOLocalCkptName,
                                        dsId, 0, 0, 
                                        clLogStreamOwnerEntrySerialiser,
                                        clLogSOStreamEntryRecreate);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetCreate(): rc[0x %x]",
                    rc));
        CL_LOG_CLEANUP(clBitmapBitClear(pSoEoEntry->hDsIdMap, dsId), CL_OK);
        return rc;
    }    
    pSoEoEntry->dsIdCnt = dsId;
    *pDsId = dsId;

    CL_LOG_CLEANUP(clCkptLibraryCkptDataSetWrite(pCommonEoData->hLibCkpt, 
                                       (ClNameT *) &gSOLocalCkptName,
                                       CL_LOG_SO_DSID_START,
                                       pSoEoEntry), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerGlobalCheckpoint(ClLogSOEoDataT         *pSoEoEntry,
                                 ClNameT                *pStreamName,
                                 ClNameT                *pStreamScopeNode,
                                 ClLogStreamOwnerDataT  *pStreamOwnerData)
{
    ClRcT                             rc                      = CL_OK;
    ClBufferHandleT                   msg                     = 
        CL_HANDLE_INVALID_VALUE;
    ClUint32T                         size                    = 0;
    ClAddrT                           pBuffer                 = NULL;
    ClCkptSectionIdT                  secId                   = {0};
    ClCkptSectionCreationAttributesT  secAttr                 = {0};
    ClBoolT                           createdSec              = CL_FALSE;
    ClUint32T                         prefixLen               = 0;
    ClUint32T                         versionCode             = 0;
    CL_LOG_DEBUG_TRACE(("Enter"));

    prefixLen   = strlen(soSecPrefix);
    secId.idLen = pStreamName->length + prefixLen; 
    secId.id    = clHeapCalloc(secId.idLen, sizeof(ClCharT)); 
    if( NULL == secId.id )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }    
    memcpy(secId.id, soSecPrefix, prefixLen);
    memcpy(secId.id + prefixLen, pStreamName->value,
           pStreamName->length); 
    if( CL_TRUE == pStreamOwnerData->isNewStream )
    {
        secAttr.sectionId      = &secId;
        secAttr.expirationTime = CL_TIME_END;
        rc = clCkptSectionCreate(pSoEoEntry->hCkpt, &secAttr, 
                                 NULL, 0);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptSectionCreate(): rc[0x %x]", rc));
            clHeapFree(secId.id);
            return rc;
        }    
        pStreamOwnerData->isNewStream = CL_FALSE;
        createdSec = CL_TRUE;
    }    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        if( CL_TRUE == createdSec )
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pSoEoEntry->hCkpt, &secId), 
                           CL_OK);
        }
        clHeapFree(secId.id);
        return rc;
    } 
    versionCode = CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION);
    rc = clXdrMarshallClUint32T(&versionCode, msg, 0);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClVersionT() rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        if(CL_TRUE == createdSec)
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pSoEoEntry->hCkpt, &secId), CL_OK);
        }
        clHeapFree(secId.id);
        return rc;
    }

    rc = clLogStreamOwnerEntryPack(pStreamName, pStreamScopeNode, 
                                   pStreamOwnerData, msg);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        if( CL_TRUE == createdSec )
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pSoEoEntry->hCkpt, &secId), 
                           CL_OK);
        }
        clHeapFree(secId.id);
        return rc;
    }
    rc = clLogServerSerialiser(0, &pBuffer, &size, msg);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        if( CL_TRUE == createdSec )
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pSoEoEntry->hCkpt, &secId), 
                           CL_OK);
        }
        clHeapFree(secId.id);
        return rc;
    }    
    rc = clCkptSectionOverwrite(pSoEoEntry->hCkpt,
                                &secId,
                                pBuffer, size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptSectionOverwrite(): rc[0x %x]", rc));
        if( CL_TRUE == createdSec )
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pSoEoEntry->hCkpt, &secId), 
                           CL_OK);
        }
    }    
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
    clHeapFree(pBuffer);
    clHeapFree(secId.id);
   
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * Function - clLogStreamOwnerCheckpoint()
 *  - Based on scope , Checkpoint the data
 */
ClRcT
clLogStreamOwnerCheckpoint(ClLogSOEoDataT     *pSoEoEntry,
                           ClLogStreamScopeT  streamScope,
                           ClCntNodeHandleT   hStreamOwnerNode,
                           ClLogStreamKeyT    *pStreamKey)
{
    ClRcT             rc         = CL_OK;
    ClCntHandleT          hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogStreamOwnerDataT *pStreamOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
        ? pSoEoEntry->hGStreamOwnerTable 
        : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeUserDataGet(hStreamTable, hStreamOwnerNode, 
                              (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("clCntNodeFind(): rc[0x %x]", rc));
        return rc;
    }    
    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, &pStreamKey->streamName,
                                         &pStreamKey->streamScopeNode, 
                                         pStreamOwnerData);
    }    
    else
    {
        clLogStreamOwnerLocalCheckpoint(pSoEoEntry, &pStreamKey->streamName,
                                        &pStreamKey->streamScopeNode,
                                        pStreamOwnerData);
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogCompTablePack(ClCntKeyHandleT   key,
                   ClCntDataHandleT  data,
                   ClCntArgHandleT   arg,
                   ClUint32T         size)
{
    ClRcT             rc        = CL_OK;
    ClLogCompKeyT     *pCompKey = (ClLogCompKeyT *) key;
    ClLogSOCompDataT  *pData    = (ClLogSOCompDataT *) data;
    ClBufferHandleT   msg       = (ClBufferHandleT) arg;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = VDECL_VER(clXdrMarshallClLogCompKeyT, 4, 0, 0)(pCompKey, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }    
    rc = VDECL_VER(clXdrMarshallClLogSOCompDataT, 4, 0, 0)(pData, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));    
        return rc;
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    

ClRcT
clLogStreamOwnerEntryPack(ClNameT                *pStreamName,
                          ClNameT                *pStreamScopeNode,
                          ClLogStreamOwnerDataT  *pStreamOwnerData,
                          ClBufferHandleT        msg)
{
    ClRcT                  rc              = CL_OK;
    ClLogStreamOwnerDataIDLT  soData = {0};
    ClUint32T                 size   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrMarshallClNameT(pStreamName, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clXdrMarshallClNameT(pStreamScopeNode, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }    
    soData.streamId        = pStreamOwnerData->streamId;
    soData.streamMcastAddr = pStreamOwnerData->streamMcastAddr;
    soData.dsId            = pStreamOwnerData->dsId;
    soData.isNewStream     = pStreamOwnerData->isNewStream;
    soData.openCnt         = pStreamOwnerData->openCnt;
    soData.ackerCnt        = pStreamOwnerData->ackerCnt;
    soData.nonAckerCnt     = pStreamOwnerData->nonAckerCnt;

    clLogStreamAttributesCopy(&pStreamOwnerData->streamAttr, 
                              &soData.streamAttr, CL_TRUE);
    rc = VDECL_VER(clXdrMarshallClLogStreamOwnerDataIDLT, 4, 0, 0)(&soData, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }    
    rc = clLogStreamOwnerFilterInfoPack(pStreamOwnerData, msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }
    rc = clCntSizeGet(pStreamOwnerData->hCompTable, &size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(); rc[0x %x]", rc));
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }    
    rc = clXdrMarshallClUint32T(&size, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        clHeapFree(soData.streamAttr.fileName.pValue);
        clHeapFree(soData.streamAttr.fileLocation.pValue);
        return rc;
    }    
    rc = clCntWalk(pStreamOwnerData->hCompTable, clLogCompTablePack,
                   (ClCntArgHandleT) msg, sizeof(msg));

    clHeapFree(soData.streamAttr.fileName.pValue);
    clHeapFree(soData.streamAttr.fileLocation.pValue);
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    
/*
 * Function - clLogStreamOwnerLocalCheckpoint()
 * - Do Local Checkpointing.
 * - If the dataset is not yet created, create the dataSet
 * - Write onto it.
 */
ClRcT
clLogStreamOwnerLocalCheckpoint(ClLogSOEoDataT         *pSoEoEntry,
                                ClNameT                *pStreamName,
                                ClNameT                *pStreamScopeNode,
                                ClLogStreamOwnerDataT  *pStreamOwnerData)
{
    ClRcT                  rc              = CL_OK;
    ClBufferHandleT     msg               = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clLogStreamOwnerEntryPack(pStreamName, pStreamScopeNode, 
                                   pStreamOwnerData, msg);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }    
    rc = clCkptLibraryCkptDataSetWrite(pCommonEoData->hLibCkpt,
                                        (ClNameT *) &gSOLocalCkptName, 
                                        pStreamOwnerData->dsId,
                                        msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
    }    
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerFilterInfoPack(ClLogStreamOwnerDataT  *pStreamOwnerData, 
                               ClBufferHandleT        msg)
{
    ClRcT         rc         = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrMarshallClUint16T(&pStreamOwnerData->streamFilter.severityFilter,
                                msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint16T(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogBitmapPack(pStreamOwnerData->streamFilter.hMsgIdMap, msg);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogBitmapPack(pStreamOwnerData->streamFilter.hCompIdMap, msg);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerFilterInfoUnpack(ClBufferHandleT        msg,
                                 ClLogStreamOwnerDataT  *pStreamOwnerData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint16T(msg, 
                                  &pStreamOwnerData->streamFilter.severityFilter);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint16T(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogBitmapUnpack(msg, pStreamOwnerData->streamFilter.hMsgIdMap);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogBitmapUnpack(msg, pStreamOwnerData->streamFilter.hCompIdMap);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogBitmapPack(ClBitmapHandleT  hBitmap,
                ClBufferHandleT  msg)
{
    ClRcT                   rc        = CL_OK;
    ClUint32T  nBytes   = 0;
    ClUint8T   *pBitMap = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBitmap2BufferGet(hBitmap, &nBytes, &pBitMap);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmap2PositionListGet(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clXdrMarshallClUint32T(&nBytes, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        clHeapFree(pBitMap);
        return rc;
    }
    rc = clXdrMarshallArrayClUint8T(pBitMap, nBytes, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallArrayClUint8T(): rc[0x %x]", rc));
    }
    clHeapFree(pBitMap);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
        return rc;
    }    

ClRcT
clLogBitmapUnpack(ClBufferHandleT  msg,
                  ClBitmapHandleT  hBitmap)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  nBytes    = 0;
    ClUint8T   *pBitMap  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint32T(msg, &nBytes);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("nBytes : %u", nBytes));

    pBitMap = clHeapCalloc(nBytes, sizeof(ClCharT));
    if( NULL == pBitMap )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    rc = clXdrUnmarshallArrayClUint8T(msg, pBitMap, nBytes);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallArrayClUint8T(): rc[0x %x]", rc));
        clHeapFree(pBitMap);
        return rc;
    }
    rc = clBitmapBufferBitsCopy(nBytes, pBitMap, hBitmap);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapPositionList2BitmapGet(): rc[0x %x]",
                    rc));
    }
    clHeapFree(pBitMap);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
        return rc;
    }

ClRcT
clLogStreamOwnerGlobalCkptGet(ClLogSOEoDataT         *pSoEoEntry,
                              ClLogSvrCommonEoDataT  *pCommonEoData,
                              ClBoolT                *pCreateCkpt)
{
    ClRcT             rc        = CL_OK;
    ClCkptOpenFlagsT openFlags = CL_CKPT_CHECKPOINT_WRITE | CL_CKPT_CHECKPOINT_READ;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pCreateCkpt = CL_FALSE;

    if (pCommonEoData->masterAddr != clIocLocalAddressGet())
      {
        rc = clCkptCheckpointOpen(pCommonEoData->hSvrCkpt, &gSOSvrCkptName, NULL, openFlags, 0, &pSoEoEntry->hCkpt);
        if (CL_OK == CL_GET_ERROR_CODE(rc))
          {
            return rc;
          }
      }

    rc = clLogStreamOwnerCheckpointCreate(pSoEoEntry,
                                          (ClNameT *) &gSOSvrCkptName,
                                           &pSoEoEntry->hCkpt);
    if( (CL_OK != rc) && (CL_ERR_ALREADY_EXIST != CL_GET_ERROR_CODE(rc)) )
    {
        CL_LOG_DEBUG_ERROR(("clCkptCheckpointOpen(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_OK == rc )
    {
        *pCreateCkpt = CL_TRUE;
    }

    if( pCommonEoData->masterAddr == clIocLocalAddressGet() )
    {
        rc = clLogStreamOwnerGlobalStateRecover(pCommonEoData->masterAddr, CL_FALSE);
    if( CL_OK != rc )
    {
            CL_LOG_CLEANUP(clCkptCheckpointClose(pSoEoEntry->hCkpt), CL_OK);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
        return rc;
    }    

ClRcT
clLogStreamOwnerLocalCkptDelete(void)
{
    ClRcT                  rc             = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoData);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_CLEANUP(clCkptLibraryCkptDataSetDelete(pCommonEoData->hLibCkpt,
                                                  (ClNameT *) &gSOLocalCkptName,
                                                  CL_LOG_SO_DSID_START),
                  CL_OK);

    CL_LOG_CLEANUP(clCkptLibraryCkptDelete(pCommonEoData->hLibCkpt,
                                           (ClNameT *) &gSOLocalCkptName), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
    
