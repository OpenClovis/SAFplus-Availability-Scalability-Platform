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
#include <clCpmApi.h>
#include <clCpmExtApi.h>
#include <clCkptApi.h>
#include <clCkptIpi.h>
#include <clLogCommon.h>
#include <clLogMasterEo.h>
#include <clLogMasterCkpt.h>
#include <clLogMaster.h>
#include <xdrClLogCompDataT.h>

#define CL_LOGMASTER_CKPT_RETENTION_DURATION (CL_TIME_END)

static ClRcT
clLogMasterEoEntryCheckpoint(ClLogMasterEoDataT *pMasterEoEntry);

static ClRcT
clLogMasterFileEntryCheckpoint(ClLogMasterEoDataT  *pMasterEoEntry,
                               ClCntNodeHandleT    hFileNode,
                               ClBoolT             addSection);

static ClRcT
clLogMasterFileEntryPack(ClLogMasterEoDataT  *pMasterEoEntry,
                         ClCntNodeHandleT    hFileNode,
                         ClCkptSectionIdT    *pSecId,
                         ClUint8T            **ppBuffer,
                         ClUint32T           *pBufferLen);
static ClRcT
clLogMasterFileEntrySecIdGet(ClLogFileKeyT     *pFileKey,
                             ClCkptSectionIdT  *pSecId);

static ClRcT
clLogMasterFileKeyPack(ClLogFileKeyT    *pFileKey,
                       ClBufferHandleT  hFileEntryBuf);

static ClRcT
clLogMasterFileDataPack(ClLogFileDataT   *pFileData,
                        ClBufferHandleT  hFileEntryBuf);

static ClRcT
clLogMasterStreamEntryPack(ClCntKeyHandleT   key,
                           ClCntDataHandleT  data,
                           ClCntArgHandleT   arg,
                           ClUint32T         size);

static ClRcT
clLogMasterEoEntryRecover(ClLogMasterEoDataT      *pMasterEoEntry,
                          ClCkptIOVectorElementT  *pIoVector,
                          ClUint32T               *pErrIndex);

static ClRcT
clLogMasterFileEntryRecover(ClLogMasterEoDataT      *pMasterEoEntry,
                            ClCkptIOVectorElementT  *pIoVector,
                            ClUint32T               *pErrIndex);

static ClRcT
clLogMasterFileKeyUnpack(ClLogFileKeyT    *pFileKey,
                         ClBufferHandleT  hFileEntryBuf);

static ClRcT
clLogMasterFileDataRecreate(ClLogFileDataT   *pFileData,
                            ClBufferHandleT  hFileEntryBuf);


static ClRcT
clLogMasterStreamTableRecreate(ClLogSvrCommonEoDataT  *pCommonEoEntry,
                               ClBufferHandleT        hFileEntryBuf,
                               ClLogFileDataT         *pFileData);

ClNameT gLogMasterCkptName
    = {sizeof("clLogMasterCkpt") - 1 , "clLogMasterCkpt"};

ClCkptSectionIdT gLogMasterDefaultSectionId
    = {sizeof("clLogMasterDefaultSection") - 1, (ClUint8T *) "clLogMasterDefaultSection"};

ClCkptSectionIdT gLogMasterCompDataSectionId
    = {sizeof("clLogMasterCompDataSection") - 1, (ClUint8T *) "clLogMasterCompDataSection"};

