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
 * ModuleName  : cor
 * File        : clCorObjTreeApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains object related APIs
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clBitApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>

/* internal includes */
#include <clCorLog.h>
#include <clCorClient.h>
#include <clCorRMDWrap.h>
#include <clCorTxnClientIpi.h>
#include <clCorPvt.h>

#include <clIdlApi.h>
#include <xdrCorObjFlagNWalkInfoT.h>
#include <xdrCorObjectInfo_t.h>
#include <xdrClCorAttrPathT.h>
#include <xdrClCorAttrCmpFlagT.h>
#include <xdrClCorAttrWalkOpT.h>
#include <xdrClCorObjectAttrInfoT.h>
#include <xdrClCorObjFlagsT.h>
#include <xdrCorObjHdlConvert_t.h>
#include <xdrClCorObjectHandleIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

#define CL_COR_OH_DEPTH_BITS 5
#define CL_COR_OH_SVCID_BITS 3
#define CL_COR_OH_DEPTH_MASK ( (1 << CL_COR_OH_DEPTH_BITS) - 1)
#define CL_COR_OH_SVCID_MASK ( (1 << CL_COR_OH_SVCID_BITS) - 1)

#define CL_COR_OH_DEPTH_SVCID_OFFSET 2

#define CL_COR_OH_MAKE_DEPTH_SVCID(depth, svcId)  \
    ( (((depth) & CL_COR_OH_DEPTH_MASK) << CL_COR_OH_SVCID_BITS) | ( (svcId) & CL_COR_OH_SVCID_MASK) )
#define CL_COR_OH_GET_DEPTH(v)  ( ((v) >> CL_COR_OH_SVCID_BITS) & CL_COR_OH_DEPTH_MASK)
#define CL_COR_OH_GET_SVCID(v)  ( (((v) & CL_COR_OH_SVCID_MASK) == CL_COR_OH_SVCID_MASK) ? -1 : (v) & CL_COR_OH_SVCID_MASK)

#define CL_COR_OH_MAKE_NIBBLE_TYPE_INSTANCE(type, instance) ( (((type)&0xf)<<4) | ((instance)&0xf) )
#define CL_COR_OH_NIBBLE_TYPE(v)     ( ((v)>>4) & 0xf )
#define CL_COR_OH_NIBBLE_INSTANCE(v) ( (v) & 0xf )

#define CL_COR_OH_ALIGN_VAL(v, a) ( ((v) + (a) - 1) & ~((a)-1) )

extern ClOsalMutexIdT gCorBundleMutex;

static __inline__ ClUint32T _corOHGetNibbleIndex(ClUint32T val);

/*
 * Nibble map covers a 64 bit range.
 */
static ClUint32T gClCorOHNibbleMap[16] = { 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64 };

static __inline__ ClUint32T _corOHGetNibbleIndex(ClUint32T val)
{
    ClUint32T shift = 0;
    while(val)
    {
        val >>= 1;
        ++shift;
    }
    shift = CL_COR_OH_ALIGN_VAL(shift, 4) >> 2;
    return shift ? shift - 1: 0;
}

static void _corOHPackData(ClUint8T* dataBuffer, ClUint32T value, ClUint32T* pFieldLen)
{
    ClUint32T fieldLen = *pFieldLen;
    switch(fieldLen)
    {
    case 1:
        {
            ClUint8T c = (ClUint8T) value;
            *dataBuffer = c;
        }
        break;
    case 2:
        {
            ClUint16T s = (ClUint16T) value;
            s = htons(s);
            memcpy(dataBuffer, &s, sizeof(s));
        }
        break;

    case 3:
        *pFieldLen = 4;
        /*
         * fall through.
         */
    case 4:
        {
            ClUint32T w = (ClUint32T) value;
            w = htonl(w);
            memcpy(dataBuffer, &w, sizeof(w));
        }
        break;
    }
}

static void _corOHUnpackData(ClInt32T *pValue, ClUint8T *dataBuffer, ClUint32T *pFieldLen)
{
    ClUint32T fieldLen = *pFieldLen;

    switch(fieldLen)
    {
    case 1:
        {
            ClUint8T c = *dataBuffer;
            *pValue = (ClInt32T)c;
            break;
        }
    case 2:
        {
            ClUint16T s = 0;
            memcpy(&s, dataBuffer, sizeof(s));
            s = ntohs(s);
            *pValue = (ClInt32T)s;
            break;
        }
    case 3:
        *pFieldLen = 4;
        /*
         * fall through.
         */
    case 4:
        {
            ClUint32T w = 0;
            memcpy(&w, dataBuffer, sizeof(w));
            w = ntohl(w);
            *pValue = (ClInt32T) w;
        }
        break;
    }
}

/**
 *  Create a cor object
 *
 *  API to create an object
 *
 *  @param   tid    COR Txn Session
 *  @param   pMoId      moid of the object  
 *  @param   pHandle    Object handle that is returned to the user  
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorObjectCreate(ClCorTxnSessionIdT *tid, ClCorMOIdPtrT pMoId, ClCorObjectHandleT *pHandle) 
{
    ClRcT  rc = CL_OK;
    ClCorTxnObjectHandleT  txnObject = {0};
    ClCorTxnSessionT  corTxnSession = {0};
    ClCorTxnJobHeaderNodeT  *pJobHdrNode = NULL;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectCreate \n"));

    /* Validate input parameters */
    if (pMoId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectCreate: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    txnObject.type = CL_COR_TXN_OBJECT_MOID;
    txnObject.obj.moId = *pMoId;

    corTxnSession.txnSessionId  = tid ? (*tid) : 0;   /* COMPLEX : SIMPLE */
    corTxnSession.txnMode  = tid ? CL_COR_TXN_MODE_COMPLEX : CL_COR_TXN_MODE_SIMPLE;

    /*
     *   Get the Txn Job Header. 
     */
    if((rc =  clCorTxnJobHeaderNodeGet(&corTxnSession, txnObject, &pJobHdrNode)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectCreate: Failed to get Job-Header"));
        CL_FUNC_EXIT();
        return(rc);
    }
 
    /* Write the reference of Object Handle */
    pJobHdrNode->pReturnHdl = pHandle;

    /* Add a 'CREATE' job to clCorTxnJobHeader */ 
    if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_CREATE, NULL)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectCreate: Could insert CREATE job to header."));
        if(corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
            clCorTxnSessionDelete(corTxnSession);
        CL_FUNC_EXIT();
        return(rc);
    }

    /* If its a SIMPLE Txn, commit here itself. else update txn Identifier. */
    if (NULL == tid)
    {
        rc = _clCorTxnSessionCommit(corTxnSession);
    }
    else
    {
        *tid = corTxnSession.txnSessionId;
    }
 
    CL_FUNC_EXIT();
    return rc;
}

/**
 *  Delete a cor cobject
 *
 *  API to delete an object
 *
 *  @param   tid    COR Txn Session
 *  @param   pHandle    Object handle of the object being deleted 
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorObjectDelete(ClCorTxnSessionIdT *tid, ClCorObjectHandleT handle) 
{
    ClRcT  rc = CL_OK;
    ClCorTxnObjectHandleT  txnObject   = {0}; 
    ClCorTxnSessionT  corTxnSession =
               { .txnSessionId  = tid? (*tid):0,   /* COMPLEX : SIMPLE */
                 .txnMode  = tid? CL_COR_TXN_MODE_COMPLEX:CL_COR_TXN_MODE_SIMPLE
                };
    ClCorTxnJobHeaderNodeT  *pJobHdrNode = NULL;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectDelete \n"));

    rc = clCorObjectHandleToMoIdGet(handle, &(txnObject.obj.moId), NULL);
    if (rc != CL_OK)
    {
        clLogError("COR", "DELETE", "Failed to get the MoId from object handle. rc [0x%x]", rc);
        return rc;
    }

    txnObject.type = CL_COR_TXN_OBJECT_MOID;

     /*
      *   Get the Txn Job Header. 
      */
    if((rc =  clCorTxnJobHeaderNodeGet(&corTxnSession, txnObject, &pJobHdrNode)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectDelete: Failed to get Job-Header"));
        CL_FUNC_EXIT();
        return(rc);
    }
    
    /* Store the object handle which will be freed after the 
     * session is committed.
     * Store the object handle instead of pointer to the object handle. 
     **/
    pJobHdrNode->pReturnHdl = handle; 

    /* Add a 'DELETE' job to clCorTxnJobHeader */ 
    if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_DELETE, NULL)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectCreate: Could not insert DELETE job to header."));
        if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
            clCorTxnSessionDelete(corTxnSession);
        CL_FUNC_EXIT();
        return(rc);
    }

    /* If its a SIMPLE Txn, commit here itself. else update txn Identifier. */
    if (NULL == tid)
    {
        rc = _clCorTxnSessionCommit(corTxnSession);
    }
    else
    {
        *tid = corTxnSession.txnSessionId;
    }

    CL_FUNC_EXIT();
    return rc;
}

/*
 * Api to create the object and set the values 
 */

