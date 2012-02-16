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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <clCommon.h>
#include <clXdrApi.h>
#include <clCkptExtApi.h>
#include <clOsalApi.h>
#include <clIdlApi.h>

#include <clLogErrors.h>
#include <clLogSvrCkpt.h>
#include <clLogStreamOwnerCkpt.h>
#include <clLogServer.h>
#include <clLogFlusher.h>
#include <clLogOsal.h>

const ClNameT 
gSvrLocalCkptName = {sizeof("clLogServerLocalCkpt") - 1, "clLogServerLocalCkpt"};

static ClRcT
clLogSvrStreamEntryPack(ClUint32T  dsId,
                        ClAddrT    *ppBuffer,
                        ClUint32T  *pBuffSize,
                        ClPtrT     cookie);

static ClRcT
clLogSvrStreamEntryRecreate(ClUint32T  dsId,
                            ClAddrT    pBuffer,
                            ClUint32T  buffSize,
                            ClPtrT     cookie);

ClRcT
clLogCkptDsIdGetCleanup(ClLogSvrEoDataT  *pSvrEoEntry, 
                        ClUint32T        dsId)
{
    ClRcT   rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pSvrEoEntry->nextDsId--;
    rc = clBitmapBitClear(pSvrEoEntry->hDsIdMap, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]", rc));
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
/*************** */

static ClRcT
clLogSvrStreamEntryUnpackNAdd(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                              CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                              CL_IN  ClBufferHandleT         msg,
                              CL_IN  ClUint32T               dsId);

static ClRcT
clLogSvrCompTablePack(CL_IN  ClCntKeyHandleT   key,
                      CL_IN  ClCntDataHandleT  data,
                      CL_IN  ClCntArgHandleT   arg,
                      CL_IN  ClUint32T         size);
static ClRcT
clLogSvrDsIdMapPack(CL_IN   ClUint32T  dsId,
                    CL_OUT  ClAddrT    *ppBuffer,
                    CL_OUT  ClUint32T  *pBuffSize,
                    CL_IN   ClPtrT      cookie);

static  ClRcT
clLogSvrDsIdMapRecreate(CL_IN  ClUint32T  dsId,
                        CL_IN  ClAddrT    pBuffer,
                        CL_IN  ClUint32T  buffSize,
                        CL_IN  ClPtrT     pCookie);

static ClRcT
clLogSvrShmOpenNFlusherCreate(CL_IN  ClLogSvrEoDataT   *pSvrEoEntry, 
                              CL_IN  ClNameT           *pStreamName,
                              CL_IN  ClNameT           *pStreamNodeName,
                              CL_IN  ClStringT         *pFileName,
                              CL_IN  ClStringT         *pFileLocation,
                              CL_IN  ClCntNodeHandleT  svrStreamNode,
                              CL_IN  ClUint32T         dsId);

static ClRcT
clLogSvrCompEntryRecreate(CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry, 
                          CL_IN  ClLogSvrEoDataT        *pSvrEoEntry, 
                          CL_IN  ClCntNodeHandleT       svrStreamNode,
                          CL_IN  ClBufferHandleT        msg,
                          CL_IN  ClUint32T              compTableSize);

static ClRcT
clLogCkptDsIdGet(CL_IN   ClLogSvrEoDataT  *pSvrEoEntry, 
                 CL_OUT  ClUint32T        *pDsId);

static ClRcT
clLogSvrStreamEntryRecover(CL_IN  ClBitmapHandleT    hBitmap,
                           CL_IN  ClUint32T          bitNum,
                           CL_IN  void               *pCookie);

static ClRcT
clLogSvrCkptCreate(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                   CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry);

static ClRcT
clLogSvrStreamInfoPack(CL_IN  ClLogSvrEoDataT     *pSvrEoEntry,
                       CL_IN  ClCntNodeHandleT    svrStreamNode,
                       CL_IN  ClBufferHandleT     msg);

ClRcT
clLogSvrStreamCurrentFilterGet(ClCntNodeHandleT  hStreamOwnerNode, 
                               ClNameT           *pStreamName,
                               ClNameT           *pStreamNodeName,
                               ClPtrT            pStreamHeader);
                           
static ClRcT
clLogSvrSOFGResponseProcess(ClLogFilterT  *pStreamFilter,
                            ClPtrT        pCookie);
static ClRcT
clLogSvrStateRecover(ClLogSvrEoDataT        *pSvrEoEntry,
                     ClLogSvrCommonEoDataT  *pSvrCommonEoEntry); 
                                 
/*********************Server Checkpoint functions*******************/

ClRcT
clLogSvrCkptGet(ClLogSvrEoDataT        *pSvrEoEntry,
                ClLogSvrCommonEoDataT  *pSvrCommonEoEntry, 
                ClBoolT                *pLogRestart)
{
    ClRcT    rc         = CL_OK;
    ClBoolT  ckptExists = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
   *pLogRestart = CL_FALSE;
    rc = clCkptLibraryDoesCkptExist(pSvrCommonEoEntry->hLibCkpt,
                                    (ClNameT *) &gSvrLocalCkptName, 
                                    &ckptExists);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryDoesCkptExist(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSvrCkptCreate(pSvrEoEntry, pSvrCommonEoEntry);
    
    if(CL_OK != rc)
    {
        return rc;
    }

    if( CL_TRUE == ckptExists )/*Log Service restart*/
    {
        *pLogRestart = CL_TRUE;
        rc = clLogSvrStateRecover(pSvrEoEntry, pSvrCommonEoEntry);
        if( CL_OK != rc )
        {
            /*
             * Just restart incase there were no streamowners checkpointed in the first place
             */
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            {
                rc = CL_OK;
                *pLogRestart = CL_FALSE;
            }
            else
            {
                CL_LOG_DEBUG_ERROR(("clCkptLibraryDoesCkptExist(): rc[0x %x]", rc));
                return rc;
            }
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
    
static ClRcT
clLogSvrStateRecover(ClLogSvrEoDataT        *pSvrEoEntry,
                     ClLogSvrCommonEoDataT  *pSvrCommonEoEntry) 
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    /* this will internaly call deserializer, clLogSvrDsIdMapRecreate() */
    rc = clCkptLibraryCkptDataSetRead(pSvrCommonEoEntry->hLibCkpt,
                                      (ClNameT *) &gSvrLocalCkptName,
                                      CL_LOG_DSID_START, 
                                      CL_HANDLE_INVALID_VALUE);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_CKPT_READ, 
                   "Checkpoint data set [%d] read failed rc[0x %x]", 
                   CL_LOG_DSID_START, rc);
        return rc;
    }

    rc = clBitmapWalkUnlocked(pSvrEoEntry->hDsIdMap, 
                              clLogSvrStreamEntryRecover, NULL);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_CKPT_READ, 
                   "Bitmap walk failed with rc[0x %x]", rc);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrStreamEntryRecover(ClBitmapHandleT  hBitmap,
                           ClUint32T        dsId,
                           void             *pCookie)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL; 
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClBoolT dataSetExists = CL_TRUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCkptLibraryDoesDatasetExist(pSvrCommonEoEntry->hLibCkpt,
                                       (ClNameT *)&gSvrLocalCkptName,
                                       dsId, &dataSetExists);

    if(rc != CL_OK || !dataSetExists)
    {
        CL_LOG_DEBUG_ERROR(("Dataset [%u] doesnt exist. Continuing", dsId));
        return CL_OK;
    }

    rc = clCkptLibraryCkptDataSetCreate(pSvrCommonEoEntry->hLibCkpt, 
                                        (ClNameT *) &gSvrLocalCkptName, 
                                        dsId, 0, 0, 
                                        clLogSvrStreamEntryPack, 
                                        clLogSvrStreamEntryRecreate);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetCreate(): rc[0x %x]",
                            rc));
        return rc;
    }    

    /* this will internaly call deserializer, clLogSvrStreamEntryRecreate() */
    rc = clCkptLibraryCkptDataSetRead(pSvrCommonEoEntry->hLibCkpt, 
                                      (ClNameT *) &gSvrLocalCkptName, dsId, 
                                      CL_HANDLE_INVALID_VALUE);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetRead(): rc[0x %x]", rc));
        CL_LOG_DEBUG_ERROR(("Unable to recover data from this dsId: %u. "
                            "Continuing", dsId));
        return CL_OK;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrCkptCreate(ClLogSvrEoDataT        *pSvrEoEntry,
                   ClLogSvrCommonEoDataT  *pSvrCommonEoEntry)
{    
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clCkptLibraryCkptCreate(pSvrCommonEoEntry->hLibCkpt, 
                                 (ClNameT *) &gSvrLocalCkptName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptCreate(): rc[0x %x]", rc));
        return rc;
    }
    
    rc = clCkptLibraryCkptDataSetCreate(pSvrCommonEoEntry->hLibCkpt,
                                        (ClNameT *) &gSvrLocalCkptName, 
                                        CL_LOG_DSID_START, 0, 0, 
                                        clLogSvrDsIdMapPack,
                                        clLogSvrDsIdMapRecreate);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDelete(): rc[0x %x]", rc));
        clCkptLibraryCkptDelete(pSvrCommonEoEntry->hLibCkpt, 
                                (ClNameT *) &gSvrLocalCkptName);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrDsIdMapPack(ClUint32T  dsId,
                    ClAddrT    *ppBuffer,
                    ClUint32T  *pBuffSize,
                    ClPtrT      cookie)
{
    ClRcT            rc           = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry = (ClLogSvrEoDataT *) cookie;
    ClBufferHandleT  inMsg        = CL_HANDLE_INVALID_VALUE; 

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == ppBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pSvrEoEntry), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((CL_LOG_DSID_START != dsId),
                      CL_LOG_RC(CL_ERR_INVALID_PARAMETER));

    rc = clBufferCreate(&inMsg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clXdrMarshallClUint32T(&pSvrEoEntry->nextDsId, inMsg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        clBufferDelete(&inMsg);
        return rc;
    }
    rc = clLogBitmapPack(pSvrEoEntry->hDsIdMap, inMsg);
    if( CL_OK != rc )
    {
        clBufferDelete(&inMsg);
        return rc;
    }
    rc = clBufferLengthGet(inMsg, pBuffSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        clBufferDelete(&inMsg);
        return rc;
    }
    *ppBuffer = clHeapCalloc(*pBuffSize, sizeof(ClUint8T));
    if( NULL == *ppBuffer )
    {
        clBufferDelete(&inMsg);
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesRead(inMsg,(ClUint8T *) *ppBuffer, pBuffSize);   
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        clHeapFree(*ppBuffer);
        clBufferDelete(&inMsg);
        return rc;
    }    
    CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

static ClRcT
clLogSvrDsIdMapRecreate(ClUint32T  dsId,
                        ClAddrT    pBuffer,
                        ClUint32T  buffSize,
                        ClPtrT     pCookie)
{
    ClRcT            rc           = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry = NULL; 
    ClBufferHandleT  inMsg        = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((CL_LOG_DSID_START != dsId),
                      CL_LOG_RC(CL_ERR_INVALID_PARAMETER));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clBufferCreate(&inMsg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(inMsg, (ClUint8T *) pBuffer, buffSize);   
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(inMsg, &pSvrEoEntry->nextDsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("DsIdCnt: %d", pSvrEoEntry->nextDsId));

    rc = clLogBitmapUnpack(inMsg, pSvrEoEntry->hDsIdMap);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogBitmapUnpack(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
   
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*FIXME serializer, deserializer for svrStreamData checkpoint*/


static ClRcT
clLogSvrStreamEntryPack(ClUint32T  dsId,
                        ClAddrT    *ppBuffer,
                        ClUint32T  *pBuffSize,
                        ClPtrT     cookie)
{
    ClRcT             rc            = CL_OK;
    ClBufferHandleT   msg           = CL_HANDLE_INVALID_VALUE; 
    ClCntNodeHandleT  svrStreamNode = cookie;
    ClLogSvrEoDataT   *pSvrEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == ppBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pBuffSize), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clLogSvrStreamInfoPack(pSvrEoEntry, svrStreamNode, msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamInfoPack(): rc[0x %x]", rc));
        clBufferDelete(&msg);
        return rc;
    }    
    rc = clBufferLengthGet(msg, pBuffSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        clBufferDelete(&msg);
        return rc;
    }
    *ppBuffer = clHeapCalloc(*pBuffSize, sizeof(ClUint8T));
    if( NULL == *ppBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        clBufferDelete(&msg);
        return rc;
    }
    rc = clBufferNBytesRead(msg, (ClUint8T *)*ppBuffer, pBuffSize);   
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        clHeapFree(*ppBuffer);
        clBufferDelete(&msg);
        return rc;
    }    
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

static ClRcT
clLogSvrStreamInfoPack(ClLogSvrEoDataT     *pSvrEoEntry,
                       ClCntNodeHandleT    svrStreamNode,
                       ClBufferHandleT     msg)
{
    ClRcT                rc              = CL_OK;
    ClUint32T            size            = 0;
    ClLogStreamKeyT      *pStreamKey     = NULL;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode,
                              (ClCntDataHandleT *)&pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForNodeGet(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clCntNodeUserKeyGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode, 
                             (ClCntKeyHandleT *)&pStreamKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clXdrMarshallClNameT(&(pStreamKey->streamName), msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClNameT(&(pStreamKey->streamScopeNode), msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClNameT(): rc[0x %x]", rc));
        return rc;
    }
    /*
     * Copy file name and file location keys to reconstruct back the fileowner address
     */
    clXdrMarshallClStringT(&pSvrStreamData->fileName, msg, 0);

    clXdrMarshallClStringT(&pSvrStreamData->fileLocation, msg, 0);

    rc = clCntSizeGet(pSvrStreamData->hComponentTable, &size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(); rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClUint32T(&size, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }
    rc = clCntWalk(pSvrStreamData->hComponentTable, clLogSvrCompTablePack, 
                   &msg, sizeof(&msg));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrCompTablePack(ClCntKeyHandleT   key,
                      ClCntDataHandleT  data,
                      ClCntArgHandleT   arg,
                      ClUint32T         size)
{
    ClRcT               rc          = CL_OK;
    ClLogSvrCompKeyT    *pCompKey   = (ClLogSvrCompKeyT *)key;
    ClLogSvrCompDataT   *pCompData  = (ClLogSvrCompDataT *)data;
    ClBufferHandleT     msg         = *(ClBufferHandleT *)arg;
                                                                                                                             
    CL_LOG_DEBUG_TRACE(("Enter"));
    rc = clXdrMarshallClUint32T(&(pCompKey->componentId), msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClUint32T(&(pCompData->refCount), msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClUint32T(&(pCompData->portId), msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

static ClRcT
clLogSvrStreamEntryRecreate(ClUint32T  dsId,
                            ClAddrT    pBuffer,
                            ClUint32T  buffSize,
                            ClPtrT     cookie)
{

    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL; 
    ClBufferHandleT        inMsg              = CL_HANDLE_INVALID_VALUE; 
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clBufferCreate(&inMsg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clBufferNBytesWrite(inMsg, (ClUint8T *) pBuffer, buffSize);   
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
        return rc;
    }
    
    rc = clLogSvrStreamEntryUnpackNAdd(pSvrEoEntry, pSvrCommonEoEntry, 
                                       inMsg, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
        return rc;
    }
    
    CL_LOG_CLEANUP(clBufferDelete(&inMsg), CL_OK);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrStreamEntryUnpackNAdd(ClLogSvrEoDataT        *pSvrEoEntry,
                              ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                              ClBufferHandleT         msg,
                              ClUint32T               dsId)
{
    ClRcT             rc              = CL_OK;
    ClNameT           streamName      = {0};
    ClNameT           streamScopeNode = {0};
    ClUint32T         compTableSize   = 0;
    ClCntNodeHandleT  hSvrStreamNode   = CL_HANDLE_INVALID_VALUE;
    ClLogStreamKeyT   *pStreamKey     = NULL;
    ClStringT shmName = {0};
    ClStringT fileName = {0};
    ClStringT fileLocation = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrStreamTableGet(pSvrEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamTableCreate(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clXdrUnmarshallClNameT(msg, &streamName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClNameT(): rc[0x %x]\n", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("streamName: %*s", streamName.length,
                          streamName.value));

    rc = clXdrUnmarshallClNameT(msg, &streamScopeNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClNameT(): rc[0x %x]\n", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("streamScopeNode: %*s", streamScopeNode.length,
                          streamScopeNode.value));

    rc = clXdrUnmarshallClStringT(msg, &fileName);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClStringT(): rc[%#x]\n", rc));
        return rc;
    }
    rc = clXdrUnmarshallClStringT(msg, &fileLocation);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClStringT(): rc[%#x]\n", rc));
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg, &compTableSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]\n", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("compSize : %u", compTableSize));
    
    rc = clLogStreamKeyCreate(&streamName, &streamScopeNode,
                              pSvrCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogShmNameCreate(&streamName,&streamScopeNode,&shmName);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogSvrStreamEntryAdd(pSvrEoEntry, pSvrCommonEoEntry,
                                pStreamKey, &shmName, &hSvrStreamNode);
    clHeapFree(shmName.pValue);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clLogSvrCompEntryRecreate(pSvrCommonEoEntry, pSvrEoEntry,  
                                   hSvrStreamNode, msg, compTableSize);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, 
                                       hSvrStreamNode), CL_OK);
        return rc;
    }

    rc = clLogSvrShmOpenNFlusherCreate(pSvrEoEntry, &streamName, 
                                       &streamScopeNode, &fileName,
                                       &fileLocation, hSvrStreamNode, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrShmOpenAndFlusherCreate(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, 
                       hSvrStreamNode), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
    shmNameCreate();
    map the shared memory;
    populate the Flusher data;
    clLogSvrFlusherThreadCreateNStart();
    populate flusher.taskId
*/
static ClRcT
clLogSvrShmOpenNFlusherCreate(ClLogSvrEoDataT   *pSvrEoEntry, 
                              ClNameT           *pStreamName,
                              ClNameT           *pStreamNodeName,
                              ClStringT         *pFileName,
                              ClStringT         *pFileLocation,
                              ClCntNodeHandleT  hSvrStreamNode,
                              ClUint32T         dsId)
{
    ClRcT                  rc                 = CL_OK;
    ClInt32T               fd                 = 0;
    ClLogStreamHeaderT     *pStreamHeader     = NULL;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClUint32T              headerSize         = 0;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
        
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForNodeGet(): rc[0x %x]\n", rc));
        return rc;
    }
    
    if(pFileLocation->pValue)
        pSvrStreamData->fileOwnerAddr = clLogFileOwnerAddressFetch(pFileLocation);
    else
        pSvrStreamData->fileOwnerAddr = CL_IOC_RESERVED_ADDRESS;

    memcpy(&pSvrStreamData->fileName, pFileName, sizeof(pSvrStreamData->fileName));
    memcpy(&pSvrStreamData->fileLocation, pFileLocation, sizeof(pSvrStreamData->fileLocation));

    rc = clLogShmNameCreate(pStreamName, pStreamNodeName,
                               &(pSvrStreamData->shmName));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrShmNameCreate(): rc[0x %x]\n", rc));
        return rc;
    }

    rc = clOsalShmOpen_L(pSvrStreamData->shmName.pValue, CL_LOG_SHM_OPEN_FLAGS,
                         CL_LOG_SHM_MODE, &fd);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalShmOpen(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pSvrStreamData->shmName), CL_OK);
        return rc;
    }

    rc = clLogSvrShmMapAndGet(fd,(ClUint8T **) &pStreamHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalShmClose_L(fd), CL_OK);
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pSvrStreamData->shmName), CL_OK);
        return rc;
    }
    pSvrStreamData->pStreamHeader = pStreamHeader;

    rc = clOsalShmClose_L(fd);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L((ClPtrT) pStreamHeader, pStreamHeader->shmSize),
                       CL_OK);
        pSvrStreamData->pStreamHeader = NULL;
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pSvrStreamData->shmName), CL_OK);
        return rc;
    }

#if 0
    /*
     * FIXME- by this time, logMaster wont be availble, so postponing this call
     * to some time
     */
    rc = clLogSvrStreamCurrentFilterGet(hSvrStreamNode, pStreamName, 
                                        pStreamNodeName, pStreamHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L((ClPtrT) pStreamHeader, pStreamHeader->shmSize),
                       CL_OK);
        CL_LOG_CLEANUP(clOsalShmUnlink_L(pSvrStreamData->shmName.pValue),
                       CL_OK);
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pSvrStreamData->shmName), CL_OK);
        return rc;
    }
