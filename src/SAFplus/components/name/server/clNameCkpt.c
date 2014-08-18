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
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clHandleApi.h>
#include <clNameCkptIpi.h>


ClCkptSvcHdlT          gNsCkptSvcHdl = CL_HANDLE_INVALID_VALUE;
extern ClCntHandleT    gNSHashTable;

#define CL_NAME_SVC_CKPT_NAME       "clNameCtxCkpt"

ClRcT
clNameSvcCkptInit(void)
{
    ClRcT    rc        = CL_OK;
    ClNameT  ckptName  = {0};
    ClBoolT  retVal    = CL_FALSE;

    CL_NAME_DEBUG_TRACE(("Enter"));

    clNameSet(&ckptName, CL_NAME_SVC_CKPT_NAME);
    rc = clCkptLibraryInitialize(&gNsCkptSvcHdl);
    if( CL_OK != rc )
    {    
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("clCkptLibraryInitialize(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clCkptLibraryDoesCkptExist(gNsCkptSvcHdl, &ckptName, &retVal);
    if( CL_OK != rc )
    {
        clCkptLibraryFinalize(gNsCkptSvcHdl);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryDoesCkptExist(): rc[0x %x]", rc));
        return rc;
    }    
    if( retVal == CL_TRUE )
    {    
        rc = clNameSvcCkptCreate();
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("clNameSvcCkptCreate returned [%#x]", rc));
            clCkptLibraryFinalize(gNsCkptSvcHdl);
            return rc;
        }
        rc = clNameSvcCkptRead();
    }    
    else
    {
        rc = clNameSvcCkptCreate();
    }    
    if( CL_OK != rc  )
    {
        clCkptLibraryFinalize(gNsCkptSvcHdl);
        return rc;
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcCkptCreate(void)
{    
    ClRcT    rc        = CL_OK;
    ClNameT  ckptName  = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNameSet(&ckptName, CL_NAME_SVC_CKPT_NAME);
    rc = clCkptLibraryCkptCreate(gNsCkptSvcHdl, &ckptName);
    if( CL_OK != rc  )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptCreate(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clCkptLibraryCkptDataSetCreate(gNsCkptSvcHdl, &ckptName,
                                        CL_NAME_CONTEXT_GBL_DSID,
                                        0, 0, clNameContextCkptSerializer,
                                        clNameContextCkptDeserializer);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetCreate(): rc[0x %x]", rc));
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &ckptName,
                CL_NAME_CONTEXT_GBL_DSID);
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcGNSTableWalk(ClCntKeyHandleT  key,
                      ClCntDataHandleT data,
                      ClCntArgHandleT  arg,
                      ClUint32T        dataSize)
{
    ClRcT                  rc        = CL_OK;
    ClNameSvcContextInfoT  *pCtxData = (ClNameSvcContextInfoT *)data;
    ClUint32T              contexId  = (ClUint32T)(ClWordT)key;
    ClIocNodeAddressT      localAddr  = clIocLocalAddressGet();
    ClIocNodeAddressT      masterAddr = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    rc = clCpmMasterAddressGet(&masterAddr);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",rc));
        return rc;
    }

    if( (contexId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) ||
        (localAddr == masterAddr) )
    {    
        rc = clNameSvcEntryRecreate(contexId, pCtxData);
    }
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcCkptRead(void)
{
    ClRcT    rc        = CL_OK;
    ClNameT  ckptName  = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNameSet(&ckptName, CL_NAME_SVC_CKPT_NAME);
    rc = clCkptLibraryCkptDataSetRead(gNsCkptSvcHdl, &ckptName, 
                                     CL_NAME_CONTEXT_GBL_DSID, 0);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetRead(): rc[0x %x]", rc));
        return rc;
    }    
    rc = clCntWalk(gNSHashTable, clNameSvcGNSTableWalk, NULL, 0);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntWalk() rc[0x %x]", rc));
        return rc;
    }    
    CL_NAME_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    

ClRcT
clNameCkptCtxInfoWrite(void)
{
    ClRcT     rc         = CL_OK;
    ClNameT   ckptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNameSet(&ckptName, CL_NAME_SVC_CKPT_NAME);
    /*
     * this internally calls the clNameContextCkptSerializer function 
     * to pack the information
     */
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl,
                                       &ckptName, 
                                       CL_NAME_CONTEXT_GBL_DSID,
                                       0);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
    }    
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcPerCtxDataSetCreate(ClUint32T  contexId,
                             ClUint32T  dsIdCnt)
{
    ClRcT    rc           = CL_OK;
    ClNameT  nsCkptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));

    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);
    rc = clCkptLibraryCkptDataSetCreate(gNsCkptSvcHdl, &nsCkptName,
                                        dsIdCnt, 0, 0,
                                        clNameSvcEntrySerializer,
                                        clNameSvcEntryDeserializer);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetCreate(): rc[0x %x]", rc));
        return rc;
    }    
    clLogInfo("DATA", "CREATE", "Data set [%d] created", dsIdCnt);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameCkptCtxAllDSDelete(ClUint32T  contexId,
                         ClUint32T  dsIdCnt,
                         ClUint8T   *freeDsIdMap,
                         ClUint32T  freeMapSize)
{
    ClRcT     rc           = CL_OK;
    ClNameT   nsCkptName   = {0};
    ClUint32T count        = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);
    for( count = CL_NS_RESERVED_DSID; count < freeMapSize; ++count)
    {
        register ClInt32T j;
        if(!freeDsIdMap[count]) continue;
        for(j = 0; j < 8; ++j)
        {
            ClUint32T dsId = 0;
            if(!(freeDsIdMap[count] & ( 1 << j))) continue;
            dsId = (count << 3) + j;
            rc = clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl,
                                                &nsCkptName,
                                                dsId);
            if( CL_OK != rc )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("clCkptLibraryCkptDataSetDelete for dsId [%d] returned rc[0x %x]", 
                                dsId, rc));
            }    
        }
    }    

    for(count = freeMapSize << 3; count <= dsIdCnt; ++count)
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl,
                                       &nsCkptName,
                                       count);
    }

    clNameSvcPerCtxCkptCreate(contexId, 1);
    clNameCkptCtxInfoWrite();

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Release dsid back to the bitmap.
 */