ClRcT clCorObjectCreateAndSet(ClCorTxnSessionIdT* tid, ClCorMOIdPtrT pMoId, ClCorAttributeValueListPtrT attrList, ClCorObjectHandleT* pHandle)
{
    ClRcT rc = CL_OK;
    ClCorTxnJobHeaderNodeT* pJobHdrNode = NULL;
    ClUint32T idx = 0;
    ClCorAttributeValueT* pAttributeValue = NULL;
    ClCorUserSetInfoT usrSetInfo = {0};
    ClCorAttrPathT attrPath = {{{0}}};
    ClCorTxnObjectHandleT txnObject;
    ClCorTxnSessionT corTxnSession;
    
    CL_FUNC_ENTER();

    /* Validate input parameters */
    if (pMoId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectCreateAndSet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    txnObject.type = CL_COR_TXN_OBJECT_MOID; 
    txnObject.obj.moId = *pMoId;
    
    corTxnSession.txnSessionId  = tid? (*tid):0,   /* COMPLEX : SIMPLE */
    corTxnSession.txnMode  = tid? CL_COR_TXN_MODE_COMPLEX:CL_COR_TXN_MODE_SIMPLE;
 
    if (attrList == NULL)
    {
        /* Then it is simple object create */
        rc = clCorObjectCreate(tid, pMoId, pHandle);
        if (rc != CL_OK)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the object. rc [0x%x]\n", rc));
            clCorMoIdShow(pMoId);
        }

        CL_FUNC_EXIT();
        return rc;
    }
    
    /*
     *   Get the Txn Job Header. 
     */
    if((rc =  clCorTxnJobHeaderNodeGet(&corTxnSession, txnObject, &pJobHdrNode)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectCreateAndSet: Failed to get Job-Header"));
        clCorMoIdShow(pMoId);
        CL_FUNC_EXIT();
        return(rc);
    }

    /* Write the reference of Object Handle */
    pJobHdrNode->pReturnHdl = pHandle;

    /* Add one CREATE_AND_SET job and the remaining SET jobs */
    if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_CREATE_AND_SET, NULL)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to insert the Create and Set job into header. rc [0x%x]", rc));
        clCorMoIdShow(pMoId);
        if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
            clCorTxnSessionDelete(corTxnSession);
        CL_FUNC_EXIT();
        return(rc);
    }

    /* Add the jobs to the Job Header */
    /* Create the Job table to index the jobs based on the attrPath */
    if (attrList->numOfValues > 1)
    {
        if((rc = clCorTxnObjJobTblCreate(&pJobHdrNode->objJobTblHdl)) != CL_OK)
        { 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the job table. rc [0x%x]", rc));
            clCorMoIdShow(pMoId);
            if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
                clCorTxnSessionDelete(corTxnSession);
            CL_FUNC_EXIT();
            return(rc);
        }
    }

    /* Insert the SET jobs */
    pAttributeValue = attrList->pAttributeValue;

    for (idx = 0; idx < attrList->numOfValues; idx++)
    {
        if (pAttributeValue != NULL)
        {
            if (pAttributeValue->pAttrPath != NULL)
            {
                usrSetInfo.pAttrPath = pAttributeValue->pAttrPath;
            }
            else
            {
                clCorAttrPathInitialize(&attrPath);
                usrSetInfo.pAttrPath = &attrPath;
            }

            if (pAttributeValue->bufferPtr == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Buffer passed for the attrId [0x%x] is NULL", pAttributeValue->attrId));
                clCorMoIdShow(pMoId);
                if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
                    clCorTxnSessionDelete(corTxnSession);                    
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            /* Check the jobStatus and bufferPtr for NULL, before accessing */
            usrSetInfo.attrId = pAttributeValue->attrId;
            usrSetInfo.index = pAttributeValue->index;
            usrSetInfo.size = pAttributeValue->bufferSize;
            usrSetInfo.value = pAttributeValue->bufferPtr;

            /* Add the job into the header */
            if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_SET, &usrSetInfo)) != CL_OK)
            { 
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to insert the job into the Job-Header. rc [0x%x]", rc));
                clCorMoIdShow(pMoId);
                if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
                    clCorTxnSessionDelete(corTxnSession);
                CL_FUNC_EXIT();
                return(rc);
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("pAttributeValue is NULL."));
            clCorMoIdShow(pMoId);
            if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
                clCorTxnSessionDelete(corTxnSession);
            CL_FUNC_EXIT();
            return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
        }

        /* Move to the next descriptor */
        pAttributeValue++;
    }

    /* If its a SIMPLE Txn, commit here itself. else update txn Identifier. */
    if (NULL == tid)
    {
        rc = _clCorTxnSessionCommit(corTxnSession);
    }
    else
    {
        *tid = corTxnSession.txnSessionId;
    }
   
    CL_FUNC_EXIT();
    return rc;    
}

/**
 *  Object Handle to moid get 
 *
 *  API to obtain moid given an object handle
 *
 *  @param  this     object handle    
 *  @param  moId     moid
 *  @param  srvcId   service id 
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT 
clCorObjectHandleToMoIdGet(ClCorObjectHandleT objH,  ClCorMOIdPtrT pMoId, ClCorServiceIdT *srvcId)
{
    ClUint8T* objBuffer = (ClUint8T *) objH;
    ClUint8T* dataBuffer = NULL;
    ClUint32T dataLen = 0;
    ClUint16T len = 0;
    ClUint32T i = 0;
    ClUint32T typeIndex = 0;
    ClUint32T instanceIndex = 0;
    ClUint32T fieldLen = 0;
    ClRcT rc = CL_OK;

    len = *(ClUint16T *) objBuffer;
    len = ntohs(len);

    clLogTrace("OH", "CONV", "Length of the object handle obtained for conversion : [%d]", len);

    rc = clCorMoIdInitialize(pMoId);
    if (rc != CL_OK)
    {
        clLogError("OH", "CONV", "Failed to initialize the MoId. rc [0x%x]", rc);
        return rc;
    }

    objBuffer = objBuffer + CL_COR_OH_DEPTH_SVCID_OFFSET;

    pMoId->depth = CL_COR_OH_GET_DEPTH(*objBuffer);
    pMoId->svcId = CL_COR_OH_GET_SVCID(*objBuffer);

    /* Go to the byte where the nibble info starts */
    objBuffer++;

    clLogTrace("OH", "CONV", "Object Handle depth : [%d]; svcId : [%d]", pMoId->depth, pMoId->svcId);

    dataBuffer = objBuffer + pMoId->depth;

    for (i=0; i < pMoId->depth; ++i)
    {
        pMoId->node[i].type = 0;
        pMoId->node[i].instance = 0;

        typeIndex = CL_COR_OH_NIBBLE_TYPE(*objBuffer);
        instanceIndex = CL_COR_OH_NIBBLE_INSTANCE(*objBuffer++);

        if (! (gClCorOHNibbleMap[typeIndex] <= 32 && gClCorOHNibbleMap[instanceIndex] <= 32))
        {
            clLogError("OH", "", "Invalid class-id or instance-id specified.");
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        fieldLen = CL_COR_OH_ALIGN_VAL(gClCorOHNibbleMap[typeIndex], 8) >> 3;
        _corOHUnpackData(&(pMoId->node[i].type), dataBuffer, &fieldLen);

        dataLen += fieldLen;
        dataBuffer += fieldLen;

        fieldLen = CL_COR_OH_ALIGN_VAL(gClCorOHNibbleMap[instanceIndex], 8) >> 3;
        _corOHUnpackData(&(pMoId->node[i].instance), dataBuffer, &fieldLen);

        dataLen += fieldLen;
        dataBuffer += fieldLen;
    }

    clLogTrace("OH", "", "Length of the handle after conversion : [%d]",
                (dataLen + pMoId->depth + 1 + 2));

    if (! (len == (dataLen + pMoId->depth + 1 + 2)))
    {
        clLogError("OH", "", "Failed to convert the object handle into moId.");
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
    }

    /* set the service-id */
    if (srvcId != NULL)
    {
        *srvcId = pMoId->svcId;
    }

    return CL_OK;
}

void clCorObjectHandleFree(ClCorObjectHandleT* pObjH)
{
    if (pObjH && *pObjH)
    {
        clHeapFree(*pObjH);
        *pObjH = NULL;
    }
}