ClRcT
clLogMasterCkptGet(void)
{
    ClRcT                                rc              = CL_OK;
    ClCkptCheckpointCreationAttributesT  ckptAttr        = {0};
    ClCkptOpenFlagsT                     openFlags       = 0;
    ClLogSvrCommonEoDataT                *pCommonEoEntry = NULL;
    ClLogMasterEoDataT                   *pMasterEoEntry = NULL;
    ClTimeT                              timeout         = 5000L;
    ClIocNodeAddressT                    localAddr = clIocLocalAddressGet();
    ClInt32T tries = 0;
    static ClTimerTimeOutT delay = { .tsSec = 0, .tsMilliSec = 100 };

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    ckptAttr.creationFlags     = CL_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_ALL_OPEN_ARE_REPLICAS;
    ckptAttr.checkpointSize    = pMasterEoEntry->maxFiles
        * pMasterEoEntry->sectionSize;
    ckptAttr.retentionDuration = CL_LOGMASTER_CKPT_RETENTION_DURATION;
    ckptAttr.maxSections       = pMasterEoEntry->maxFiles;
    ckptAttr.maxSectionSize    = pMasterEoEntry->sectionSize;
    ckptAttr.maxSectionIdSize  = pMasterEoEntry->sectionIdSize;

    //   openFlags = CL_CKPT_CHECKPOINT_WRITE | CL_CKPT_CHECKPOINT_READ;
    //rc = clCkptCheckpointOpen(pCommonEoEntry->hSvrCkpt, &gLogMasterCkptName, 
    //                          NULL, openFlags, 0,  &pMasterEoEntry->hCkpt);
    //    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    //  {
    openFlags = CL_CKPT_CHECKPOINT_CREATE | CL_CKPT_CHECKPOINT_WRITE
        | CL_CKPT_CHECKPOINT_READ;

    reopen:
    rc = clCkptCheckpointOpen(pCommonEoEntry->hSvrCkpt, &gLogMasterCkptName,
                              &ckptAttr,openFlags, timeout,
                              &pMasterEoEntry->hCkpt);
    if( (CL_OK != rc) && (CL_ERR_ALREADY_EXIST != rc) )
    {
        /*
         * No replica found and we are the only master.
         * Delete and try re-opening the checkpoint
         */
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NO_RESOURCE &&
           pCommonEoEntry->masterAddr == localAddr)
        {
            if(tries++ < 1)
            {
                clLogNotice("CKP", "GET", "No replica for log checkpoint."
                            "Deleting ckpt [%.*s] and retrying the ckpt open",
                            gLogMasterCkptName.length, gLogMasterCkptName.value);
                clCkptCheckpointDelete(pCommonEoEntry->hSvrCkpt, &gLogMasterCkptName);
                clOsalTaskDelay(delay);
                goto reopen;
            }
        }
        CL_LOG_DEBUG_ERROR(("clCkptCheckpointOpen(): rc[0x%x]", rc));
        return rc;
    }
    //     if( CL_ERR_ALREADY_EXIST != CL_GET_ERROR_CODE(rc))
    {/* Create default section */
    }
    //    return rc;
    //    }

    if(pCommonEoEntry->masterAddr == localAddr)
    {
        rc = clLogMasterStateRecover(pCommonEoEntry, pMasterEoEntry, CL_FALSE);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clCkptCheckpointClose(pMasterEoEntry->hCkpt),
                           CL_OK);
        }
    } /* else you are standby so just keep the ckpt opened */

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x%x]", rc));
    return rc;
}