ClRcT
clNameSvcPerCtxDataSetIdPut(ClUint32T contextId,
                            ClUint32T dsId)
{
    /* Find the appripriate context to look into */
    ClNameSvcContextInfoPtrT pCtxData = NULL;
    ClRcT rc = CL_OK;
    rc = nameSvcContextGet(contextId, &pCtxData);
    if(rc != CL_OK) 
        return rc;
    if(dsId  >= ((ClUint32T)sizeof(pCtxData->freeDsIdMap) << 3))
    {
        if(dsId >= pCtxData->dsIdCntMax)
        {
            pCtxData->dsIdCntMax = --pCtxData->dsIdCnt;
        }
        return CL_OK;
    }
    pCtxData->freeDsIdMap[dsId >> 3] &= ~(1 << (dsId & 7));
    return CL_OK;
}

ClRcT
clNameSvcPerCtxDataSetIdGet(ClNameSvcContextInfoT *pCtxData,
                            ClUint32T             *pDsIdCnt)
{
    ClRcT  rc = CL_OK;
    ClUint32T dsId = 0;
    register ClInt32T i;
    CL_NAME_DEBUG_TRACE(("Enter"));
    for(i = 0; i < sizeof(pCtxData->freeDsIdMap); ++i)
    {
        register ClInt32T j;
        if(pCtxData->freeDsIdMap[i] == 0xff) continue;
        for(j = 0; j < 8; ++j)
        {
            if(!(pCtxData->freeDsIdMap[i] & ( 1 << j)))
            {
                pCtxData->freeDsIdMap[i] |= ( 1 << j );
                dsId = (i << 3 ) + j;
                if(dsId >= CL_NS_RESERVED_DSID) 
                    goto update_dsid;
            }
        }
    }
    /*  
     * Bitmap is full. Get one from the running counter datasets.
     */
    if(pCtxData->dsIdCnt < CL_NS_RESERVED_DSID)
    {
        /*
         * Initialize start dsId counter.
         */
        dsId = (ClUint32T)sizeof(pCtxData->freeDsIdMap) << 3;
        pCtxData->dsIdCnt = dsId;
    }
    else
    {
        CL_ASSERT(pCtxData->dsIdCnt + 1 > pCtxData->dsIdCnt);
        dsId = ++pCtxData->dsIdCnt;
    }

    pCtxData->dsIdCntMax = dsId;

    update_dsid:
    *pDsIdCnt = dsId;
    CL_NAME_DEBUG_TRACE(("Exit: *pDsIdCnt [%d]\n", *pDsIdCnt));
    return rc;
}    
        