#if 0
ClRcT 
clCorObjectHandleToMoIdGet(ClCorObjectHandleT this,  ClCorMOIdPtrT moId, ClCorServiceIdT *srvcId)
{
    ClRcT  rc = CL_OK;
    corObjHdlConvert_t convertInfo;
    ClUint32T size;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectHandleToMoIdGet \n"));

    /* validate input parameters */
    if((moId == NULL) || (srvcId == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectHandleToMoIdGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    size = sizeof(corObjHdlConvert_t);
    /* convertInfo.version    = CL_COR_VERSION_NO; */
	memset(&convertInfo, 0 , sizeof(convertInfo));
	CL_COR_VERSION_SET(convertInfo.version);
    convertInfo.pObjHandle = this;
    convertInfo.operation  = COR_OBJ_OP_OBJHDL_TO_MOID_GET;
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECTHDL_CONV,
                                     VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo , 
                                     sizeof(corObjHdlConvert_t),
                                     VDECL_VER(clXdrUnmarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectHandleToMoIdGet returns error,rc=%x",rc));
    }
    else
    {
        *moId   = convertInfo.moId;
        *srvcId = convertInfo.svcId;
    }

    CL_FUNC_EXIT();
    return rc;
}
#endif

/**
 *  Object Handle to type get 
 *
 *  API to obtain moid given an object handle
 *
 *  @param  this     object handle    
 *  @param  type     type 
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR null ptr passsed
 */

ClRcT 
clCorObjectHandleToTypeGet(ClCorObjectHandleT objH, ClCorObjTypesT* pObjType)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;

    rc = clCorObjectHandleToMoIdGet(objH, &moId, NULL);
    if (rc != CL_OK)
    {
        clLogError("OBJ", "TYPE", "Failed to get the object handle from moId. rc [0x%x]", rc);
        return rc;
    }

    if (moId.svcId == CL_COR_INVALID_SVC_ID)
        *pObjType = CL_COR_OBJ_TYPE_MO;
    else
        *pObjType = CL_COR_OBJ_TYPE_MSO;

    return CL_OK;
}

ClRcT clCorObjectHandleServiceSet(ClCorObjectHandleT objH, ClCorServiceIdT svcId)
{
   ClUint8T* buffer = NULL;
   ClUint16T depth = 0;

   if (! (svcId >= CL_COR_INVALID_SVC_ID &&
               svcId < CL_COR_SVC_ID_MAX))
   {
       clLogError("OBJ", "SVCSET", "Invalid service-id specified.");
       return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
   }

   buffer = ((ClUint8T *) objH) + CL_COR_OH_DEPTH_SVCID_OFFSET;

   depth = CL_COR_OH_GET_DEPTH(*buffer);

   *buffer = CL_COR_OH_MAKE_DEPTH_SVCID(depth, svcId);

   return CL_OK;
}

#if 0
ClRcT 
clCorObjectHandleToTypeGet(ClCorObjectHandleT this, ClCorObjTypesT* type)
{
    ClRcT  rc = CL_OK;
    corObjHdlConvert_t convertInfo;
    ClUint32T size;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectHandleToTypeGet \n"));

    /* validate input parameters */
    if(type == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectHandleToTypeGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    size = sizeof(corObjHdlConvert_t);
    /* convertInfo.version    = CL_COR_VERSION_NO; */
	memset(&convertInfo, 0, sizeof(convertInfo));
	CL_COR_VERSION_SET(convertInfo.version);
    convertInfo.pObjHandle = this;
    convertInfo.operation  = COR_OBJ_OP_OBJHDL_TO_TYPE_GET;
   
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECTHDL_CONV,
                                     VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo , 
                                     sizeof(corObjHdlConvert_t),
                                     VDECL_VER(clXdrUnmarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectHandleToTypeGet returns error,rc=%x",rc));
    }
    else
    {
        *type = convertInfo.type;
    }

    CL_FUNC_EXIT();
    return rc;
}
#endif


/**
 *  Get Object Handle
 *
 *  API to obtain object handle given a moid
 *
 *  @param  this     object handle    
 *  @param  type     type 
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR null ptr passsed
 */

ClRcT 
clCorObjectHandleGet(ClCorMOIdPtrT moId, ClCorObjectHandleT *this)
{
    ClRcT  rc = CL_OK;
    corObjHdlConvert_t convertInfo;
    ClUint32T size;
    CL_FUNC_ENTER();
    clLogTrace("COR", "OHG", "Inside clCorObjectHandleToTypeGet");

    /* validate input parameters */
    if((moId == NULL) || (this == NULL))
    {
        clLogError("COR", "OHG", "The pointer to the MOID or the object handle structure is passed NULL.");
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset( &convertInfo, 0, sizeof(corObjHdlConvert_t));
    size = sizeof(corObjHdlConvert_t);
    /* convertInfo.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(convertInfo.version);
    convertInfo.moId       = *moId;
    convertInfo.operation  = COR_OBJ_OP_OBJHDL_GET;
  
    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECTHDL_CONV,
                                     VDECL_VER(clXdrMarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo, 
                                     sizeof(corObjHdlConvert_t),
                                     VDECL_VER(clXdrUnmarshallcorObjHdlConvert_t, 4, 0, 0),
                                     &convertInfo,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        clLogTrace("COR", "OHG", "Failed while getting the object handle from the MOID. rc[0x%x]",rc);
    }
	else 
    	*this = convertInfo.pObjHandle;

    CL_FUNC_EXIT();
    return rc;
}

 
/**
 * MoId to Object Handle Get.
 */
ClRcT clCorMoIdToObjectHandleGet(ClCorMOIdPtrT pMoId, ClCorObjectHandleT* pObjH)
{
    ClUint8T objectHandle[CL_COR_HANDLE_MAX_DEPTH + 3 +
                          CL_COR_HANDLE_MAX_DEPTH*(sizeof(ClCorClassTypeT) + sizeof(ClCorInstanceIdT))
                          ];
    ClUint8T* objBuffer = ((ClUint8T *) objectHandle) + CL_COR_OH_DEPTH_SVCID_OFFSET; /* 2 bytes to store the handle length */
    ClUint8T* dataBuffer = NULL;
    ClUint32T dataLen = 0;
    ClUint32T i = 0;
    ClUint32T fieldLen = 0;
    ClUint32T typeIndex = 0;
    ClUint32T instanceIndex = 0;

    *objBuffer++ = CL_COR_OH_MAKE_DEPTH_SVCID(pMoId->depth, pMoId->svcId);
    dataBuffer = objBuffer + pMoId->depth;

    for(i = 0; i < pMoId->depth; ++i)
    {
        typeIndex = _corOHGetNibbleIndex(pMoId->node[i].type);
        instanceIndex = _corOHGetNibbleIndex(pMoId->node[i].instance);

        if (! (typeIndex < 16 && instanceIndex < 16))
        {
            clLogError("OH", "", "Invalid class-id or instance-id specified.");
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        if (! (gClCorOHNibbleMap[typeIndex] <= 32 && gClCorOHNibbleMap[instanceIndex] <= 32))
        {
            clLogError("OH", "", "Invalid class-id or instance-id specified.");
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        *objBuffer++ = CL_COR_OH_MAKE_NIBBLE_TYPE_INSTANCE(typeIndex, instanceIndex);
        /*
         * Now hit the data part of the handle and update the type instance values.
         * accordingly based on the nibbleMap bit range.
         */
        fieldLen =  CL_COR_OH_ALIGN_VAL(gClCorOHNibbleMap[typeIndex], 8) >> 3;
        _corOHPackData(dataBuffer, pMoId->node[i].type, &fieldLen);
        dataLen += fieldLen;
     
        dataBuffer += fieldLen;
        fieldLen = CL_COR_OH_ALIGN_VAL(gClCorOHNibbleMap[instanceIndex], 8) >> 3;
        _corOHPackData(dataBuffer, pMoId->node[i].instance, &fieldLen);
        dataLen += fieldLen;
        dataBuffer += fieldLen;
    }

    /*
     * Update len header part
     */
    dataLen += pMoId->depth + 1 + 2;
    *(ClUint16T *) objectHandle = htons(dataLen);

    /*
     * Allocate the object handle.
     */
    *pObjH = (void *) clHeapAllocate(dataLen);
    if (*pObjH == NULL)
    {
        clLogError("OH", "OHG", "Failed to allocate memory.");
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    memcpy((void *) *pObjH, (void *) objectHandle, dataLen);

    return CL_OK;
}

/**
 *  Object walk 
 *
 *  API for cor object walk
 *
 *  @param  moIdRoot    moid from which the walk should start
 *  @param  moIdFilter  moid with Wildcard to filter unwanted node walk 
 *  @param  fp          function to be called
 *  @param  flags       CL_COR_MOTREE_WALK/CL_COR_MO_WALK/CL_COR_MSO_WALK/CL_COR_MO_SUBTREE_WALK/
 *                      CL_COR_MO_WALK_UP/CL_COR_MSO_WALK_UP
 *  @param cookie 	pointer to any user specific data
 *
 *  @returns
 *    CL_OK on success <br/>
 */

ClRcT 
clCorObjectWalk(ClCorMOIdPtrT moIdRoot, ClCorMOIdPtrT moIdFilter, ClCorObjectWalkFunT fp, ClCorObjWalkFlagsT flags, void *cookie)
{
    ClRcT                  rc = CL_OK;
    corObjFlagNWalkInfoT   walkInfo;
    ClUint32T              buffSize = 0;
    ClUint32T              tmpDepth = 0;
    ClBufferHandleT        inMsgHdl    =  0;
    ClBufferHandleT        outMsgHdl   =  0;

    clLogTrace("COR", "OBW", "Inside clCorObjectWalk function");

    /* validate input parameters */
    if(fp == NULL)
    {
        clLogError("COR", "OBW", "NULL argument passed for the callback function pointer. ");
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (flags == CL_COR_MO_WALK_UP)
    {
        clLogError("COR", "OBW", "The object walk is not supported with CL_COR_MO_WALKUP option");
        return  CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);  
    }

    if (flags ==  CL_COR_MSO_WALK_UP)
    {
        clLogError("COR", "OBW", "The object walk is not supported with CL_COR_MSO_WALKUP option");
        return  CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);  
    }

    memset(&walkInfo, 0, sizeof(corObjFlagNWalkInfoT));
    clCorMoIdInitialize(&walkInfo.moId);
    clCorMoIdInitialize(&walkInfo.moIdWithWC);
    if(moIdRoot)
        walkInfo.moId = *moIdRoot;
    if(moIdFilter)
	walkInfo.moIdWithWC = *moIdFilter;

    tmpDepth = walkInfo.moIdWithWC.depth;

    walkInfo.moIdWithWC.depth = walkInfo.moId.depth;
    
    if( (moIdFilter && moIdRoot) && (clCorMoIdCompare(&walkInfo.moId, &walkInfo.moIdWithWC) != 0))
    {
        clLogError("COR", "OBW", "Mismatch in Root MOId and Filter MOId ");
	    return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
		
    walkInfo.moIdWithWC.depth = tmpDepth;

    /* walkInfo.version = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(walkInfo.version);
    walkInfo.flags = flags;
    walkInfo.operation = COR_OBJ_WALK_DATA_GET;

    if((rc = clBufferCreate(&inMsgHdl))!= CL_OK)
    {
       clLogError("COR", "OBW", "Failed to create input buffer message. rc [0x%x]", rc);
       return (rc);
    }
 
    if((rc = VDECL_VER(clXdrMarshallcorObjFlagNWalkInfoT, 4, 0, 0)(&walkInfo, inMsgHdl, 0)) != CL_OK)
    {
       clLogError("COR", "OBW", "Failed to marshal object walk information. rc [0x%x]", rc);
       clBufferDelete(&inMsgHdl);
       return (rc);
    }
 
    if((rc = clBufferCreate(&outMsgHdl))!= CL_OK)
    {
       clLogError("COR", "OBW", "Failed to create output buffer message. rc [0x%x]", rc);
       clBufferDelete(&inMsgHdl);
       return (rc);
    }

    COR_CALL_RMD_SYNC_WITH_MSG(COR_EO_OBJECT_WALK, inMsgHdl, outMsgHdl, rc);

    if(rc != CL_OK)
    {
       clLogError("COR", "OBW", "Failed to walk object tree.  rc [0x%x]", rc);
    }
     
    if(rc == CL_OK)
    {
        VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHdlIDL = {0};
        ClUint32T offset = 0;

        rc = clBufferLengthGet(outMsgHdl, &buffSize);
        if (rc != CL_OK)
        {
            clLogError("COR", "OBW", "Failed to get the length of the buffer. rc [0x%x]", rc);
        }

        while (offset < buffSize)
        {
            rc = VDECL_VER(clXdrUnmarshallClCorObjectHandleIDLT, 4, 1, 0) (outMsgHdl, (void *) &objHdlIDL);
            if (rc != CL_OK)
            {
                clLogError("COR", "OBW", "Failed to unmarshall ClCorObjectHandleIDLT. rc [0x%x]", rc);
                break;
            }

            rc = fp((void *) &(objHdlIDL.oh), cookie);
            if (rc != CL_OK)
                break;

            rc = clBufferReadOffsetGet(outMsgHdl, &offset);
            if (rc != CL_OK)
            {
                clLogError("COR", "OBW", "Failed to get the read offset. rc [0x%x]", rc);
                break;
            }
        }
     }

#if 0        
    rc = clBufferLengthGet(outMsgHdl, &size);
    if ((rc == CL_OK) && (size > 0))
    {
        pOutBuff = clHeapAllocate(size); 
        if(!pOutBuff)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
            rc = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        else
        {
            if((rc = clBufferNBytesRead(outMsgHdl, (ClUint8T *)pOutBuff, &size)) == CL_OK)
            {
                for(iCount=0; iCount<(size/sizeof(ClCorObjectHandleT)); iCount++)
                {
                    memcpy(&objHdl,
                            pOutBuff+(iCount*sizeof(ClCorObjectHandleT)),
                            sizeof(ClCorObjectHandleT));

                    /* call the user specified function */
                    rc = fp((void *)&objHdl, cookie);
                    if(rc != CL_OK)
                        break;
                }
            }
            clHeapFree(pOutBuff);
        }
    }
    else
    {
        clLogNotice("COR", "OBW", "Buffer read from the server is not proper. sz [%d]. rc [0x%x]", size, rc);
    }
#endif           

     clBufferDelete(&inMsgHdl);
     clBufferDelete(&outMsgHdl);
    
    return (rc);
}



/**
 *  Set flags for MO/MSO.
 *
 *  API to set flags associated with an MO Id. 
 *
 *  @param  moh      ClCorMOId handle
 *  @param  flags    Flags associated with the MO Id
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR null ptr passsed
 */

ClRcT 
clCorObjectFlagsSet(ClCorMOIdPtrT moh, ClCorObjFlagsT flags)
{
    ClRcT  rc = CL_OK;
    corObjFlagNWalkInfoT flagInfo;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectFlagsSet \n"));

    /* validate input parameters */
    if(moh == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectFlagsSet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&flagInfo, 0, sizeof(corObjFlagNWalkInfoT));
    /* flagInfo.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(flagInfo.version);
    flagInfo.flags     = flags;
    flagInfo.moId      = *moh;
    flagInfo.operation = COR_OBJ_FLAG_SET;

    COR_CALL_RMD(COR_EO_OBJECT_FLAG_OP,
                 VDECL_VER(clXdrMarshallcorObjFlagNWalkInfoT, 4, 0, 0),
                 &flagInfo,
                 sizeof(corObjFlagNWalkInfoT), 
                 VDECL_VER(clXdrUnmarshallcorObjFlagNWalkInfoT, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorObjectFlagsSet returns error,rc=%x",rc));
    }

    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Get flags for MO/MSO.
 *
 *  API to get flags associated with an MO Id. 
 *
 *  @param  moh      ClCorMOId handle
 *  @param  pFlags   Flags associated with the MO Id
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR null ptr passsed
 */

ClRcT 
clCorObjectFlagsGet(ClCorMOIdPtrT moh, ClCorObjFlagsT* pFlags)
{
    ClRcT  rc = CL_OK;
    corObjFlagNWalkInfoT flagInfo;
    int size = 0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectFlagsGet \n"));

    /* validate input parameters */
    if((moh == NULL) || (pFlags == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectFlagsGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&flagInfo, 0, sizeof(corObjFlagNWalkInfoT));
    size = sizeof(ClCorObjFlagsT);
    /* flagInfo.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(flagInfo.version);
    flagInfo.moId      = *moh;
    flagInfo.operation = COR_OBJ_FLAG_GET;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECT_FLAG_OP,
                                     VDECL_VER(clXdrMarshallcorObjFlagNWalkInfoT, 4, 0, 0),
                                     &flagInfo ,
                                     sizeof(corObjFlagNWalkInfoT),
                                     VDECL_VER(clXdrUnmarshallClCorObjFlagsT, 4, 0, 0),
                                     pFlags,
                                     &size,
                                     rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorObjectFlagsSet returns error,rc=%x",rc));
    }


    CL_FUNC_EXIT();
    return rc;
}



ClRcT 
                   // coverity[pass_by_value]
clCorSubTreeDelete(ClCorMOIdT cAddr)
{
    ClRcT  rc = CL_OK;
    corObjFlagNWalkInfoT walkInfo;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectWalk \n"));

    memset(&walkInfo, 0, sizeof(corObjFlagNWalkInfoT));
    clCorMoIdInitialize(&walkInfo.moId);
    clCorMoIdInitialize(&walkInfo.moIdWithWC);
    walkInfo.moId = cAddr;
    /* walkInfo.version = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(walkInfo.version);
    walkInfo.operation = COR_OBJ_SUBTREE_DELETE;

    /* Get the size first */
    COR_CALL_RMD(COR_EO_OBJECT_WALK,
                 VDECL_VER(clXdrMarshallcorObjFlagNWalkInfoT, 4, 0, 0),
                 &walkInfo,
                 sizeof(corObjFlagNWalkInfoT), 
                 VDECL_VER(clXdrUnmarshallcorObjFlagNWalkInfoT, 4, 0, 0),
                 NULL,
                 NULL,
                 rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorObjectWalk returns error,rc=%x",rc));
    }
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 * Get the first child.
 *
 * NOTE: The last node class tag has to be completed and instanceId is
 * to be left at zero, and that will be updated by this function.  Get
 * the First child Id updated.
 *
 *  @param this: [IN/OUT] updated ClCorMOId is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */

ClRcT 
clCorMoIdFirstInstanceGet(ClCorMOIdPtrT pMoId) 
{
    ClRcT  rc = CL_OK;
    corObjectInfo_t objectInfo;
    ClUint32T size = sizeof(corObjectInfo_t);

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorMoIdFirstInstanceGet \n"));

    /* Validate input parameters */
    if (pMoId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMoIdFirstInstanceGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&objectInfo, 0, sizeof(corObjectInfo_t));

    /* objectInfo.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(objectInfo.version);
    objectInfo.moId      = *(pMoId);
    objectInfo.operation = COR_OBJ_OP_FIRSTINST_GET;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECT_OP,
                                     VDECL_VER(clXdrMarshallcorObjectInfo_t, 4, 0, 0),
                                     &objectInfo , 
                                     sizeof(corObjectInfo_t),
                                     VDECL_VER(clXdrUnmarshallcorObjectInfo_t, 4, 0, 0),
                                     &objectInfo,
                                     &size,
                                     rc);
    
    if(CL_OK == rc)
        *pMoId = objectInfo.moId;
 
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 * Get the Next Sibling.
 *
 * Get the Next Sibling in the MO Id tree.
 *
 *  @param this: [IN/OUT] updated ClCorMOId is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */
ClRcT
clCorMoIdNextSiblingGet(ClCorMOIdPtrT pMoId)
{
    ClRcT  rc = CL_OK;
    corObjectInfo_t objectInfo;
    ClUint32T size = sizeof(corObjectInfo_t);

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorMoIdNextSiblingGet \n"));

    /* Validate input parameters */
    if (pMoId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorMoIdNextSiblingGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&objectInfo, 0, sizeof(corObjectInfo_t));

    /* objectInfo.version   = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(objectInfo.version);
    objectInfo.moId      = *(pMoId);
    objectInfo.operation = COR_OBJ_OP_NEXTSIBLING_GET;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_OBJECT_OP,
                                     VDECL_VER(clXdrMarshallcorObjectInfo_t, 4, 0, 0),
                                     &objectInfo, 
                                     sizeof(corObjectInfo_t),
                                     VDECL_VER(clXdrUnmarshallcorObjectInfo_t, 4, 0, 0),
                                     &objectInfo,
                                     &size,
                                     rc);
 
    if(CL_OK == rc)
        *pMoId = objectInfo.moId;
    
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Get Attribute
 *
 *  Generic API to get all kind of attributes. 
 *
 *  @param   tid         transaction id
 *  @param   pHandle     Object handle of the object whose attribute is being read
 *  @param   attrPath    containment hierarchy.
 *  @param   attrId      attribute id
 *  @param   idx         attribute index (CL_COR_INVALID_ATTR_IDX for simple attribute.)
 *  @param   value       the value to be set
 *  @param   size        the size of the value
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_ERR_NULL_PTR        NULL pointer
 *
 */

ClRcT clCorObjectAttributeGet(ClCorObjectHandleT this, ClCorAttrPathPtrT contAttrPath, ClCorAttrIdT attrId, ClInt32T index, void *value, ClUint32T *size)
{
    ClRcT                           rc          = CL_OK;
    ClCorBundleHandleT              bundleHandle = 0;
    ClCorBundleConfigT              bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorListT   attrList = {0};
    ClCorAttrValueDescriptorT       attrInfo = {0};
    ClCorJobStatusT                 jobStatus = 0;

    clDbgIfNullReturn(value, CL_CID_COR);
    clDbgIfNullReturn(size, CL_CID_COR);

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        clLogError("COR", "GET", "Failed to initialize the bundle. rc[0x%x]", rc);
        return rc;
    }

    attrInfo.pAttrPath = contAttrPath;
    attrInfo.attrId = attrId;
    attrInfo.index = index;
    attrInfo.bufferPtr = value;
    attrInfo.bufferSize = *size;
    attrInfo.pJobStatus = &jobStatus;

    attrList.numOfDescriptor = 1;
    attrList.pAttrDescriptor = &attrInfo;
    
    rc = clCorBundleObjectGet(bundleHandle, &this, &attrList);
    if(CL_OK != rc)
    {
        clLogError("COR", "GET", "Failed to add the job in the bundle. rc[0x%x]", rc);
        jobStatus = rc;
        goto handleError;
    }

    rc = clCorBundleApply(bundleHandle);
    if(CL_OK != rc)
    {
        clLogError("COR", "GET", "Failed while applying the bundle. rc[0x%x]", rc);
        jobStatus = rc;
        goto handleError;
    }

handleError:

    rc = clCorBundleFinalize(bundleHandle);
    if(CL_OK != rc)
    {
        clLogError("COR", "GET", "Failed while finalizing the bundle. rc[0x%x]", rc);
        return rc;
    }

    rc = jobStatus;

    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Set Attribute
 *
 *  Generic API to set all kind of attributes. 
 *
 *  @param   tid         transaction id
 *  @param   pHandle     Object handle of the object whose attribute is being set
 *  @param   attrPath    containment hierarchy.
 *  @param   attrId      attribute id
 *  @param   idx         attribute index (CL_COR_INVALID_ATTR_IDX for simple attribute.)
 *  @param   value       the value to be set
 *  @param   size        the size of the value
 *
 *  @returns
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)        NULL pointer
 *
 */

ClRcT clCorObjectAttributeSet(ClCorTxnSessionIdT *tid, ClCorObjectHandleT this, ClCorAttrPathPtrT contAttrPath, ClCorAttrIdT attrId, ClUint32T index, void *value, ClUint32T size)
{
    ClRcT  rc = CL_OK;
    ClCorUserSetInfoT usrSetInfo = {0};
    ClCorTxnObjectHandleT  txnObject = {0};
    ClCorTxnSessionT  corTxnSession =
               { .txnSessionId  = tid? (*tid):0,   /* COMPLEX : SIMPLE */
                 .txnMode  = tid? CL_COR_TXN_MODE_COMPLEX:CL_COR_TXN_MODE_SIMPLE
                };
    ClCorTxnJobHeaderNodeT  *pJobHdrNode = NULL;
    ClCorAttrPathT       attrPath;

    CL_FUNC_ENTER();

    rc = clCorObjectHandleToMoIdGet(this, &(txnObject.obj.moId), NULL);
    if (rc != CL_OK)
    {
        clLogError("COR", "SET", "Failed to convert the object handle into moId. rc [0x%x]", rc);
        return rc;
    }

    txnObject.type = CL_COR_TXN_OBJECT_MOID;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjectAttributeSet \n"));

    /* Validate input parameters */
    if ((value == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjectAttributeSet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

     /*
      *   Get the Txn Job Header. 
      */
    if((rc =  clCorTxnJobHeaderNodeGet(&corTxnSession, txnObject, &pJobHdrNode)) != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectAttributeSet: Failed to get Job-Header"));
        CL_FUNC_EXIT();
        return(rc);
    }

    /* Complex Transaction. So need to index the jobs based on attrPath */
     if(tid != NULL && pJobHdrNode->objJobTblHdl == 0) 
     {
       if((rc = clCorTxnObjJobTblCreate(&pJobHdrNode->objJobTblHdl)) != CL_OK) 
       { 
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
           ( "\nclCorObjectAttributeSet: COMPLEX TXN -  Failed to create object-job table"));
           if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
               clCorTxnSessionDelete(corTxnSession);
           CL_FUNC_EXIT();
           return(rc);
       }
     }

     if(contAttrPath)
       usrSetInfo.pAttrPath = contAttrPath;
     else
     {
       /* Set the attribute path. */
         clCorAttrPathInitialize(&attrPath);
         usrSetInfo.pAttrPath = &attrPath;
     }
       usrSetInfo.attrId    = attrId;
       usrSetInfo.index     = index;
       usrSetInfo.size      = size;
       usrSetInfo.value     = value;

     /* Insert this SET job to the transaction. */
     if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_SET, &usrSetInfo)) != CL_OK)
     { 
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nclCorObjectAttributeSet: Could insert SET job to header."));
         if (corTxnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
            clCorTxnSessionDelete(corTxnSession);
         CL_FUNC_EXIT();
         return(rc);
     }
 
    /* If its a SIMPLE Txn, commit here itself. else update txn Identifier. */
    if (NULL == tid)
    {
        rc = _clCorTxnSessionCommit(corTxnSession);
    }
    else
    {
        *tid = corTxnSession.txnSessionId;
    }

    CL_FUNC_EXIT();
    return rc;
}


/**
 *  Valildate the service ID argument.
 *
 *  This API validates the input service ID argument.
 *                                                                        
 *  @param srvcId  Service ID argument
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT 
clCorServiceIdValidate(ClCorServiceIdT srvcId)
{
    CL_FUNC_ENTER();
  
    if ((srvcId > CL_COR_SVC_ID_MAX)||
        (srvcId <= CL_COR_INVALID_SRVC_ID))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Invalid service ID specified 0x%d",
                              (ClUint32T)srvcId));
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
    }
    CL_FUNC_EXIT();
    return(CL_OK);
}

#ifdef GETNEXT

ClRcT 
clCorNextMOGet(ClCorObjectHandleT stOH, 
           ClCorClassTypeT classId,
	   ClCorServiceIdT  svcId,
           ClCorObjectHandleT *nxtOH
           )
{
    ClRcT  rc = CL_OK;
    corObjGetNextInfo_t getNextInfo;
    ClUint32T size = sizeof(ClCorObjectHandleT);

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorNextMOGet \n"));
 
    if ((svcId != CL_COR_SVC_WILD_CARD) && 
		 ((svcId > CL_COR_SVC_ID_MAX) ||
                         (svcId < CL_COR_INVALID_SRVC_ID)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid service ID specified 0x%x",
                              (ClUint32T)svcId));
        return(CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
    }

    if (classId != CL_COR_CLASS_WILD_CARD && classId <= 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid Class Id specified."));
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }

    if (nxtOH == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorNextMOGet: NULL argument"));
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    memset(&getNextInfo, 0, sizeof(corObjGetNextInfo_t));
   
    /* Set the info.*/
    getNextInfo.operation = COR_OBJ_OP_NEXTMO_GET;
    getNextInfo.objHdl = stOH;
    getNextInfo.classId = classId;
    getNextInfo.svcId = svcId;
    
    COR_CALL_RMD(COR_OBJ_GET_NEXT_OPS, clXdrMarshallcorObjGetNextInfo_t, &getNextInfo, sizeof(corObjGetNextInfo_t),
		                          clXdrUnmarshallcorObjGetNextInfo_t, &(getNextInfo.objHdl), &size, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorNextMOGet returns error,rc=%x",rc));
    }	
    else
    {
        *nxtOH = getNextInfo.objHdl;
    }
    
    CL_FUNC_EXIT();
    return rc;
}
#endif


ClRcT 
_clCorObjBufUnpackNWalk(ClBufferHandleT objMsgBufHdl,
                             ClCorObjAttrWalkFuncT fp, void *cookie)
{
    ClRcT rc = CL_OK;
    ClCorObjectAttrInfoT *pObjAttrInfo = clHeapAllocate(sizeof(ClCorObjectAttrInfoT)); /*(ClCorObjectAttrInfoT *) userArg;*/
    void *value = NULL;
    ClCorAttrPathPtrT pAttrPath;
    ClUint32T  size;
    ClCorAttrFlagT attrFlag = 0;
  
    CL_FUNC_ENTER();

    if(!objMsgBufHdl)
      {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }

   if((rc = VDECL_VER(clXdrUnmarshallClCorObjectAttrInfoT, 4, 0, 0)(objMsgBufHdl, (void *)pObjAttrInfo)) != CL_OK)
   {
     CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to unmarshall ClCorObjectAttrInfoT\n"));
     return (rc);
   }
   
   if((rc = clCorAttrPathAlloc(&pAttrPath)) != CL_OK)
   {
     return (rc);
   }

    /** while not EOF **/
    while(!(pObjAttrInfo->attrType == 0 &&
                pObjAttrInfo->attrId == 0 &&
                      pObjAttrInfo->lenOrContOp == 0) &&
                         rc == CL_OK)
    {
        if(pObjAttrInfo->attrType != CL_COR_CONTAINMENT_ATTR) 
        {  
           size = pObjAttrInfo->lenOrContOp;
		  /* TODO : Need to call the api which will convert the value to an appropriate format. */
           attrFlag = pObjAttrInfo->attrFlag;
           if(pObjAttrInfo->attrType == CL_COR_ARRAY_ATTR ||
                pObjAttrInfo->attrType == CL_COR_ASSOCIATION_ATTR)
           {
              ClUint32T type ; 
			  clXdrUnmarshallClUint32T(objMsgBufHdl, (void *)&type); 

              if(pObjAttrInfo->attrType == CL_COR_ASSOCIATION_ATTR)
             	 type = CL_COR_INVALID_DATA_TYPE;
             
              /* 4-bytes reserved for array type */ 
              size -= sizeof(ClUint32T);

	      	  value = clHeapAllocate(size);
              if(!value)
              {
                 /* TODO: write log.*/
              	 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                         CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
				CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to allocate memory"));
				return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
              }
 	      	  rc = clBufferNBytesRead(objMsgBufHdl, value, &size);
	          /* TODO : Check Error  */
			  if(rc != CL_OK)
			  {
					CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to read Bytes from the Message. rc[0x%x]", rc));
					return rc;
			  }

              if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet()) 
			  {
                 rc = clCorObjAttrValSwap(value, size, pObjAttrInfo->attrType, type);
			  	 if(rc != CL_OK)
			     {
				  	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Attribute value swap failed in callback. rc[0x%x]", rc));
					return rc;
			     }
			   }


               /* pass NULL as attrPath, if attribute is of base object.*/
               rc = fp(pAttrPath->node[0].attrId != CL_COR_INVALID_ATTR_ID ?pAttrPath:NULL, 
                       pObjAttrInfo->attrId,
                          pObjAttrInfo->attrType,
                            type, value, size, attrFlag, cookie); 
	     	  clHeapFree(value);
			  value = NULL;
           }
           else
           { 
	          value = clHeapAllocate(size);

              if(!value)
              {
                 /* TODO: write log.*/
              	 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                         CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
              }

	          rc = clBufferNBytesRead(objMsgBufHdl, value, &size);
			  if(rc != CL_OK)
			  {
					CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to read Bytes from the Message. rc[0x%x]", rc));
					return rc;
			  }
	          /* TODO : Check Error  */
              if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
			  {
                 rc = clCorObjAttrValSwap(value, size, pObjAttrInfo->attrType, CL_COR_INVALID_DATA_TYPE);
			  	 if(rc != CL_OK)
			     {
				  	CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Attribute value swap failed in callback. rc[0x%x]", rc));
					return rc;
			     }
			   }
              /* pass NULL as attrPath, if attribute is of base object.*/
              rc = fp(pAttrPath->node[0].attrId != CL_COR_INVALID_ATTR_ID?pAttrPath:NULL,
                          pObjAttrInfo->attrId,
                           CL_COR_SIMPLE_ATTR,
                             pObjAttrInfo->attrType,
                                  value, size, attrFlag, cookie); 
		     clHeapFree(value);
		     value = NULL;

           }
		   VDECL_VER(clXdrUnmarshallClCorObjectAttrInfoT, 4, 0, 0)(objMsgBufHdl, (void *)pObjAttrInfo);
        }
        else
        {
           ClUint32T index = 0;

           if(pObjAttrInfo->lenOrContOp != CL_COR_OBJSTREAM_CONT_TRUNCATE)
           {
			  clXdrUnmarshallClUint32T(objMsgBufHdl, (void *)&index);
           } 
        
           if(pObjAttrInfo->lenOrContOp == CL_COR_OBJSTREAM_CONT_APPEND)
             clCorAttrPathAppend(pAttrPath, pObjAttrInfo->attrId, index);
           if(pObjAttrInfo->lenOrContOp == CL_COR_OBJSTREAM_CONT_SET)
             clCorAttrPathSet(pAttrPath, pAttrPath->depth, pObjAttrInfo->attrId, index);
           if(pObjAttrInfo->lenOrContOp == CL_COR_OBJSTREAM_CONT_TRUNCATE)
             clCorAttrPathTruncate(pAttrPath, pAttrPath->depth-1);
 
			 VDECL_VER(clXdrUnmarshallClCorObjectAttrInfoT, 4, 0, 0)(objMsgBufHdl, (void *)pObjAttrInfo);
       }
    }
	clHeapFree(pObjAttrInfo);
    clCorAttrPathFree(pAttrPath);
    return (CL_OK);
    CL_FUNC_EXIT();
}


/**
 *  Object Attribute walk 
 *
 *  API for cor object attibute walk with Filter
 *
 *  @todo: Note down all the params.
 *  
 *  @returns
 *    CL_OK on success <br/>
 */

ClRcT 
clCorObjectAttributeWalk(ClCorObjectHandleT objH,
                           ClCorObjAttrWalkFilterT *pFilter,
                              ClCorObjAttrWalkFuncT   fp,
                                 void * cookie)
{
   ClRcT rc = CL_OK;
   ClBufferHandleT inMsgHdl;
   ClBufferHandleT outMsgHdl;
   ClCorAttrPathT     attrPath;
   ClVersionT 		  version;
   ClUint8T           srcArch; /* client arch. whether litte endian/big endian */
   VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIdl = {0};

   /* Version Set */
   CL_COR_VERSION_SET(version);
   /* Get the client endianness */
   srcArch = clBitBlByteEndianGet(); /* It will be used in the server side */
   
    if(fp == NULL )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL function pointer .. Error [0x%x] \n", rc));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
 
    if(pFilter)
    {
      if(pFilter->attrId != CL_COR_INVALID_ATTR_ID)
      {
           if(pFilter->cmpFlag <=CL_COR_ATTR_CMP_FLAG_INVALID ||
              pFilter->cmpFlag >= CL_COR_ATTR_CMP_FLAG_MAX    ||
              pFilter->attrWalkOption <=CL_COR_ATTR_INVALID_OPTION  ||
              pFilter->attrWalkOption  >= CL_COR_ATTR_WALK_MAX)
           {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid Parameter passed in filter !!! \n"));
              return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
           }
          
           if(pFilter->index < CL_COR_INVALID_ATTR_IDX)
           {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid Index passed in filter !!! \n"));
              return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
           }
           if(pFilter->value == NULL)
           {
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL pointer passed as value !!! \n"));
              return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
           }
      }

      if ((pFilter->contAttrWalk == CL_FALSE) && (pFilter->pAttrPath != NULL))
      {
            /* if containment attribute walk is specified as false, and he has specified some attr paths */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Invalid parameter passed in filter. Valid Attr Path is passed when contAttrWalk is FALSE"));
            return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
       }
 
    }

    clBufferCreate(&inMsgHdl);
    clCorAttrPathInitialize(&attrPath);

	rc = clXdrMarshallClVersionT(&version, inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATR", "Failed to Marshall the version information. rc [0x%x]", rc);
        goto error_on_exit;
    }

    rc = clCorObjectHandleSizeGet(objH, &(objHIdl.ohSize));
    if (rc != CL_OK)
    {
        clLogError("COR", "ATR", "Failed to get the object handle size. rc [0x%x]", rc);
        goto error_on_exit;
    }

    objHIdl.oh = objH;

    rc = VDECL_VER(clXdrMarshallClCorObjectHandleIDLT, 4, 1, 0)((void *) &objHIdl, inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATR", "Failed to Marshall ClCorObjectHandleIDLT_4_1_0. rc [0x%x]", rc);
        goto error_on_exit;
    }

#if 0    
	rc = VDECL_VER(clXdrMarshallClCorObjectHandleT, 4, 0, 0)(&objH, inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATR", "Failed to Marshall the object handle. rc [0x%x]", rc);
        goto error_on_exit;
    }
#endif

    /* Marshall the endianness */
    rc = clXdrMarshallClUint8T(&(srcArch), inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATR", "Failed to Marshall source architecture. rc [0x%x]", rc);
        goto error_on_exit;
    }

    if(pFilter)
    {
		rc = clXdrMarshallClUint8T(&pFilter->baseAttrWalk, inMsgHdl, 0);
        if (rc != CL_OK)
        {
            clLogError("COR", "ATR", "Failed to Marshall baseAttrWalk flag. rc [0x%x]", rc);
            goto error_on_exit;
        }

		rc = clXdrMarshallClUint8T(&pFilter->contAttrWalk, inMsgHdl, 0);
        if (rc != CL_OK)
        {
            clLogError("COR", "ATR", "Failed to Marshall containment attribute walk flag. rc [0x%x]", rc);
            goto error_on_exit;
        }

        if(pFilter->pAttrPath != NULL)
        {
		   rc = VDECL_VER(clXdrMarshallClCorAttrPathT, 4, 0, 0)(pFilter->pAttrPath, inMsgHdl, 0);
           if (rc != CL_OK)
           {
                clLogError("COR", "ATR", "Failed to Marshall user specified attribute path. rc [0x%x]", rc);
                goto error_on_exit;
           }
        }
        else
        {
		   rc = VDECL_VER(clXdrMarshallClCorAttrPathT, 4, 0, 0)(&attrPath, inMsgHdl, 0);
           if (rc != CL_OK)
           {
                clLogError("COR", "ATR", "Failed to Marshall default attribute path. rc [0x%x]", rc);
                goto error_on_exit;
           }
        }

        rc = clXdrMarshallClInt32T( &pFilter->attrId, inMsgHdl, 0);
        if (rc != CL_OK)
        {
            clLogError("COR", "ATR", "Failed to Marshall attribute id. rc [0x%x]", rc);
            goto error_on_exit;
        }

        /** Following filter paramters make sense only for valid attribute id.**/
        if(pFilter->attrId != CL_COR_INVALID_ATTR_ID)
        {
             rc = clXdrMarshallClUint32T(&pFilter->index, inMsgHdl, 0);
             if (rc != CL_OK)
             {
                clLogError("COR", "ATR", "Failed to Marshall attribute index. rc [0x%x]", rc);
                goto error_on_exit;
             }

             rc = VDECL_VER(clXdrMarshallClCorAttrCmpFlagT, 4, 0, 0)(&pFilter->cmpFlag, inMsgHdl, 0);
             if (rc != CL_OK)
             {
                clLogError("COR", "ATR", "Failed to Marshall attribute comparison flag. rc [0x%x]", rc);
                goto error_on_exit;
             }

             rc = VDECL_VER(clXdrMarshallClCorAttrWalkOpT, 4, 0, 0)(&pFilter->attrWalkOption, inMsgHdl, 0);
             if (rc != CL_OK)
             {
                clLogError("COR", "ATR", "Failed to Marshall Attribute Walk Option. rc [0x%x]", rc);
                goto error_on_exit;
             }

             rc = clXdrMarshallClUint32T(&pFilter->size, inMsgHdl, 0);
             if (rc != CL_OK)
             {
                clLogError("COR", "ATR", "Failed to Marshall attribute size. rc [0x%x]", rc);
                goto error_on_exit;
             }

             if(pFilter->size)
             {
                rc = clXdrMarshallPtrClCharT(pFilter->value, pFilter->size, inMsgHdl, 0); 
                if (rc != CL_OK)
                {
                    clLogError("COR", "ATR", "Failed to marshall attribute value. rc [0x%x]", rc);
                    goto error_on_exit;
                }
             }
        }
    }

    /* out message */
    clBufferCreate(&outMsgHdl);

    COR_CALL_RMD_SYNC_WITH_MSG(COR_OBJ_EXT_OBJ_PACK, inMsgHdl, outMsgHdl, rc);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n clCorObjectAttributeWalk failed .. Error [0x%x] \n", rc));
        clBufferDelete(&inMsgHdl);
        clBufferDelete(&outMsgHdl);
        return (rc);
    }
   
    rc = _clCorObjBufUnpackNWalk(outMsgHdl, fp, cookie);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Unpacking failed for Object on client .. Error [0x%x] \n", rc));

        clBufferDelete(&inMsgHdl);
        clBufferDelete(&outMsgHdl);

        return (rc);
    }
    
    /* Delete the buffers */
    clBufferDelete(&outMsgHdl);

error_on_exit:    
    clBufferDelete(&inMsgHdl);
   
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Function to swap the attribute value.
 *  This function does not consider any endianness. It is
 *   caller's duty to see, if he needs to swap the values.
 *  Also the caller need to make sure that size is correct.
 *
 *  @returns
 *    CL_OK on success <br/>
 */

ClRcT 
clCorObjAttrValSwap(CL_INOUT  ClUint8T        *pData,
                    CL_IN      ClUint32T       dataSize,
                    CL_IN      ClCorAttrTypeT  attrType,
                    CL_IN      ClCorTypeT      arrayDataType  /*data qualifier,
                                                                if attrType is of type CL_COR_ARRAY_ATTR */
                   )
{
    ClRcT       rc = CL_OK;
    ClUint8T   *pLoc;
    ClUint32T   currSize = 0;
    ClUint8T    done = CL_FALSE;
    ClUint32T   type;
    ClUint32T   dataType; 
    ClUint8T    arr = 0;


    if(pData == NULL )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument .. Error [0x%x] \n", 
                    (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR))));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
 
    pLoc = (ClUint8T *)pData;
    type = attrType;

    while(done == CL_FALSE)
    {
         dataType = type;
         switch (type)
         {
             case CL_COR_ARRAY_ATTR:
             {
                arr = 1;
                dataType = arrayDataType;
                if(dataType == CL_COR_UINT8 || dataType == CL_COR_INT8)
                {
                   /* Need not do anything. */
                    return (rc);
                }
   
                if(currSize == dataSize)
                {
                   done = CL_TRUE;
                   return (CL_OK);
                }
                else if(currSize > dataSize)
                {
                    /* invalid size passed. */
                    return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
                }
             }

             case CL_COR_INT16: 
             case CL_COR_UINT16: 
             if(dataType == CL_COR_UINT16 || dataType == CL_COR_INT16)
             { 
                  /* Swap */
                  *(ClUint16T *)pLoc = CL_BIT_SWAP16(*(ClUint16T *)pLoc);
                  currSize += sizeof(ClUint16T);
                  pLoc     += sizeof(ClUint16T);  
                  if(!arr)
                     done = CL_TRUE;
               break;
             } 

             case CL_COR_INT32:
             case CL_COR_UINT32: 
             if(dataType == CL_COR_UINT32 || dataType == CL_COR_INT32)
             {
                  /* Swap */
                  *(ClUint32T *)pLoc =  CL_BIT_SWAP32(*(ClUint32T *)pLoc);
                  currSize += sizeof(ClUint32T);
                  pLoc     += sizeof(ClUint32T);  
                  if(!arr)
                     done = CL_TRUE;
               break;
             }

             case CL_COR_INT64:
             case CL_COR_UINT64:
             if(dataType == CL_COR_UINT64 || dataType == CL_COR_INT64)
             {
                  /* Swap */
                  *(ClUint64T *)pLoc = CL_BIT_SWAP64(*(ClUint64T *)pLoc);
                  currSize += sizeof(ClUint64T);
                  pLoc     += sizeof(ClUint64T);  
                  if(!arr)
                     done = CL_TRUE;
                break;
             }

          /* complex data types */
             case CL_COR_ASSOCIATION_ATTR:
             {
                /* DO nothing */
                 return (rc);
             }
             break;
            default:
             {
    	        CL_FUNC_EXIT(); 
 	        return (rc);
             }
             break;    
         }
   }
  return (rc);
}