ClRcT
clLogMasterCkptDestroy(void)
{
    ClRcT                  rc              = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogMasterEoDataT     *pMasterEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_CLEANUP(clCkptCheckpointClose(pMasterEoEntry->hCkpt), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterDataCheckpoint(ClLogMasterEoDataT  *pMasterEoEntry,
                          ClCntNodeHandleT    hFileNode,
                          ClBoolT             addCkptEntry)
{
    ClRcT                  rc              = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_TRUE == addCkptEntry )
    {
        rc = clLogMasterEoEntryCheckpoint(pMasterEoEntry);
        if( CL_OK != rc )
        {
            return rc;
        }/* we don't recover streamId */
    }

    rc = clLogMasterFileEntryCheckpoint(pMasterEoEntry,
                                        hFileNode,
                                        addCkptEntry);

    CL_LOG_DEBUG_TRACE(("Exit: rc [0x %x]", rc));
    return rc;
}

static ClRcT
clLogMasterEoEntryCheckpoint(ClLogMasterEoDataT *pMasterEoEntry)
{
    ClRcT            rc          = CL_OK;
    ClBufferHandleT  hEoEntryBuf = CL_HANDLE_INVALID_VALUE;
    ClUint8T         *pBuffer    = NULL;
    ClUint32T        bufferLen   = 0;
    ClVersionT       version     = {.releaseCode = CL_RELEASE_VERSION,
                                    .majorVersion = CL_MAJOR_VERSION,
                                    .minorVersion = CL_MINOR_VERSION,
    };
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferCreate(&hEoEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClVersionT(&version, hEoEntryBuf, 0);
    
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClVersionT(): rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    rc = clXdrMarshallClUint16T(&(pMasterEoEntry->nextStreamId),
                                hEoEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint16T(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    rc = clBufferLengthGet(hEoEntryBuf, &bufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet: rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    rc = clBufferFlatten(hEoEntryBuf, &pBuffer);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferFlatten(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    rc = clCkptSectionOverwrite(pMasterEoEntry->hCkpt, 
                                &gLogMasterDefaultSectionId, pBuffer,
                                bufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptSectionOverwrite(): rc[0x %x]", rc));
    }

    clHeapFree(pBuffer);
    CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileEntryCheckpoint(ClLogMasterEoDataT  *pMasterEoEntry,
                               ClCntNodeHandleT    hFileNode,
                               ClBoolT             addSection)
{
    ClRcT                             rc        = CL_OK;
    ClUint8T                          *pBuffer  = NULL;
    ClUint32T                         bufferLen = 0;
    ClCkptSectionIdT                  secId     = {0};
    ClCkptSectionCreationAttributesT  secAttr   = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterFileEntryPack(pMasterEoEntry, hFileNode, &secId, &pBuffer,
                                  &bufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileEntryPack(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_TRUE == addSection )
    {
        secAttr.sectionId      = &secId;
        secAttr.expirationTime = CL_TIME_END;
        rc = clCkptSectionCreate(pMasterEoEntry->hCkpt, &secAttr, NULL, 0);
        if( CL_OK != rc )
        {
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
            {
                rc = CL_OK;
                addSection = CL_FALSE;
            }
            else
            {
                CL_LOG_DEBUG_ERROR(("clCkptSectionCreate(): rc[0x %x]", rc));
                clHeapFree(secId.id);
                clHeapFree(pBuffer);
                return rc;
            }
        }
    }

    rc = clCkptSectionOverwrite(pMasterEoEntry->hCkpt, &secId, pBuffer,
                                bufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptSectionOverwrite(): rc[0x %x]", rc));
        if( CL_TRUE == addSection )
        {
            CL_LOG_CLEANUP(clCkptSectionDelete(pMasterEoEntry->hCkpt, &secId),
                           CL_OK);
            clHeapFree(secId.id);
            clHeapFree(pBuffer);
            return rc;
        }
    }

    clHeapFree(secId.id);
    clHeapFree(pBuffer);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileEntryPack(ClLogMasterEoDataT  *pMasterEoEntry,
                         ClCntNodeHandleT    hFileNode,
                         ClCkptSectionIdT    *pSecId,
                         ClUint8T            **ppBuffer,
                         ClUint32T           *pBufferLen)
{
    ClBufferHandleT  hFileEntryBuf = CL_HANDLE_INVALID_VALUE;
    ClLogFileKeyT    *pFileKey     = NULL;
    ClLogFileDataT   *pFileData    = NULL;
    ClRcT            rc            = CL_OK;
    ClVersionT       version       = { .releaseCode = CL_RELEASE_VERSION,
                                       .majorVersion = CL_MAJOR_VERSION,
                                       .minorVersion = CL_MINOR_VERSION,
    };
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserKeyGet(pMasterEoEntry->hMasterFileTable,
                             hFileNode, (ClCntKeyHandleT *) &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntNodeUserDataGet(pMasterEoEntry->hMasterFileTable,
                              hFileNode, (ClCntDataHandleT *) &pFileData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogMasterFileEntrySecIdGet(pFileKey, pSecId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileEntrySecIdGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBufferCreate(&hFileEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        clHeapFree(pSecId->id);
        return rc;
    }
    rc = clXdrMarshallClVersionT(&version, hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClVersionT(): rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }

    rc = clLogMasterFileKeyPack(pFileKey, hFileEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileKeyPack(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }

    rc = clLogMasterFileDataPack(pFileData, hFileEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileDataPack(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }

    rc = clBufferLengthGet(hFileEntryBuf, pBufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet: rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }

    /* rc = clBufferFlatten(hFileEntryBuf, ppBuffer); */
    *ppBuffer = clHeapCalloc(*pBufferLen, sizeof(ClCharT));
    if( NULL == *ppBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }
    rc = clBufferNBytesRead(hFileEntryBuf, (ClUint8T *) *ppBuffer, pBufferLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferFlatten(): rc[0x %x]", rc));
        clHeapFree(*ppBuffer);
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        clHeapFree(pSecId->id);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileEntrySecIdGet(ClLogFileKeyT     *pFileKey,
                             ClCkptSectionIdT  *pSecId)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter: namelen: %u locationlen: %u",
                       pFileKey->fileName.length,
                       pFileKey->fileLocation.length));

    CL_LOG_PARAM_CHK((NULL == pFileKey->fileName.pValue),
                     CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pFileKey->fileLocation.pValue),
                     CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pSecId), CL_LOG_RC(CL_ERR_NULL_POINTER));

    /*
     * ClString: Assuming length includes NULL terminator also.
     * So we idLen will include 2 more than the characters.
     * One we will use for / so we need to decrement it by 1
     */
    pSecId->idLen = pFileKey->fileName.length + pFileKey->fileLocation.length + 1;
    pSecId->id    = clHeapCalloc(pSecId->idLen, sizeof(ClCharT));
    if( NULL == pSecId->id )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    snprintf((ClCharT *) pSecId->id, pSecId->idLen, "%s/%s", pFileKey->fileLocation.pValue,
            pFileKey->fileName.pValue);

    CL_LOG_DEBUG_TRACE(("Exit [%*s]", pSecId->idLen, pSecId->id));
    return rc;
}

static ClRcT
clLogMasterFileKeyPack(ClLogFileKeyT    *pFileKey,
                       ClBufferHandleT  hFileEntryBuf)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrMarshallClStringT(&pFileKey->fileLocation, hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClStringT(&pFileKey->fileName, hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileDataPack(ClLogFileDataT   *pFileData,
                        ClBufferHandleT  hFileEntryBuf)
{
    ClRcT      rc      = CL_OK;
    ClUint32T  cntSize = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    /*
     * Assuming: fileLocation and fileName in the Attributes are empty
     */
    rc = VDECL_VER(clXdrMarshallClLogStreamAttrIDLT, 4, 0, 0)(&(pFileData->streamAttr),
                                          hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR((
            "clXdrMarshallClLogStreamAttributesT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntSizeGet(pFileData->hStreamTable, &cntSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(); rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClUint32T(&cntSize, hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntWalk(pFileData->hStreamTable, clLogMasterStreamEntryPack,
                   (ClCntArgHandleT) hFileEntryBuf, sizeof(hFileEntryBuf));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterStreamEntryPack(ClCntKeyHandleT   key,
                           ClCntDataHandleT  data,
                           ClCntArgHandleT   arg,
                           ClUint32T         size)
{
    ClRcT                   rc            = CL_OK;
    ClLogStreamKeyT         *pStreamKey   = (ClLogStreamKeyT *) key;
    ClLogMasterStreamDataT  *pStreamData  = (ClLogMasterStreamDataT *) data;
    ClBufferHandleT         hFileEntryBuf = (ClBufferHandleT) arg;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrMarshallClNameT(&(pStreamKey->streamName), hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClNameT(&(pStreamKey->streamScopeNode), hFileEntryBuf,
                              0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClUint64T(&(pStreamData->streamMcastAddr), hFileEntryBuf,
                                0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint64T(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrMarshallClUint16T(&(pStreamData->streamId), hFileEntryBuf, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint16T(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogMasterGlobalCkptRead(ClLogSvrCommonEoDataT  *pCommonEoEntry, ClBoolT switchover)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
      
    rc = clLogMasterStateRecover(pCommonEoEntry, pMasterEoEntry, switchover);
    if( CL_OK != rc )
    {
       return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterStateRecover(ClLogSvrCommonEoDataT  *pCommonEoEntry,
                        ClLogMasterEoDataT    *pMasterEoEntry,
                        ClBoolT switchover)
{
    ClRcT                             rc             = CL_OK;
    ClHandleT                         hSecIter       = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionDescriptorT          secDescriptor  = {{0}};
    ClCkptIOVectorElementT            ioVector       = {{0}};
    ClUint32T                         errIndex       = 0;
    ClCkptSectionCreationAttributesT  secAttr        = {0};
    ClBoolT                           logReadFlag    = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntrySet(pMasterEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterEoEntrySet(): rc[0x %x]", rc));
        return rc;
    }

    if(switchover)
    {
        rc = clCkptActiveReplicaSetSwitchOver(pMasterEoEntry->hCkpt);
    }
    else
    {
        rc = clCkptActiveReplicaSet(pMasterEoEntry->hCkpt);
    }
    if (CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clCkptActiveReplicaSet(): rc[%#x],switchover flag [%d]", rc, switchover));
        return rc;
    }

    rc = clCkptSectionIterationInitialize(pMasterEoEntry->hCkpt,
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
        if( CL_OK != rc)
        {
            break;
        }

        logReadFlag         = CL_TRUE;
        if( pCommonEoEntry->masterAddr == clIocLocalAddressGet() )
        {
            ioVector.sectionId  = secDescriptor.sectionId;
            ioVector.dataBuffer = NULL;
            ioVector.dataSize   = 0;
            ioVector.readSize   = 0;
            ioVector.dataOffset = 0;
            clLogNotice(CL_LOG_AREA_MASTER, CL_LOG_CTX_CKPT_READ, 
                       "Got section [%.*s] to be read", 
                       secDescriptor.sectionId.idLen, secDescriptor.sectionId.id);
            if( 0 != strncmp((ClCharT *) ioVector.sectionId.id,
                             (ClCharT *) gLogMasterCompDataSectionId.id,
                             gLogMasterCompDataSectionId.idLen) )
            {
                rc = clCkptCheckpointRead(pMasterEoEntry->hCkpt, &ioVector, 1,
                                          &errIndex);
                if( CL_OK == rc ) /* create whatever we can */
                {
                    if( 0 == strncmp((ClCharT *) ioVector.sectionId.id,
                                     (ClCharT *) gLogMasterDefaultSectionId.id,
                                      gLogMasterDefaultSectionId.idLen) )
                    {
                        rc = clLogMasterEoEntryRecover(pMasterEoEntry, &ioVector,
                                &errIndex);
                        if( CL_OK != rc )
                        {
                            clHeapFree(ioVector.dataBuffer);
                            clHeapFree(ioVector.sectionId.id);
                            break; /* break out of the loop, can't continue */
                        }
                        clHeapFree(ioVector.dataBuffer);
                    }
                    else
                    {
                        /* create whatever we can */
                        clLogMasterFileEntryRecover(pMasterEoEntry, &ioVector,
                                &errIndex);
                        clHeapFree(ioVector.dataBuffer);
                    }
                }
            }
            clHeapFree(secDescriptor.sectionId.id);
        }
        else
        {
            return rc;
        }
    }while( (rc == CL_OK) );

    CL_LOG_CLEANUP(clCkptSectionIterationFinalize(hSecIter), CL_OK);

    if( CL_TRUE == logReadFlag )
    {
        ioVector.sectionId  = gLogMasterCompDataSectionId;
        ioVector.dataBuffer = NULL;
        ioVector.dataSize   = 0;
        ioVector.readSize   = 0;
        ioVector.dataOffset = 0;
        rc = clCkptCheckpointRead(pMasterEoEntry->hCkpt, &ioVector, 1, &errIndex);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptCheckpointRead():rc[0x %x]", rc));
            return rc;
        }
        rc = clLogMasterCompTableStateRecover(pMasterEoEntry, ioVector.dataBuffer, 
                                              ioVector.readSize);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("Unable to recreate the state of compTable"));
        }
        clHeapFree(ioVector.dataBuffer);
    }
    else
    {
        secAttr.sectionId      = &gLogMasterDefaultSectionId;
        secAttr.expirationTime = CL_TIME_END;
        rc = clCkptSectionCreate(pMasterEoEntry->hCkpt, &secAttr, NULL, 0);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptSectionCreate(): rc[0x %x]", rc));
            return rc;
        }
        secAttr.sectionId      = &gLogMasterCompDataSectionId;
        secAttr.expirationTime = CL_TIME_END;
        rc = clCkptSectionCreate(pMasterEoEntry->hCkpt, &secAttr, NULL,
                0);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptSectionCreate(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clCkptSectionDelete(pMasterEoEntry->hCkpt,
                                               &gLogMasterDefaultSectionId),
                           CL_OK);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

static ClRcT
clLogMasterEoEntryRecover(ClLogMasterEoDataT      *pMasterEoEntry,
                          ClCkptIOVectorElementT  *pIoVector,
                          ClUint32T               *pErrIndex)
{
    ClRcT            rc          = CL_OK;
    ClBufferHandleT  hEoEntryBuf = CL_HANDLE_INVALID_VALUE;
    ClVersionT version  = {0};

    CL_LOG_DEBUG_TRACE(("Enter: size %lld", pIoVector->readSize));

    rc = clBufferCreate(&hEoEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBufferNBytesWrite(hEoEntryBuf, pIoVector->dataBuffer,
                             pIoVector->readSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    rc = clXdrUnmarshallClVersionT(hEoEntryBuf, &version);

    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClVersionT(): rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
        return rc;
    }

    switch(CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion))
    {

    case CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION):
        {
            rc = clXdrUnmarshallClUint16T(hEoEntryBuf, &(pMasterEoEntry->nextStreamId));
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
                CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);
                return rc;
            }
        }
        break;

    default:
        rc = CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
        clLogError("MASTER", "EO-Recover", "Version [%d.%d.%d] unsupported",
                   version.releaseCode, version.majorVersion, version.minorVersion);
        break;
    }

    CL_LOG_CLEANUP(clBufferDelete(&hEoEntryBuf), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


static ClRcT fileEntryRecoverBaseVersion(ClLogMasterEoDataT *pMasterEoEntry,
                                         ClBufferHandleT hFileEntryBuf)
{
    ClRcT rc = CL_OK;
    ClLogFileKeyT        fileKey       = {{0}};
    ClLogFileKeyT        *pFileKey     = NULL;
    ClLogFileDataT       *pFileData    = NULL;
    
    rc = clLogMasterFileKeyUnpack(&fileKey, hFileEntryBuf);
    if( CL_OK != rc )
    {
        return rc;
    }
    clLogInfo(CL_LOG_AREA_MASTER, CL_LOG_CTX_CKPT_READ, 
              "Recreating files fileName: %.*s fileLocation: %.*s", 
              fileKey.fileName.length, fileKey.fileName.pValue, 
              fileKey.fileLocation.length, fileKey.fileLocation.pValue);

    rc = clLogFileKeyCreate(&fileKey.fileName, &fileKey.fileLocation,
                            pMasterEoEntry->maxFiles, &pFileKey);
    if( CL_OK != rc )
    {
        clHeapFree(fileKey.fileName.pValue);
        clHeapFree(fileKey.fileLocation.pValue);
        return rc;
    }
    clHeapFree(fileKey.fileName.pValue);
    clHeapFree(fileKey.fileLocation.pValue);

    /* find out the filelocation is available */
    pFileData = clHeapCalloc(1, sizeof(ClLogFileDataT));
    if( NULL == pFileData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clLogFileKeyDestroy(pFileKey);
        return rc;
    }

    pFileData->nActiveStreams = 0;
    rc = clLogMasterFileDataRecreate(pFileData, hFileEntryBuf);
    if( CL_OK != rc )
    {
        clLogFileKeyDestroy(pFileKey);
        return rc;
    }

    rc = clCntNodeAdd(pMasterEoEntry->hMasterFileTable,
                      (ClCntKeyHandleT) pFileKey, 
                      (ClCntDataHandleT) pFileData, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAdd()"));
        CL_LOG_CLEANUP(clCntDelete(pFileData->hStreamTable), CL_OK); //FIXME
        clHeapFree(pFileData);
        clLogFileKeyDestroy(pFileKey);
        return rc;
    }

    return rc;
}

static ClRcT
clLogMasterFileEntryRecover(ClLogMasterEoDataT      *pMasterEoEntry,
                            ClCkptIOVectorElementT  *pIoVector,
                            ClUint32T               *pErrIndex)
{
    ClRcT                rc            = CL_OK;
    ClBufferHandleT      hFileEntryBuf = CL_HANDLE_INVALID_VALUE;
    ClVersionT version = {0};

    CL_LOG_DEBUG_TRACE(("Enter: size %lld", pIoVector->readSize));

    rc = clBufferCreate(&hFileEntryBuf);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBufferNBytesWrite(hFileEntryBuf, pIoVector->dataBuffer,
                             pIoVector->readSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        return rc;
    }
    
    rc = clXdrUnmarshallClVersionT(hFileEntryBuf, &version);
    
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClVersionT(): rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);
        return rc;
    }
    
    switch(CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion))
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION):
        {
            rc = fileEntryRecoverBaseVersion(pMasterEoEntry, hFileEntryBuf);
        }
        break;
    default:
        rc = CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
        clLogError("FILE", "RECOVER", "Version [%d.%d.%d] unsupported",
                   version.releaseCode, version.majorVersion, version.minorVersion);
        break;
    }

    CL_LOG_CLEANUP(clBufferDelete(&hFileEntryBuf), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileKeyUnpack(ClLogFileKeyT    *pFileKey,
                         ClBufferHandleT  hFileEntryBuf)
{
    ClRcT rc = CL_OK;
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClStringT(hFileEntryBuf, &(pFileKey->fileLocation));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClStringT(): rc[0x%x]", rc));
        return rc;
    }

    rc = clXdrUnmarshallClStringT(hFileEntryBuf, &(pFileKey->fileName));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClStringT(): rc[0x%x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

static ClRcT
clLogMasterFileDataRecreate(ClLogFileDataT   *pFileData,
                            ClBufferHandleT  hFileEntryBuf)
{
    ClRcT                  rc              = CL_OK;
    ClUint32T              cntSize         = 0;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClUint32T              count           = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
      return rc;
    }

    rc = VDECL_VER(clXdrUnmarshallClLogStreamAttrIDLT, 4, 0, 0)(hFileEntryBuf,
                                            &(pFileData->streamAttr));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR((
            "clXdrUnmarshallClLogStreamAttributesT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(hFileEntryBuf, &cntSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntHashtblCreate(pCommonEoEntry->maxStreams,
                            clLogStreamKeyCompare, clLogStreamHashFn,
                            clLogMasterStreamEntryDeleteCb,
                            clLogMasterStreamEntryDeleteCb, CL_CNT_UNIQUE_KEY,
                            &(pFileData->hStreamTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtbleCreate(): rc[0x %x]\n", rc));
        return rc;
    }

    for(count = 0; count < cntSize; count++)
    {
        rc = clLogMasterStreamTableRecreate(pCommonEoEntry, hFileEntryBuf,
                                            pFileData);
        if( CL_OK != rc )
        {
           /* Just keep on create as much as u can */ 
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClBoolT
clLogMasterStreamIsValid(ClNameT  *pNodeName)
{
    ClCpmSlotInfoT  slotInfo  = {0};

    clNameCopy(&slotInfo.nodeName, pNodeName);
    if( CL_OK == (clCpmSlotGet(CL_CPM_NODENAME, &slotInfo)) )
    {
        return CL_TRUE;
    }
    return CL_FALSE;
}

static ClRcT
clLogMasterStreamTableRecreate(ClLogSvrCommonEoDataT  *pCommonEoEntry,
                               ClBufferHandleT        hFileEntryBuf,
                               ClLogFileDataT         *pFileData)
{
    ClRcT                   rc              = CL_OK;
    ClLogStreamKeyT         streamKey       = {{0}};
    ClLogStreamKeyT         *pStreamKey     = NULL;
    ClLogMasterStreamDataT  *pStreamData    = NULL;
    ClCntNodeHandleT        hStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClUint32T               bitNum          = 0;
    ClLogMasterEoDataT      *pMasterEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
      return rc;
    }

    rc = clXdrUnmarshallClNameT(hFileEntryBuf, &(streamKey.streamName));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clXdrUnmarshallClNameT(hFileEntryBuf, &(streamKey.streamScopeNode));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamKeyCreate(&streamKey.streamName, &streamKey.streamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    pStreamData = clHeapCalloc(1, sizeof(ClLogMasterStreamDataT));
    if( NULL == pStreamData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clXdrUnmarshallClUint64T(hFileEntryBuf, &(pStreamData->streamMcastAddr));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint64T(): rc[0x %x]", rc));
        clHeapFree(pStreamData);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clXdrUnmarshallClUint16T(hFileEntryBuf, &(pStreamData->streamId));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint16T(): rc[0x %x]", rc));
        clHeapFree(pStreamData);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clCntNodeAddAndNodeGet(pFileData->hStreamTable, 
                                (ClCntKeyHandleT) pStreamKey,
                                (ClCntDataHandleT) pStreamData, 
                                NULL, &hStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAdd(): rc[0x %x]", rc));
        clHeapFree(pStreamData);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    /* Find out the partiuclar stream is still valid */
    if( strncmp(gStreamScopeGlobal, pStreamKey->streamScopeNode.value,
                pStreamKey->streamScopeNode.length)
        &&
        CL_FALSE == clLogMasterStreamIsValid(&pStreamKey->streamScopeNode))
    {
        /*
         * marking this as invalid by assigining CL_IOC_RESERVED_ADDRESS
         * so that streamMcastAddr will not be allocated & bit will not be set 
         * numActive streams count also will be valid 
         */
        pStreamData->streamMcastAddr = CL_IOC_RESERVED_ADDRESS;
        clLogNotice(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
                     "Invalidating the stream [%.*s:%.*s]", 
                     pStreamKey->streamScopeNode.length, pStreamKey->streamScopeNode.value, 
                     pStreamKey->streamName.length, pStreamKey->streamName.value);
    }
    if( CL_IOC_RESERVED_ADDRESS != pStreamData->streamMcastAddr )
    {
        bitNum = pStreamData->streamMcastAddr - pMasterEoEntry->startMcastAddr;
        CL_LOG_DEBUG_TRACE(("bitNUm: %d  pStreamData->streamMcastAddr : %lld \n", bitNum,
                pStreamData->streamMcastAddr));
        rc = clBitmapBitSet(pMasterEoEntry->hAllocedAddrMap, bitNum);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapBitClear()"));
            CL_LOG_CLEANUP(clCntNodeDelete(pFileData->hStreamTable, hStreamNode), CL_OK);
        }
        pFileData->nActiveStreams++;
        clLogInfo(CL_LOG_AREA_MASTER, CL_LOG_CTX_CKPT_READ, 
                  "Active stream [%.*s:%.*s] has been added",
                   pStreamKey->streamScopeNode.length, pStreamKey->streamScopeNode.value,
                   pStreamKey->streamName.length, pStreamKey->streamName.value);
    }
    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogMasterCompData2BufferGet(ClLogCompDataT  *pCompData,
                              ClUint8T        **ppBuffer,
                              ClUint32T       *pDataSize)
{
    ClRcT            rc  = CL_OK;
    ClBufferHandleT  msg = CL_HANDLE_INVALID_VALUE;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferCreate(&msg);
    if(CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    
    rc = VDECL_VER(clXdrMarshallClLogCompDataT, 4, 0, 0)(pCompData, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMarshallClLogCompDataT(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clBufferLengthGet(msg, pDataSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    *ppBuffer = clHeapCalloc(*pDataSize, sizeof(ClUint8T));
    if( NULL == *ppBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clBufferNBytesRead(msg, *ppBuffer, pDataSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        clHeapFree(*ppBuffer);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogMasterCompDataCheckpoint(ClLogMasterEoDataT   *pMasterEoEntry, 
                              ClLogCompDataT       *pCompData)
{
    ClRcT                   rc           = CL_OK;
    ClCkptIOVectorElementT  ioVector[3]  = {{{0}}};
    ClUint32T               numComps     = pMasterEoEntry->numComps;
    ClUint32T               compCnt      = 0; 
    ClUint32T               dataSize     = 0;
    ClUint8T                *pDataBuffer = NULL;
    ClUint32T               dataOffset   = 0;
    ClUint32T               errIdx       = 0;
    ClUint32T               versionCode = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterCompData2BufferGet(pCompData, &pDataBuffer, &dataSize);
    if( CL_OK != rc )
    {
        return rc;
    }

    versionCode = CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION);
    versionCode = htonl(versionCode);

    ++numComps;
    compCnt                = htonl(numComps);

    ioVector[0].sectionId = gLogMasterCompDataSectionId;
    ioVector[0].dataBuffer = (ClUint8T*)&versionCode;
    ioVector[0].dataSize = sizeof(versionCode);
    ioVector[0].readSize = 0;
    ioVector[0].dataOffset = 0;

    ioVector[1].sectionId  = gLogMasterCompDataSectionId;
    ioVector[1].dataBuffer = (ClUint8T *) &compCnt;
    ioVector[1].dataSize   = sizeof(numComps);
    ioVector[1].readSize   = 0;
    ioVector[1].dataOffset = sizeof(versionCode);

    ioVector[2].sectionId  = gLogMasterCompDataSectionId;
    dataOffset = (pMasterEoEntry->numComps * CL_LOG_COMP_SEC_OFFSET) +
        sizeof(numComps) + sizeof(versionCode);
    ioVector[2].dataBuffer = pDataBuffer;
    ioVector[2].dataSize   = dataSize; 
    ioVector[2].readSize   = 0;
    ioVector[2].dataOffset = dataOffset;

    rc = clCkptCheckpointWrite(pMasterEoEntry->hCkpt, ioVector, 3, &errIdx);
    if( (CL_OK != rc) && (CL_ERR_TRY_AGAIN != CL_GET_ERROR_CODE(rc)) )
    {
        CL_LOG_DEBUG_ERROR(("clCkptCheckpointWrite(): rc[0x %x]", rc));
        clHeapFree(pDataBuffer);
        return rc;
    }
    pMasterEoEntry->numComps = numComps;
    clHeapFree(pDataBuffer);
    
    CL_LOG_DEBUG_TRACE(("curr Comp Cnt: %d ", numComps));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT 
compTableStateRecoverBaseVersion(ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClUint32T        numComps = 0;
    ClUint32T        count    = 0;
    ClLogCompDataT   compData = {{0}};

    rc = clXdrUnmarshallClUint32T(msg, &numComps);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Num comps received: %d", numComps));
    
    for( count = 0; count < numComps; count++)
    {
        rc = VDECL_VER(clXdrUnmarshallClLogCompDataT, 4, 0, 0)(msg, &compData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogMarshallClLogCompDataT(): rc[0x %x]", rc));
            return rc;
        }
        rc = clLogMasterCompEntryUpdate(&compData.compName, &compData.clientId, 
                                        CL_TRUE);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterCompIdChkNGet, 4, 0, 0)(): rc[0x %x]", rc));
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogMasterCompTableStateRecover(ClLogMasterEoDataT  *pMasterEoEntry, 
                                 ClUint8T            *pBuffer, 
                                 ClUint32T           dataSize)
{
    ClRcT            rc       = CL_OK;
    ClBufferHandleT  msg      = CL_HANDLE_INVALID_VALUE;
    ClUint32T versionCode = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferCreate(&msg);
    if(CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, dataSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg, &versionCode);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[%#x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    switch(versionCode)
    {

    case CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION):
        {
            rc = compTableStateRecoverBaseVersion(msg);
        }
        break;
        
    default:
        clLogError("COMP", "TBL-RECOVER", "Version [%d.%d.%d] is not supported",
                   CL_VERSION_RELEASE(versionCode), CL_VERSION_MAJOR(versionCode),
                   CL_VERSION_MINOR(versionCode));
        rc = CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
    }

    clBufferDelete(&msg);

    return rc;
}

 