ClRcT
clNameSvcPerCtxInfoWrite(ClUint32T              contexId,
                         ClNameSvcContextInfoT  *pCtxData) 
{
    ClRcT          rc           = CL_OK;
    ClNameT        nsCkptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));

    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId); 
    /* 
     * this will internally calls clNameSvcPerCtxSerializer to pack 
     * the info 
     */
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl, &nsCkptName,
                                       CL_NS_PER_CTX_DSID, 
                                       pCtxData);
    if( CL_OK != rc )
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName, 
                                       CL_NS_PER_CTX_DSID); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        return rc;
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcBindingDataWrite(ClUint32T              contexId,
                          ClNameSvcContextInfoT  *pCtxData,
                          ClNameSvcBindingT      *pBindData)
{
    ClRcT          rc           = CL_OK;
    ClNsEntryPackT nsEntryInfo  = {0};
    ClUint32T      dsIdCnt      = 0;
    ClNameT        nsCkptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);

    clNameSvcPerCtxDataSetIdGet(pCtxData, &dsIdCnt);
    rc = clNameSvcPerCtxDataSetCreate(contexId, dsIdCnt);
    if( CL_OK != rc )
    {
        return rc;
    }    
    nsEntryInfo.type    = CL_NAME_SVC_ENTRY;
    clNameCopy(&nsEntryInfo.nsInfo.name, &pBindData->name);
    nsEntryInfo.nsInfo.priority = pBindData->priority;
    nsEntryInfo.nsInfo.dsId     = dsIdCnt;
    pBindData->dsId             = dsIdCnt;
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl, &nsCkptName,
                                       dsIdCnt, &nsEntryInfo);
    if( CL_OK != rc )
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName, dsIdCnt); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        return rc;
    }    
    
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcBindingDetailsWrite(ClUint32T                 contexId,
                             ClNameSvcContextInfoT     *pCtxData,
                             ClNameSvcBindingT         *pBindData,
                             ClNameSvcBindingDetailsT  *pBindDetail)
{
    ClRcT          rc             = CL_OK;
    ClNsEntryPackT *pNsEntryInfo  = NULL;
    ClUint32T      dsIdCnt        = 0;
    ClNameT        nsCkptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);

    clNameSvcPerCtxDataSetIdGet(pCtxData, &dsIdCnt);
    rc = clNameSvcPerCtxDataSetCreate(contexId, dsIdCnt);
    if( CL_OK != rc )
    {
        return rc;
    }    
    pNsEntryInfo = (ClNsEntryPackT *)clHeapCalloc(1, sizeof(ClNsEntryPackT));
    if( NULL == pNsEntryInfo )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc()"));
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }    
    pNsEntryInfo->type    = CL_NAME_SVC_BINDING_DATA;
    clNameCopy(&pNsEntryInfo->nsInfo.name, &pBindData->name);
    pNsEntryInfo->nsInfo.objReference = pBindDetail->objReference;
    pNsEntryInfo->nsInfo.compId       = pBindDetail->compId.compId;
    pNsEntryInfo->nsInfo.eoID       = pBindDetail->compId.eoID;
    pNsEntryInfo->nsInfo.nodeAddress       = pBindDetail->compId.nodeAddress;
    pNsEntryInfo->nsInfo.clientIocPort       = pBindDetail->compId.clientIocPort;
    pNsEntryInfo->nsInfo.priority     = pBindDetail->compId.priority;
    pNsEntryInfo->nsInfo.attrCount    = pBindDetail->attrCount;
    pNsEntryInfo->nsInfo.attrLen      = pBindDetail->attrLen;
    pNsEntryInfo->nsInfo.dsId         = dsIdCnt;
    if(pBindDetail->attrCount > 0 )
    {    
        pNsEntryInfo->nsInfo.attr = clHeapCalloc(1, pBindDetail->attrLen);
        if( NULL == pNsEntryInfo->nsInfo.attr )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc"));
            clHeapFree(pNsEntryInfo);
            return CL_NS_RC(CL_ERR_NO_MEMORY);
        }
        memcpy(pNsEntryInfo->nsInfo.attr, pBindDetail->attr,
                pBindDetail->attrLen);
    }
    pBindDetail->dsId                = dsIdCnt;
    pBindDetail->compId.dsId         = dsIdCnt;
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl, &nsCkptName,
                                       dsIdCnt, pNsEntryInfo);
    if( CL_OK != rc )
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName, dsIdCnt); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        return rc;
    }    
    if( pBindDetail->attrCount > 0 )
    {
        clHeapFree(pNsEntryInfo->nsInfo.attr);
    }
    clHeapFree(pNsEntryInfo);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT clNameSvcCompInfoOverwrite(ClUint32T contextId, ClUint32T dsId, ClNameSvcBindingT *pBindData, ClNameSvcBindingDetailsT *pBindDetail)
{

    ClRcT rc = CL_OK;
    ClNameT nsCkptName = {0};
    ClNsEntryPackT *pNsEntryInfo = NULL;

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contextId);

    pNsEntryInfo = (ClNsEntryPackT *)clHeapCalloc(1, sizeof(ClNsEntryPackT));
    if( NULL == pNsEntryInfo )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc()"));
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }
    pNsEntryInfo->type    = CL_NAME_SVC_COMP_INFO;
    clNameCopy(&pNsEntryInfo->nsInfo.name, &pBindData->name);
    pNsEntryInfo->nsInfo.objReference = pBindDetail->objReference;
    pNsEntryInfo->nsInfo.compId       = pBindDetail->compId.compId;
    pNsEntryInfo->nsInfo.eoID       = pBindDetail->compId.eoID;
    pNsEntryInfo->nsInfo.nodeAddress       = pBindDetail->compId.nodeAddress;
    pNsEntryInfo->nsInfo.clientIocPort       = pBindDetail->compId.clientIocPort;
    pNsEntryInfo->nsInfo.priority     = pBindDetail->compId.priority;
    pNsEntryInfo->nsInfo.attrCount    = pBindDetail->attrCount;
    pNsEntryInfo->nsInfo.attrLen      = pBindDetail->attrLen;
    pNsEntryInfo->nsInfo.dsId         = dsId;
    if(pBindDetail->attrCount > 0 )
    {
        pNsEntryInfo->nsInfo.attr = clHeapCalloc(1, pBindDetail->attrLen);
        if( NULL == pNsEntryInfo->nsInfo.attr )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc"));
            clHeapFree(pNsEntryInfo);
            return CL_NS_RC(CL_ERR_NO_MEMORY);
        }
        memcpy(pNsEntryInfo->nsInfo.attr, pBindDetail->attr, pBindDetail->attrLen);
    }
    pBindDetail->compId.dsId         = dsId;
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl, &nsCkptName, dsId, pNsEntryInfo);
    if( CL_OK != rc )
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName, dsId);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        return rc;
    }
    if( pBindDetail->attrCount > 0 )
    {
        clHeapFree(pNsEntryInfo->nsInfo.attr);
    }
    clHeapFree(pNsEntryInfo);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}

                                   