/**
 * Function to remove all the jobs that got added when there is a failure
 * occured during adding the jobs.
 */

ClRcT _clCorBundleJobRemove(CL_IN    ClCorBundleInfoPtrT       pBundleInfo,
                            // coverity[pass_by_value]
                            CL_IN    ClCorTxnObjectHandleT       bundleContKey,
                            CL_INOUT ClCorTxnJobHeaderNodeT *pJobHdrNode, 
                            CL_IN    ClCorAttrValueDescriptorListPtrT pAttrDesc,
                            CL_IN    ClUint32T                       till)
{
    ClRcT                           rc = CL_OK;
    ClUint32T                       index = 0;
    ClCorTxnObjJobNodeT             *pObjJobNode = NULL, *pTempObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT         *pAttrJobNode = NULL, *pTempAttrJobNode = NULL;
    ClCorAttrValueDescriptorPtrT    pTempAttrDesc = NULL;
    ClCorAttrPathT                  attrPath;
    

    for(index = 0; index < till ; index++)
    {
        pTempAttrDesc = (pAttrDesc->pAttrDescriptor + index);

        if(NULL == pTempAttrDesc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer found. "));
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

        if(pTempAttrDesc->pAttrPath == NULL)
            clCorAttrPathInitialize(&attrPath);
        else
             attrPath = *pTempAttrDesc->pAttrPath;

        rc = clCorTxnObjJobNodeGet(pJobHdrNode,
                                    &attrPath, &pObjJobNode);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot find the job Node. rc[0x%x]", rc));
            return rc;
        }
         
        if(pObjJobNode)
        {
            pAttrJobNode = pObjJobNode->head;

            while(pAttrJobNode)
            {
                if(pAttrJobNode->job.attrId == 
                        pTempAttrDesc->attrId)
                {
                    pTempAttrJobNode = pAttrJobNode;

                    /* Macro to adjust the pointers*/
                    CL_COR_BUNDLE_JOB_ADJUST(pObjJobNode, pAttrJobNode);

                    pAttrJobNode = pTempAttrJobNode->next;
                    
                    pObjJobNode->job.numJobs--;

                    clHeapFree(pTempAttrJobNode);
                    break;
                }

                pAttrJobNode = pAttrJobNode->next;
            }

            if(0 == pObjJobNode->job.numJobs)
            {
                pTempObjJobNode = pObjJobNode;

                /* Macro to adjust the pointers*/
                CL_COR_BUNDLE_JOB_ADJUST(pJobHdrNode, pObjJobNode);
                
                clCntAllNodesForKeyDelete(pJobHdrNode->objJobTblHdl, 
                        (ClCntKeyHandleT)&pTempObjJobNode->job.attrPath);

                clHeapFree(pTempObjJobNode);
                
                pJobHdrNode->jobHdr.numJobs--;

            }
        }
    }

    /* Look for all the jobs there were added. */
    

    if(0 == pJobHdrNode->jobHdr.numJobs)
    {
        rc = clCntAllNodesForKeyDelete(pBundleInfo->jobStoreCont, 
                (ClCntKeyHandleT)&bundleContKey);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the bundle from the job container. rc[0x%x]", rc));
            return rc;
        }
    }
    
    return rc;
}