#endif

    pSvrStreamData->dsId           = dsId;
    /* 
     * streamHeader  |--------------------|
     *               |                    | 
     *               |                    |
     *        maxMsgs|--------------------|
     *       maxComps|--------------------|
     * pStreamRecords|--------------------|
     *               |                    |
     *               |--------------------|
     */
    headerSize                     =
        CL_LOG_HEADER_SIZE_GET(pSvrCommonEoEntry->maxMsgs, 
                               pSvrCommonEoEntry->maxComponents);
    pSvrStreamData->pStreamRecords = 
        ((ClUint8T *) (pSvrStreamData->pStreamHeader)) + headerSize; 

    rc = clOsalTaskCreateAttached(pSvrStreamData->shmName.pValue, 
                                  CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
                                  CL_OSAL_MIN_STACK_SIZE, clLogFlusherStart, 
                                  pSvrStreamData, &pSvrStreamData->flusherId);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L((ClPtrT) pStreamHeader, pStreamHeader->shmSize),
                       CL_OK);
        pSvrStreamData->pStreamHeader = NULL;
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pSvrStreamData->shmName), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void 
clLogSvrSOFGResponse(ClIdlHandleT  hLogIdl,
                     ClNameT       *pStreamName,
                     ClNameT       *pStreamScopeNode,
                     ClLogFilterT  *pStreamFilter,
                     ClRcT         retCode,
                     ClPtrT        pCookie)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    CL_ASSERT( NULL != pCookie ); 
    if( CL_OK != retCode )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrSOFGResponse(): rc[0x %x]", retCode));
        return ;
    }
    rc = clLogSvrSOFGResponseProcess(pStreamFilter, pCookie);
   
    clHeapFree(pStreamFilter->pMsgIdSet);
    clHeapFree(pStreamFilter->pCompIdSet);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