ClRcT
clNameSvcCompInfoWrite(ClUint32T                  contexId,
                       ClNameSvcContextInfoT     *pCtxData,
                       ClNameSvcBindingT         *pBindData,
                       ClNameSvcBindingDetailsT  *pBindDetail)
{
    ClRcT          rc             = CL_OK;
    ClNsEntryPackT *pNsEntryInfo  = NULL;
    ClUint32T      dsIdCnt        = 0;
    ClNameT        nsCkptName     = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);

    clNameSvcPerCtxDataSetIdGet(pCtxData, &dsIdCnt);
    rc = clNameSvcPerCtxDataSetCreate(contexId, dsIdCnt);
    if( CL_OK != rc )
    {
        return rc;
    }    
    pNsEntryInfo = (ClNsEntryPackT *)clHeapCalloc(1, sizeof(ClNsEntryPackT)); 
    if( NULL == pNsEntryInfo )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc()"));
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }    
    pNsEntryInfo->type    = CL_NAME_SVC_COMP_INFO;
    clNameCopy(&pNsEntryInfo->nsInfo.name, &pBindData->name);
    pNsEntryInfo->nsInfo.objReference = pBindDetail->objReference;
    pNsEntryInfo->nsInfo.compId       = pBindDetail->compId.compId;
    pNsEntryInfo->nsInfo.eoID       = pBindDetail->compId.eoID;
    pNsEntryInfo->nsInfo.nodeAddress       = pBindDetail->compId.nodeAddress;
    pNsEntryInfo->nsInfo.clientIocPort       = pBindDetail->compId.clientIocPort;
    pNsEntryInfo->nsInfo.priority     = pBindDetail->compId.priority;
    pNsEntryInfo->nsInfo.attrCount    = pBindDetail->attrCount;
    pNsEntryInfo->nsInfo.attrLen      = pBindDetail->attrLen;
    pNsEntryInfo->nsInfo.dsId         = dsIdCnt;
    if(pBindDetail->attrCount > 0 )
    {    
        pNsEntryInfo->nsInfo.attr = clHeapCalloc(1, pBindDetail->attrLen);
        if( NULL == pNsEntryInfo->nsInfo.attr )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHeapCalloc"));
            clHeapFree(pNsEntryInfo);
            return CL_NS_RC(CL_ERR_NO_MEMORY);
        }
        memcpy(pNsEntryInfo->nsInfo.attr, pBindDetail->attr,
                pBindDetail->attrLen);
    }
    pBindDetail->compId.dsId         = dsIdCnt;
    rc = clCkptLibraryCkptDataSetWrite(gNsCkptSvcHdl, &nsCkptName,
                                       dsIdCnt, pNsEntryInfo);
    if( CL_OK != rc )
    {
        clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName, dsIdCnt); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetWrite(): rc[0x %x]", rc));
        return rc;
    }    
    if( pBindDetail->attrCount > 0 )
    {
        clHeapFree(pNsEntryInfo->nsInfo.attr);
    }
    clHeapFree(pNsEntryInfo);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcPerCtxCkptCreate(ClUint32T  key,
                          ClBoolT    isDelete)
{
    ClRcT      rc           = CL_OK;
    ClNameT    nsCkptName   = {0}; 

    CL_NAME_DEBUG_TRACE(("Enter"));

    clNamePrintf(nsCkptName, "clNSCkpt%d", key);
    if( isDelete )
    {
        rc = clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName,
                                            CL_NS_PER_CTX_DSID);
        if( CL_OK != rc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCkptLibraryCkptDataSetDelete(): rc[0x %x]", rc));
        }    
        rc = clCkptLibraryCkptDelete(gNsCkptSvcHdl, &nsCkptName);
        if( CL_OK != rc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCkptLibraryCkptDelete(): rc[0x %x]", rc));
            return rc;
        }    
    }   
    else
    {    
        rc = clCkptLibraryCkptCreate(gNsCkptSvcHdl, &nsCkptName);
        if( CL_OK != rc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCkptLibraryCkptCreate(); rc[0x %x]", rc));
            return rc;
        }    
        rc = clCkptLibraryCkptDataSetCreate(gNsCkptSvcHdl, &nsCkptName, 
                                            CL_NS_PER_CTX_DSID, 0, 0,
                                            clNameSvcPerCtxSerializer,
                                            clNameSvcPerCtxDeserializer);
        if( CL_OK != rc )
        {
            clCkptLibraryCkptDelete(gNsCkptSvcHdl, &nsCkptName);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCkptLibraryCkptDataSetCreate(): rc[0x %x]", rc));
            return rc;
        }    
    } 
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcCkptDelete(void)
{    
    ClRcT    rc        = CL_OK;
    ClNameT  ckptName  = {strlen(CL_NAME_SVC_CKPT_NAME),
                          CL_NAME_SVC_CKPT_NAME};

    CL_NAME_DEBUG_TRACE(("Enter"));
    
    rc = clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &ckptName,
                                        CL_NAME_CONTEXT_GBL_DSID);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetDelete(): rc[0x %x]", rc));
    }
    rc = clCkptLibraryCkptDelete(gNsCkptSvcHdl, &ckptName);
    if( CL_OK != rc  )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDelete(): rc[0x %x]", rc));
    }    

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    


ClRcT
clNameSvcCkptFinalize(void)
{
    ClRcT    rc        = CL_OK;

    CL_NAME_DEBUG_TRACE(("Enter"));

    if( CL_OK != (rc =  clNameSvcCkptDelete()) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clNameSvcCkptDelete() failed"));
    }    
    if( CL_OK != (rc = clCkptLibraryFinalize(gNsCkptSvcHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryFinalize(): rc[0x %x]", rc));
    }    
    gNsCkptSvcHdl = CL_HANDLE_INVALID_VALUE;

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcDataSetDelete(ClUint32T  contexId,
                       ClUint32T  dsId)
{
    ClRcT    rc           = CL_OK;
    ClNameT  nsCkptName   = {0};

    CL_NAME_DEBUG_TRACE(("Enter"));
    clNamePrintf(nsCkptName, "clNSCkpt%d", contexId);
    rc = clCkptLibraryCkptDataSetDelete(gNsCkptSvcHdl, &nsCkptName,
                                        dsId);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCkptLibraryCkptDataSetDelete(): rc[0x %x]", rc));
        return rc;
    }    
    clLogInfo("DATA", "DELETE", "Dataset [%d] deleted", dsId);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    