/**
 * Function to add the get jobs in the bundle.
 */

ClRcT _clCorBundleGetJob(CL_IN      ClCorBundleHandleT bundleHandle,
                         CL_IN      const ClCorObjectHandleT *pObjectHandle,
                         CL_INOUT   ClCorAttrValueDescriptorListPtrT pAttrDescList)
{
    ClRcT                       rc = CL_OK;
    ClCorTxnObjectHandleT       bundleContKey  = {0};  
    ClCorTxnJobHeaderNodeT      *pJobHdrNode = NULL;
    ClCorUserSetInfoT           usrGetInfo = {0};
    ClCorAttrValueDescriptorPtrT pTempAttrDesc = NULL;
    ClCorBundleInfoPtrT         pBundleInfo = NULL;
    ClUint32T                   index = 0;
    ClCorAttrPathT              attrPath;
    

    if((NULL == pObjectHandle) || (NULL == pAttrDescList))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL parameter passed."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(0 == pAttrDescList->numOfDescriptor)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(" Invalid parameter passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clCorObjectHandleToMoIdGet((*pObjectHandle), &(bundleContKey.obj.moId), NULL);
    if (rc != CL_OK)
    {
        clLogError("COR", "BUNDLEGET", "Failed to get the MoId from Object handle. rc [0x%x]", rc);
        return rc;
    }

    bundleContKey.type = CL_COR_TXN_OBJECT_MOID;
    
    memset(&usrGetInfo, 0, sizeof(usrGetInfo));

    clOsalMutexLock(gCorBundleMutex);

    rc = clCorBundleHandleGet(bundleHandle, &pBundleInfo);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gCorBundleMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the bundle handle. rc[0x%x]", rc));
        rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);

    /* Check the status of the bundle. */
    if(CL_COR_BUNDLE_IN_EXECUTION == pBundleInfo->bundleStatus)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot add job for this bundle as bundle is in progress. "));
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_IN_EXECUTION);
    }

    /*Check for the bundle type */
    if(CL_COR_BUNDLE_TRANSACTIONAL == pBundleInfo->bundleType)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot add get job in the transactional bundle."));
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_INVALID_TYPE);
    }


    rc = clCorBundleJobHeaderNodeGet(pBundleInfo, bundleContKey, &pJobHdrNode);
    if( CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(" Failed while getting the bundle header"));
        return rc;
    }     

    /* Bundle operation. So need to index the jobs based on attrPath */
    if( 0 == pJobHdrNode->objJobTblHdl ) 
    {
        if((rc = clCorTxnObjJobTblCreate(&pJobHdrNode->objJobTblHdl)) != CL_OK) 
        { 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to create the job table"));
            CL_FUNC_EXIT();
            return(rc);
        }
    }

    /* Add all the jobs in the bundle */
    for (index = 0; index < pAttrDescList->numOfDescriptor; index++)
    {
        pTempAttrDesc = (pAttrDescList->pAttrDescriptor+index);

        if(NULL == pTempAttrDesc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The Bundle job contain less data then value of numOfDescriptor."));
            if(_clCorBundleJobRemove(pBundleInfo, bundleContKey, 
                    pJobHdrNode, pAttrDescList, index) != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while removing the jobs."));
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
        
        if(NULL != pTempAttrDesc->pAttrPath)
                usrGetInfo.pAttrPath = pTempAttrDesc->pAttrPath;
        else
        {
            /* Set the attribute path. */
            clCorAttrPathInitialize(&attrPath);
            usrGetInfo.pAttrPath = &attrPath;
        }

        usrGetInfo.attrId    = pTempAttrDesc->attrId;
        usrGetInfo.index     = pTempAttrDesc->index;
        usrGetInfo.size      = pTempAttrDesc->bufferSize;
        usrGetInfo.value     = pTempAttrDesc->bufferPtr;
        usrGetInfo.pJobStatus = pTempAttrDesc->pJobStatus;

        if((rc  = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_GET, &usrGetInfo)) != CL_OK)
        { 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Could insert the bundle of get job. rc[0x%x]", rc));

            if(_clCorBundleJobRemove(pBundleInfo, bundleContKey, 
                    pJobHdrNode, pAttrDescList, index) != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while removing the jobs."))

            CL_FUNC_EXIT();
            return(rc);
        }
    }
    
    CL_FUNC_EXIT();
    return(rc);
}




/**
 * Function to enqueue attribute read jobs in the bundle.
 */


ClRcT clCorBundleObjectGet( CL_IN    ClCorBundleHandleT bundleHandle,
                            CL_IN    const ClCorObjectHandleT *pObjectHandle,
                            CL_INOUT ClCorAttrValueDescriptorListPtrT pAttrDescList)
{
    ClRcT                   rc = CL_OK;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" Inside the bundle attribute get. "));
    
    if((NULL == pObjectHandle) || (NULL == pAttrDescList))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(pAttrDescList->numOfDescriptor <= 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid number of attribute descriptor passed."));
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = _clCorBundleGetJob(bundleHandle, pObjectHandle, pAttrDescList); 
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in the job list. rc[0x%x]", rc));
        return rc;
    }
    
    CL_FUNC_EXIT();
    return rc;
}