static ClRcT
clLogSvrSOFGResponseProcess(ClLogFilterT  *pStreamFilter,
                            ClPtrT        pCookie)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       hStreamOwnerNode   = (ClCntNodeHandleT) pCookie;
    ClLogSvrStreamDataT    *pStreamData       = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hStreamOwnerNode,
                              (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clLogServerStreamMutexLock(pStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);

    /*
     * Log Server will not intimate the client about this filter update,
     * as it is recreating the state for itself and filter may or may 
     * not have been updated during that transient period.
     */
    rc = clLogFilterAssign(pStreamData->pStreamHeader, pStreamFilter);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogFilterAssign(): rc[0x %x]\n", rc));
    }

    CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pStreamData),CL_OK);
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrStreamCurrentFilterGet(ClCntNodeHandleT  hStreamOwnerNode, 
                               ClNameT           *pStreamName,
                               ClNameT           *pStreamNodeName,
                               ClPtrT            pStreamHeader)
{
    ClRcT              rc              = CL_OK;
    ClIdlHandleT       hStreamOwnerIdl = CL_HANDLE_INVALID_VALUE;
    ClLogStreamScopeT  streamScope     = CL_LOG_STREAM_GLOBAL;
        
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamScopeGet(pStreamNodeName, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogSvrIdlHandleInitialize(streamScope, &hStreamOwnerIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrIdlHandleInitialize(): rc[0x %x]\n", rc));
        return rc;
    }

#if 0
    FIXME - streamOwnerShould expose the function
    rc = clLogStreamOwnerFilterGetClientAsync(hStreamOwnerIdl, pStreamName,
                                              streamScope, pStreamNodeName,
                                              &filter, clLogSvrSOFGResponse, 
                                              (void *) hStreamOwnerNode);
#endif
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamOwnerFilterGetClientAsync(): rc[0x %x]\n",
                             rc));
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hStreamOwnerIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrCompEntryRecreate(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry, 
                          ClLogSvrEoDataT        *pSvrEoEntry, 
                          ClCntNodeHandleT       hSvrStreamNode,
                          ClBufferHandleT        msg,
                          ClUint32T              compTableSize)
{
    ClRcT                rc              = CL_OK;
    ClUint32T            count           = 0;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;
    ClLogSvrCompDataT    *pCompData      = NULL;
    ClLogSvrCompKeyT     *pCompKey       = NULL;
        
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                            (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForNodeGet(): rc[0x %x]\n", rc));
        return rc;
    }
    for( count = 0; count < compTableSize; count++ )
    {
        pCompKey = clHeapCalloc(1, sizeof(ClLogSvrCompKeyT));
        if( NULL == pCompKey )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            return CL_LOG_RC(CL_ERR_NULL_POINTER);
        }
        rc = clXdrUnmarshallClUint32T(msg, &pCompKey->componentId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
            clHeapFree(pCompKey);
            return rc;
        }
        pCompKey->hash = pCompKey->componentId % pSvrCommonEoEntry->maxComponents;

        pCompData = clHeapCalloc(1, sizeof(ClLogSvrCompDataT));
        if( NULL == pCompData )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            clHeapFree(pCompKey);
            return CL_LOG_RC(CL_ERR_NULL_POINTER);
        }
        rc = clXdrUnmarshallClUint32T(msg, &pCompData->refCount);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
            clHeapFree(pCompData);
            clHeapFree(pCompKey);
            return rc;
        }
        rc = clXdrUnmarshallClUint32T(msg, &pCompData->portId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
            clHeapFree(pCompData);
            clHeapFree(pCompKey);
            return rc;
        }

        rc = clCntNodeAdd(pSvrStreamData->hComponentTable, 
                          (ClCntKeyHandleT) pCompKey,
                          (ClCntDataHandleT) pCompData, NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeAdd(): rc[0x %x]\n", rc));
            clHeapFree(pCompData);
            clHeapFree(pCompKey);
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrCkptDataSetDelete(ClUint32T         dsId)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
        
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }    

    if( CL_LOG_DSID_START >= dsId )
    {
        CL_LOG_DEBUG_ERROR(("Invalid DataSet ID\n "));
        return CL_LOG_RC(CL_ERR_INVALID_STATE);/*FIXME*/
    }
    rc = clBitmapBitClear(pSvrEoEntry->hDsIdMap, dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetDelete(): rc[0x %x]", rc));
        return rc;
    }    
        
    rc = clCkptLibraryCkptDataSetDelete(pSvrCommonEoEntry->hLibCkpt, 
                                        (ClNameT *) &gSvrLocalCkptName,
                                        dsId); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetDelete(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clCkptLibraryCkptDataSetWrite(pSvrCommonEoEntry->hLibCkpt, 
                                       (ClNameT *) &gSvrLocalCkptName, 
                                       CL_LOG_DSID_START, 
                                       pSvrEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
    }    


    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrStreamCheckpoint(ClLogSvrEoDataT     *pSvrEoEntry,
                         ClCntNodeHandleT    svrStreamNode)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClBoolT                addedDataSet       = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]\n", rc));
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForNodeGet(): rc[0x %x]\n", rc));
        return rc;
    }
    if( CL_LOG_INVALID_DSID == pSvrStreamData->dsId )
    {
        rc = clLogCkptDsIdGet(pSvrEoEntry, &pSvrStreamData->dsId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogCkptDsIdGet(): rc[0x %x]",
                        rc));
            return rc;
        }    
        rc = clCkptLibraryCkptDataSetCreate(pSvrCommonEoEntry->hLibCkpt, 
                                            (ClNameT *) &gSvrLocalCkptName, 
                                            pSvrStreamData->dsId, 0, 0, 
                                            clLogSvrStreamEntryPack, 
                                            clLogSvrStreamEntryRecreate);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetCreate(): rc[0x %x]",
                        rc));
            CL_LOG_CLEANUP(clLogCkptDsIdGetCleanup(pSvrEoEntry, 
                                                   pSvrStreamData->dsId), 
                           CL_OK);
            pSvrStreamData->dsId = CL_LOG_INVALID_DSID;
            return rc;
        }    
        /* 
         * This function will internally call serialiser,
         * clLogSvrDsIdMapPack()
         */ 
        rc = clCkptLibraryCkptDataSetWrite(pSvrCommonEoEntry->hLibCkpt, 
                                           (ClNameT *) &gSvrLocalCkptName, 
                                           CL_LOG_DSID_START, 
                                           pSvrEoEntry);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
            clCkptLibraryCkptDataSetDelete(pSvrCommonEoEntry->hLibCkpt,
                                           (ClNameT *) &gSvrLocalCkptName,
                                           pSvrStreamData->dsId);
            CL_LOG_CLEANUP(clLogCkptDsIdGetCleanup(pSvrEoEntry,
                           pSvrStreamData->dsId), CL_OK);
            pSvrStreamData->dsId = CL_LOG_INVALID_DSID;
            return rc;
        }
        addedDataSet = CL_TRUE;
    }    
    /* This function will internally call, clLogSvrStreamEntryPack() */
    rc = clCkptLibraryCkptDataSetWrite(pSvrCommonEoEntry->hLibCkpt, 
                                       (ClNameT *) &gSvrLocalCkptName, 
                                       pSvrStreamData->dsId, svrStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        if( CL_TRUE == addedDataSet )
        {
            clCkptLibraryCkptDataSetDelete(pSvrCommonEoEntry->hLibCkpt, 
                                           (ClNameT *) &gSvrLocalCkptName, 
                                            pSvrStreamData->dsId);
        }
    }    
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogCkptDsIdGet(ClLogSvrEoDataT  *pSvrEoEntry, 
                 ClUint32T        *pDsId)
{
    ClRcT   rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pSvrEoEntry->nextDsId++;
    *pDsId = pSvrEoEntry->nextDsId;
    rc = clBitmapBitSet(pSvrEoEntry->hDsIdMap, *pDsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
        pSvrEoEntry->nextDsId--;
        return rc;
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    