/** 
 *  Function to get the attribute type, basic type if attribute is 
 *  a array type and size of an attribute.
 */

ClRcT
clCorObjAttrInfoGet(CL_IN  ClCorObjectHandleT objH, 
                    CL_IN  ClCorAttrPathPtrT pAttrPath,
                    CL_IN  ClCorAttrIdT attrId,
                    CL_OUT ClCorTypeT *attrType, 
                    CL_OUT ClCorAttrTypeT *arrDataType,
                    CL_OUT ClUint32T *attrSize, 
                    CL_OUT ClUint32T *userFlags )
{
    ClRcT                  rc          = CL_OK;
    corObjectInfo_t       *pObjectInfo = NULL;
    ClBufferHandleT inMsgHdl    =  0;
    ClBufferHandleT outMsgHdl   =  0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Inside clCorObjAttrInfoGet \n"));

    /* Validate input parameters */
    if (attrType == NULL || arrDataType == NULL || attrSize == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorObjAttrInfoGet: NULL argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    pObjectInfo = (corObjectInfo_t *) clHeapAllocate(sizeof(corObjectInfo_t));
    if(pObjectInfo)
    {
        memset(pObjectInfo, 0, sizeof(corObjectInfo_t));
		CL_COR_VERSION_SET(pObjectInfo->version);
        pObjectInfo->operation  = COR_OBJ_OP_ATTR_TYPE_SIZE_GET;
        clCorObjectHandleSizeGet(objH, &pObjectInfo->objHSize);
        pObjectInfo->pObjHandle = objH;
        pObjectInfo->attrId     = attrId;

        if(pAttrPath)
            pObjectInfo->attrPath   = *pAttrPath;
    }
    else
    {
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                         CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
              CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
              return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }

    if((rc = clBufferCreate(&inMsgHdl))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to create buffer message. rc [0x%x]\n", rc));
        clHeapFree(pObjectInfo);
        return (rc);
    }

    if((rc = VDECL_VER(clXdrMarshallcorObjectInfo_t, 4, 0, 0)(pObjectInfo, inMsgHdl, 0)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to marshal corObjectInfo_t. rc [0x%x]\n", rc));
        clHeapFree(pObjectInfo);
        return rc;
    }

    if((rc = clBufferCreate(&outMsgHdl))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to create buffer message. rc [0x%x]\n", rc));
        clHeapFree(pObjectInfo);
        return (rc);
    }

    COR_CALL_RMD_SYNC_WITH_MSG(COR_EO_OBJECT_OP, inMsgHdl, outMsgHdl, rc);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get attribute metadata.  rc [0x%x]\n", rc));
    }

    if(rc == CL_OK)
    {
        rc = clXdrUnmarshallClInt32T(outMsgHdl, attrType);
        if(rc == CL_OK)
        {
            rc = clXdrUnmarshallClInt32T(outMsgHdl, arrDataType);
            if(rc == CL_OK)
            {
                rc = clXdrUnmarshallClUint32T(outMsgHdl, attrSize);
                if(rc == CL_OK)
                {
                    rc = clXdrUnmarshallClUint32T(outMsgHdl, userFlags);
                    if(rc != CL_OK)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the userFlags"));
                }
                else
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get attribute arraySize.  rc [0x%x]\n", rc));
            }
            else
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get attribute arraySize.  rc [0x%x]\n", rc));
        }
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get attribute type .  rc [0x%x]\n", rc));
    }
            
    clBufferDelete(&inMsgHdl);
    clBufferDelete(&outMsgHdl);

    clHeapFree(pObjectInfo);
    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorObjectHandleSizeGet(ClCorObjectHandleT objH, ClUint16T* pSize)
{
    if (!objH || !pSize)
    {
        clLogError("OBJ", "GET", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    *pSize = *(ClUint16T *) objH;
    *pSize = ntohs(*pSize);

    return CL_OK;
}
