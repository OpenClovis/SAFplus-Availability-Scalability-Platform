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
 * ModuleName  : med
 * File        : clMedUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module implements Mediation component
 *
 *
 *****************************************************************************/


/** @pkg med*/
#include <clMedApi.h>
#include <clMedDs.h>
#include <clCommonErrors.h>
#include<clCommon.h>
#include <clDebugApi.h>
#include<string.h>
#include <clAlarmDefinitions.h>
#include <clCorTxnApi.h>

#include <ipi/clAlarmIpi.h>

#ifdef MORE_CODE_COVERAGE
#include <clCodeCovStub.h>
#endif

#include <signal.h>
#include<clLogApi.h>
#include<clMedLog.h>
#include <clMedDebug.h>
#define CL_CONTAINMENT_ATTR_VALID_ENTRY 1



#define CL_MED_IDXLN 1   /* Identifier to store MM Object*/
#define MMM  10
#define MAX_ATTR_DEPTH 20

ClInt32T clMedMoIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClCorMOIdPtrT      hId1 = (ClCorMOIdPtrT )key1;
    ClCorMOIdPtrT      hId2 = (ClCorMOIdPtrT )key2;
    ClRcT rc = CL_OK;

    rc =  clCorMoIdCompare(hId1, hId2);
    return rc;
}


/*
 *  Compare routine to compare two watch attributes 
 */
ClInt32T clMedOpCodeCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClMedAgntOpCodeT  *pOp1 = NULL;
    ClMedAgntOpCodeT  *pOp2 = NULL;
    ClInt32T           result = 0;

    CL_FUNC_ENTER();

    CL_ASSERT(0 != key1);
    CL_ASSERT(0 != key2);

    pOp1 = (ClMedAgntOpCodeT *)key1;
    pOp2 = (ClMedAgntOpCodeT *)key2;

    /* Check the opcode first */
    result = pOp1->opCode - pOp2->opCode;

    CL_FUNC_EXIT();
    return result;
}

/*
 *
 *  Compare routine to compare two watch attributes 
 *
 */
ClInt32T clMedWatchIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClMedTgtIdT     *pWatchId1 = NULL;
    ClMedTgtIdT     *pWatchId2 = NULL;
    ClInt32T            result = 0;

    CL_FUNC_ENTER();
    
    CL_ASSERT(0 != key1);
    CL_ASSERT(0 != key2);

    pWatchId1 = (ClMedTgtIdT *)key1;
    pWatchId2 = (ClMedTgtIdT *)key2;

    result = pWatchId1->type - pWatchId2->type;

    /* Different types?? */ 
    if (result != 0) 
    {
        CL_FUNC_EXIT();
        return result;
    }

    /* Types are same */
    switch (pWatchId1->type )
    {
        case CL_MED_XLN_TYPE_COR: 
            {
                result =  memcmp(&pWatchId1->info.corId, 
                        &pWatchId2->info.corId, sizeof(ClMedCorIdT)); 
                break;
            }
        default:
            result =0;
    }
    CL_FUNC_EXIT();
    return (result);
}

/*
 *
 *  Compare routine to compare two error Ids in COR language
 *
 */
ClInt32T clMedErrorIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClMedTgtErrInfoT   *pErrId1 = NULL;
    ClMedTgtErrInfoT   *pErrId2 = NULL;
    ClInt32T            result = 0;

    CL_FUNC_ENTER();

    CL_ASSERT(0 != key1);
    CL_ASSERT(0 != key2);

    pErrId1 = (ClMedTgtErrInfoT *)key1;
    pErrId2 = (ClMedTgtErrInfoT *)key2;

    result = pErrId1->type - pErrId2->type;
    /* Different error types?? */
    if (result != 0) 
    {
        CL_FUNC_EXIT();
        return result;
    }
    /* Types are same */
    CL_FUNC_EXIT();
    return (pErrId1->id - pErrId2->id);
}

/*
 *
 *  Compare routine to compare two attribute Ids in agent language
 *
 */
ClInt32T clMedAttrIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClMedAgentIdT    *id1 = NULL;
    ClMedAgentIdT    *id2 = NULL;
    ClInt32T          result = 0;

    CL_FUNC_ENTER();

    CL_ASSERT(0 != key1);
    CL_ASSERT(0 != key2);

    id1 = (ClMedAgentIdT *)key1;
    id2 = (ClMedAgentIdT *)key2;

    result = id1->len -id2->len;
    if (result != 0) 
    {
        CL_FUNC_EXIT();
        return result;
    }
    result = memcmp( id1->id, id2->id, id1->len); 
    CL_FUNC_EXIT();
    return (result);
}

/*
 *
 *  Compare routine to compare two sessions 
 *
 */
ClInt32T clMedSessionCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return ((ClWordT)key1 - (ClWordT)key2);
}
/*
 *
 *       Unsubscribe an attribute (for a given session)
 *
 */
ClRcT clMedWatchStop(ClMedObjT     *pMm,
                    ClMedTgtIdT   *pTgtId,
                    ClUint32T   sessionId)
{
    ClMedWatchAttrT       *pWatchAttr; /* Attribute to be deleted */
    ClRcT            rc;  /* Return Code */

    CL_FUNC_ENTER();
    /* Find this element in the Watch list*/
    if (CL_OK != (rc = clCntDataForKeyGet (pMm->watchAttrList,
                    (ClCntKeyHandleT) &pTgtId,
                    (ClCntDataHandleT*)&pWatchAttr)))
    {
        /* Over my head. Just pass the error */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
        CL_FUNC_EXIT();
        return rc;
    } /* End of container Get */ 

    if (pWatchAttr == NULL)
    {
        /* No element!! Must be wrong parameters*/
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No translation found"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_WATCH_ATTR_IS_NOT_PRESENT, rc);         
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 			  		 
    }
    rc = clMedWatchSessionDel(pMm, pWatchAttr, sessionId);
    CL_FUNC_EXIT();
    return rc;
}

/*
 *
 *      Delete watch on a session
 *
 */

ClRcT clMedWatchSessionDel(ClMedObjT     *pMm,
                          ClMedWatchAttrT  *pWatchAttr,
                          ClUint32T    sessionId)
{
    ClCntNodeHandleT   burden; /* Sad that this variable is in my code*/
    ClRcT            rc;

    CL_FUNC_ENTER();
    /* Remove the session id */
    if (CL_OK != (rc = clCntNodeFind(pWatchAttr->sessionList,
                    (ClCntKeyHandleT)(ClWordT)sessionId ,
                    (ClCntNodeHandleT*)&burden)))
    {
        /* Over my head. Just pass the error */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_FIND_FAILED, rc);          
        CL_FUNC_EXIT();
        return rc;
    }

    if((rc = clCntNodeDelete(pWatchAttr->sessionList, burden))!= CL_OK)
    {
        /* Over my head. Just pass the error */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED,  rc);         
        CL_FUNC_EXIT();
        return rc;
    }
    if((rc = clCntFirstNodeGet(pWatchAttr->sessionList, &burden))!= CL_OK)
    {

        /* Over my head. Just pass the error */
        if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            CL_FUNC_EXIT();
            return CL_OK;
        }
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_GET_FAILED,  rc);         
        CL_FUNC_EXIT();
        return rc;
    }

    if (burden == 0)
    {
        {
            /* Over my head. Just pass the error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Cor returned failure"));
            CL_FUNC_EXIT();
            return rc;
        }
    }
    return CL_OK;
}
/*
 *
 *   Create a watchAttr
 *
 */
ClRcT clMedWatchAttrCreate(ClMedObjT     *pMm,
                          ClMedAgentIdT  *pNativeId,
                          ClMedTgtIdT   *pTgtId,
                          ClUint32T    sessionId)

{
    ClMedWatchAttrT        *pWatchAttr;
    ClUint32T         *pSession;
    ClRcT             rc = CL_OK;

    CL_FUNC_ENTER();  
    /* Allocate memory for the translation */
    if ((pWatchAttr  = (ClMedWatchAttrT *)clHeapAllocate(sizeof(ClMedWatchAttrT)))== NULL)
    {
        /* Things are real bad.   Return failure */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);          
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 		 		 		 			  		 		  
    }
    pWatchAttr->pNativeId = pNativeId;
    pWatchAttr->pMed = pMm;
    memcpy(&pWatchAttr->tgtId, pTgtId, sizeof(ClMedTgtIdT));

    /* Create Container for the session Ids */

    if (CL_OK != clCntLlistCreate(clMedSessionCompare,
                clMedDummyCallback,
                clMedDummyCallback,
                CL_CNT_UNIQUE_KEY,
                &pWatchAttr->sessionList))
    {
        /* Over my head. Just pass the error */
        clHeapFree(pWatchAttr);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED,  rc);         
        CL_FUNC_EXIT();
        return rc;
    }

    if ((pSession = (ClUint32T *) clHeapAllocate(sizeof(sessionId)))== NULL)
    {
        /* Things are real bad.   Return failure */
        clHeapFree(pWatchAttr); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 		 		 		 			  		 		  
    }

    /* Add the session */
    if (CL_OK != (rc = clCntNodeAdd(pWatchAttr->sessionList,
                    (ClCntKeyHandleT)(ClWordT)sessionId,
                    (ClCntDataHandleT)(ClWordT)*pSession,
                    NULL)))
    {
        /* Over my head. Just pass the error */
        clHeapFree(pSession); 
        clHeapFree(pWatchAttr); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_ADD_FAILED,  rc);         
        CL_FUNC_EXIT();
        return rc;
    } /* End of container Add */

    /* Add this element to the Watch list*/
    if (CL_OK != (rc = clCntNodeAdd(pMm->watchAttrList,
                    (ClCntKeyHandleT) &pWatchAttr->tgtId,
                    (ClCntDataHandleT) pWatchAttr,
                    NULL)))
    {
        /* Over my head. Just pass the error */
        clHeapFree(pSession); 
        clHeapFree(pWatchAttr); 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_ADD_FAILED,  rc);         
        CL_FUNC_EXIT();
        return rc;
    } /* End of container Add */

    /* Subscribe */
    if (pTgtId->type == CL_MED_XLN_TYPE_COR)
    {
        ClCorAttrListT    attrList;
        ClEoExecutionObjT*  hEoObj; /* Eo Obj Pointer */

        attrList.attrCnt = 1; 
        attrList.attr[0] = (ClUint32T) pTgtId->info.corId.attrId[0];

        if ( (rc = clEoMyEoObjectGet(&hEoObj)) != CL_OK)
        {
            /* Over my head. Just pass the error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clEoMyEoObjectGet failed "));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED,  rc);             
            CL_FUNC_EXIT();
            return rc;
        }

        if (CL_OK != (rc = clCorEventSubscribe(gCorEvtChannelHandle,
                        pTgtId->info.corId.moId,
                        NULL,
                        &attrList,
                        CL_COR_OP_SET,
                        (void*) pWatchAttr,
                        (ClEventSubscriptionIdT)(ClWordT)pWatchAttr)))
        {
            /* Over my head. Just pass the error */
            clHeapFree(pWatchAttr);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nSubscripion Failed"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_EVT_SUBSCRIBE_FAILED,  rc);             
            CL_FUNC_EXIT();
            return rc;
        } 
    }
    else
    {   
        /* What should be done??? Perplexed:(*/
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

/* 
 *      
 *    Delete an ID translation object
 *
 */

void clMedIdXlationFree(ClMedIdXlationT    *pIdXlation)
{
    CL_FUNC_ENTER(); 

    /* Free the native Id string */
    clHeapFree(pIdXlation->nativeId.id);

    /* Free the tgt Id array*/
    clHeapFree(pIdXlation->tgtId);

    /* Free the translation*/
    clHeapFree(pIdXlation);
    CL_FUNC_EXIT();
    return;
}


/*
 *
 *      Delete an operation code translation
 *
 */
void  clMedOpCodeXlationFree(ClMedOpCodeXlationT  *pOpXln)
{
    CL_FUNC_ENTER(); 
    clHeapFree(pOpXln->tgtOpCode); 
    clHeapFree(pOpXln);
    CL_FUNC_EXIT();
    return;
}

/*
 *
 *   Session clHeapFree 
 *
 */
ClRcT clMedSessionRelease(ClCntKeyHandleT     key,
                       ClCntDataHandleT    pSession,
                       void                   *dummy,
                       ClUint32T dataLength)
{
    ClUint32T   *tmp;
    CL_FUNC_ENTER();
    tmp = (ClUint32T *)pSession;
    clHeapFree(tmp); 
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 *
 *
 *
 */
ClRcT clMedAllOpCodeXlationDel(ClMedObjT  *pMm)
{
    ClRcT       rc;

    CL_FUNC_ENTER();
    if ((rc = clCntWalk(pMm->opCodeXlationTbl, 
                    clMedOpCodeXlationRelease ,NULL, 0))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_WALK_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }

    if ((rc = clCntDelete(pMm->opCodeXlationTbl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DELETE_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 *
 *   Clean the error Id translation table
 *
 */
ClRcT clMedAllErrIdXlationDel(ClMedObjT   *pMm)
{
    ClRcT    rc;

    CL_FUNC_ENTER();
    if ((rc = clCntWalk(pMm->errIdXlationTbl, 
                    clMedErrXlationRelease ,NULL, 0))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_WALK_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }

    if ((rc = clCntDelete(pMm->errIdXlationTbl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DELETE_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 *
 *  Clean the identifier translation table
 *
 */
ClRcT clMedAllIdXlationDel(ClMedObjT   *pMm)
{
    ClRcT     rc;

    CL_FUNC_ENTER();

    if ((rc = clCntWalk(pMm->idXlationTbl,clMedIdXlationRelease,NULL,0))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_WALK_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }

    if ((rc = clCntDelete(pMm->idXlationTbl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DELETE_FAILED,  rc);        
        CL_FUNC_EXIT();
        return rc;
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 * Function which will look into the error list mapping
 * and convert the error passed into the corresponding mapped
 * return code.
 */
void 
_clMedErrorMapGet (ClMedObjT *pMm,
                   ClRcT    srcErr,
                   ClRcT    *dstErr)
{
    ClMedErrIdXlationT     *pErr = NULL; /* Error Translation */ 
    ClMedTgtErrInfoT        sysErr;       /* System Error */
    ClRcT                   rc;

    CL_FUNC_ENTER();
    /* Find out the translation */
    sysErr.type  =  CL_GET_CID(srcErr);
    sysErr.id    =  CL_GET_ERROR_CODE(srcErr);
    clLogTrace(CL_MED_AREA, NULL," NativeId [0x%x] Error type: [0x%x], id[0x%x]", 
             srcErr, sysErr.type, sysErr.id);
    if (CL_OK != (rc = clCntDataForKeyGet(pMm->errIdXlationTbl,
                    (ClCntKeyHandleT) &sysErr,
                    (ClCntDataHandleT*)&pErr)))
    {
        /* Error!!! Can be ignored */
        clLogError(CL_MED_AREA, NULL,
                "Error in getting data for key{errType [0x%x], errId [0x%x]}",
                sysErr.type,
                sysErr.id);
        CL_FUNC_EXIT();
        return;
    } /* End of container Add */

    if (pErr != NULL)
    {
        *dstErr = pErr->nativeErrorId;
    }

    CL_FUNC_EXIT();
    return ;

}

/*
 *
 *   Perform operation
 *
 */
ClRcT  clMedOpPerform(ClMedObjT          *pMm,
                     ClMedOpCodeXlationT   *pOpXlation,
                     ClMedOpT           *pOpInfo)
{
    ClMedTgtOpCodeT  *pTmpOp = pOpXlation->tgtOpCode; 
    ClRcT          rc = CL_OK;

    CL_FUNC_ENTER();
    if (pTmpOp->type == CL_MED_XLN_TYPE_COR)
    { 
        rc = clMedCorOpPerform(pMm, pOpXlation, pOpInfo);
        _clMedErrorMapGet(pMm, rc, &rc);
    } 
    CL_FUNC_EXIT();
    return rc;
}

/*
 *
 *     Notify Translator 
 *
 */     

ClRcT clMedNotifyHandler(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void *pCookie)
{
    ClRcT              rc = CL_OK;
    ClMedObjT          *pMm = NULL;
    ClMedWatchAttrT       *pWatch = NULL;
    ClCntNodeHandleT  tmpNodeHdl = 0;
    ClUint32T         Session;
    ClCorMOIdPtrT     moIdH;
    ClCorOpsT         op;
    /*  ClCorTxnIdT trans =0; */
    ClMedCorJobInfoT   *jobInfo = NULL;
    ClUint32T      value = 0;
    /*  void* pCookie = NULL; */

    CL_FUNC_ENTER();

    pWatch = (ClMedWatchAttrT *)pCookie;
    pMm    = pWatch->pMed; 

    clCorMoIdAlloc(&moIdH);
    memset(&jobInfo, '\0', sizeof(jobInfo)); 

    rc = clCorTxnJobMoIdGet(corTxnId,  moIdH);
    if (CL_OK != rc)
    {
        clCorMoIdFree(moIdH);
        clLogError(CL_MED_AREA, "NFH", 
                "Failed to get the MoId for the notification. rc[0x%x]", rc);
        return rc;
    }
    
    rc = clCorTxnJobOperationGet(corTxnId, jobId, &op);
    if (CL_OK != rc)
    {
        clCorMoIdFree(moIdH);
        clLogError(CL_MED_AREA, "NFH", 
                "Failed to get the Operation type for the notification. rc[0x%x]", rc);
        return rc;
    }

    clCorTxnJobWalk(corTxnId, clMedTransJobWalk, (void *)&jobInfo);

    /* For all sessions on this watch.. Call the notifier*/ 
    clCntFirstNodeGet (pWatch->sessionList, &tmpNodeHdl);
    while (tmpNodeHdl != 0)
    {
        clCntNodeUserDataGet(pWatch->sessionList,
                tmpNodeHdl,
                (ClCntDataHandleT *) &Session );

        clCntNextNodeGet(pWatch->sessionList, tmpNodeHdl, &tmpNodeHdl);
        memcpy(&value, jobInfo->value, jobInfo->size);
        pMm->fpNotifyHdler(moIdH,
                jobInfo->attrId,
                0,
                (ClCharT *)jobInfo->value,
                jobInfo->size);

        memcpy(&value, jobInfo->value, jobInfo->size);
    }

    clHeapFree(jobInfo);
    clCorMoIdFree(moIdH);
    CL_FUNC_EXIT();
    return CL_OK;
}



/*
 *
 *      Dummy Call back till container lib accepts NULL ptrs
 *
 */                                                                                
void clMedDummyCallback(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    /* Dont clHeapFree any thing here */
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return;
}

/*
 *
 *     Free a target ID memory
 *
 */ 
void clMedTgtIdFree(ClMedTgtIdT   *pTgtId)
{
    CL_FUNC_ENTER();
    clHeapFree(pTgtId);
    CL_FUNC_EXIT();
}

/*
 *
 *     Free an error ID xlation 
 *
 */ 
ClRcT  clMedErrXlationRelease(ClCntKeyHandleT     key,
                           ClCntDataHandleT    err,
                           void                   *dummy,
                           ClUint32T dataLength)
{
    ClMedTgtErrInfoT      *pErr = (ClMedTgtErrInfoT *)err; 
    CL_FUNC_ENTER();
    clHeapFree(pErr);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 *
 *     Free a ID translation  memory
 *
 */ 
ClRcT  clMedIdXlationRelease(ClCntKeyHandleT     key,
                          ClCntDataHandleT    idXlation,
                          void                   *dummy,
                          ClUint32T dataLength)
{
    ClMedIdXlationT   *pId = (ClMedIdXlationT*)idXlation; 

    CL_FUNC_ENTER();
    clCntDelete(pId->instXlationTbl);
    clCntDelete(pId->moIdTbl);
    clMedIdXlationFree(pId);
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 *
 *     Free an OpCode translation
 *
 */ 
ClRcT clMedOpCodeXlationRelease(ClCntKeyHandleT     key,
                             ClCntDataHandleT    idXlation,
                             void                   *dummy,
                             ClUint32T dataLength)
{
    ClMedOpCodeXlationT   *pId = (ClMedOpCodeXlationT *)idXlation; 

    CL_FUNC_ENTER();
    clMedOpCodeXlationFree(pId);
    CL_FUNC_EXIT();

    return (CL_OK);
}


/* 
 *
 *      Translate the error id into client language
 *
 */
void medErrIdXlate(ClMedObjT      *pMm,
    ClUint32T    nativeId,
    ClMedVarBindT  *pVarBind)
{
    _clMedErrorMapGet(pMm, nativeId, &(pVarBind->errId));

    CL_FUNC_EXIT();
    return;
}

/*
 *
 *   Perform COR operation
 *
 */
ClRcT  clMedCorOpPerform(ClMedObjT          *pMm,
                         ClMedOpCodeXlationT   *pOpXlation,
                         ClMedOpT           *pOpInfo)
{
    ClMedTgtOpCodeT     *pTmpOp = pOpXlation->tgtOpCode; 
    ClMedIdXlationT     *pIdXlation;  /* Pointer to the Translation*/
    ClMedVarBindT       *pVarInfo =  pOpInfo->varInfo;
    ClMedAgentIdT       *pAgntId = &pVarInfo->attrId;
    ClCorMOIdT          prevMoId ;
    ClCorMOIdPtrT       pPrevMoId = NULL;
    ClCorObjectHandleT  objHdl;
    ClUint32T           tmpCount = 0;
    ClCorTxnSessionIdT  txnSessionId = 0;
    ClCorMOIdPtrT       pMoId = NULL;
    ClCorAttrPathPtrT   pContainedPath = NULL;
    ClUint32T           instModified=0;
    ClUint16T           count =0;
    ClUint32T           rc = 0, retVal = 0;
    ClCorAttrTypeT      attrType = CL_COR_SIMPLE_ATTR;
    ClUint32T           services = 0;
    ClUint32T           i = 0;
    ClUint32T           j = 0;
    ClCorAttrIdT        corAttrId = 0;
    ClCorClassTypeT     classId = 0;
    ClCorTxnInfoPtrT    txnInfo;
    ClCorAttributeValueListPtrT pCorAttrValDescList = NULL;
    ClCorBundleHandleT  bundleHandle = CL_HANDLE_INVALID_VALUE;

    CL_FUNC_ENTER();

    memset(&prevMoId, 0, sizeof(ClCorMOIdT));
    memset(&objHdl, 0, sizeof(ClCorObjectHandleT));

    txnInfo = (ClCorTxnInfoPtrT)clHeapCalloc(1,pOpInfo->varCount * sizeof(ClCorTxnInfoT) );
    if(NULL == txnInfo)
    {
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not allocate memory.");
        rc = CL_ERR_NO_MEMORY;
        goto exitOnError;
    }

    if((rc = clCorMoIdAlloc(&pMoId)) != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not allocate memory.");
        rc = CL_ERR_NO_MEMORY;
        goto exitOnError;
    }
    if((rc = clCorAttrPathAlloc(&pContainedPath)) != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not allocate memory.");
        rc = CL_ERR_NO_MEMORY;
        goto exitOnError;
    }

    pCorAttrValDescList = (ClCorAttributeValueListPtrT)
    clHeapAllocate(sizeof(ClCorAttributeValueListT));
    if(pCorAttrValDescList == NULL)
    {
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not allocate memory.");
        rc = CL_ERR_NO_MEMORY;
        goto exitOnError;
    }
    memset(pCorAttrValDescList, 0, sizeof (ClCorAttributeValueListT));
    pCorAttrValDescList->pAttributeValue = NULL;

    
    clLogTrace(CL_MED_AREA, NULL," pOpInfo->varCount[%d]", 
            pOpInfo->varCount);
    for (tmpCount =0;tmpCount < pOpInfo->varCount; tmpCount++)
    {
        pAgntId  = &pVarInfo->attrId;

        /* Get the corresponding COR identifier */
        if (CL_OK != (rc = clCntDataForKeyGet(pMm->idXlationTbl,
                                              (ClCntKeyHandleT) pAgntId,
                                              (ClCntDataHandleT *)&pIdXlation)))
        {
            /* Over my head. Just pass the error */
            clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Container Lib returned failure for agentid %s",(char *)pAgntId->id);
            rc = CL_GET_ERROR_CODE(rc);
            medErrIdXlate(pMm, rc, pVarInfo);
            retVal = rc;
            pVarInfo++; /* Get to the next varInfo */
            continue;
        }
        {
            ClUint8T *pChar = (ClUint8T *)pVarInfo->pInst;
            if (pTmpOp->opCode != CL_COR_CREATE && pTmpOp->opCode != CL_COR_CONTAINMENT_SET) 
            {
                if (CL_OK != (rc =  clMedInstFill(pIdXlation,
                                                  pTmpOp->opCode,
                                                  (void*)pChar, pMoId, pContainedPath)))
                {
                    /* Over my head. Just pass the error */
                    /* Think this is the culprit */
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    clLogTrace(CL_MED_AREA, NULL," ErrorId [0x%x] in pVarInfo after instfill", 
                             pVarInfo->errId);
                    retVal = rc;
                    pVarInfo++; /* Get to the next varInfo */
                    continue;
                }
                if(pTmpOp->opCode == CL_COR_GET_FIRST || pTmpOp->opCode == CL_COR_GET_NEXT)
                {
                    pVarInfo++; /* Get to the next varInfo */
                    continue;
                }
                memcpy(&txnInfo[tmpCount].moId, pMoId, sizeof(ClCorMOIdT) );
                memcpy(&txnInfo[tmpCount].attrPath, pContainedPath, sizeof(ClCorAttrPathT) );
                pMoId->svcId = pIdXlation->tgtId->info.corId.moId->svcId;
            }
            if ((0 != pIdXlation->instXlationTbl) && (NULL != pMm->fpInstXlator )  &&
                ((pTmpOp->opCode == CL_COR_CREATE)    || (pTmpOp->opCode == CL_COR_CONTAINMENT_SET) ) )
            {
                ClUint8T *pChar1 = NULL;
                ClUint32T   instLen;
                ClMedInstXlationOpEnumT  instXlnOp = CL_MED_CREATION_BY_OI;

                if(pTmpOp->opCode == CL_COR_CREATE)
                {
                    instXlnOp = CL_MED_CREATION_BY_NORTHBOUND;
                }

                pChar1 = (ClUint8T *)pVarInfo->pInst;

                if (CL_OK != (rc = pMm->fpInstXlator(pAgntId,pMoId, pContainedPath,
                                                     (void**)&pChar1, &instLen, pCorAttrValDescList,instXlnOp, NULL)))
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Instance Translation Failed for agentid %s",(char *)pAgntId->id);
                    CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    retVal = rc;
                    pVarInfo++; /* Get to the next varInfo */
                    continue;
                }
                pVarInfo->pInst = (void*)pChar1;
                pChar = pChar1;
            }
            instModified = 1;
        }
        /* Get the handle of the MSO for all operations but CL_COR_CREATE */
        if (pTmpOp->opCode != CL_COR_CREATE && pTmpOp->opCode != CL_COR_DELETE)
        {
            rc = clCorObjectHandleGet(pMoId, &objHdl);
            if (rc != CL_OK)
            {
                /* Over my head. Just pass the error */
                clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not get object handle for agentid %s",(char *)pAgntId->id);
                CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                rc = CL_GET_ERROR_CODE(rc);
                medErrIdXlate(pMm, rc, pVarInfo);
                retVal = rc;
                pVarInfo++; /* Get to the next varInfo */
                continue;
            }
        }
        corAttrId = pIdXlation->tgtId->info.corId.attrId[0];
        txnInfo[tmpCount].attrId = corAttrId;
            
        clLog(CL_LOG_DEBUG, CL_MED_AREA, CL_MED_CTX_CFG, 
              "Operation SET[%d] GET/FIRST/NEXT[%d/%d/%d] CREATE/DELETE[%d/%d] CURRENT[%d]", 
              CL_COR_SET, CL_COR_GET, CL_COR_GET_FIRST, CL_COR_GET_NEXT, CL_COR_CREATE, CL_COR_DELETE, pTmpOp->opCode );

        CL_MED_DEBUG_XLATE_INFO_PRINT(CL_MED_CREATION_BY_NORTHBOUND, &(pOpInfo->varInfo->attrId), 
                                      (ClMedInstXlationT*)NULL, pMoId, CL_MED_CTX_CFG );

        switch (pTmpOp->opCode)
        {
            case CL_COR_SET:
            {
                ClCorMOServiceIdT svcId = clCorMoIdServiceGet(pMoId);
                ClCorMoIdClassGetFlagsT flag = CL_COR_MO_CLASS_GET;
                clLogTrace(CL_MED_AREA, NULL," Received SET op");
                if(svcId == CL_COR_INVALID_SVC_ID)
                    flag = CL_COR_MO_CLASS_GET;
                else
                    flag = CL_COR_MSO_CLASS_GET;
                pVarInfo->index = 0; /*TBD - Remove this when SNMP/CLI can pass correct index */
                /* Get the attribute type first */
                if(clCorMoIdToClassGet(pMoId, flag, &classId) == CL_OK)
                {
                    /* Get the attribute type first */
                    if( (rc = clCorClassAttributeTypeGet(classId, corAttrId, &attrType)) != CL_OK)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Could not get attribute type for agentid %s Attribute ID [%d] ",
                              (char *)pAgntId->id, corAttrId);
                        
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        retVal = rc;
                        pVarInfo++; /* Get to the next varInfo */
                        continue;
                    }
                }
                if(attrType == CL_COR_CONTAINMENT_ATTR)
                {
                    ClUint32T i = 0;
                    while(pIdXlation->tgtId->info.corId.attrId[i + 1] != CL_MED_ATTR_VALUE_END)
                    {
                        i += 2; /*Skip all the containment attributes*/
                    }
                    corAttrId = pIdXlation->tgtId->info.corId.attrId[i];
                    rc  = clCorObjectAttributeSet(&txnSessionId, objHdl, pContainedPath,
                                                  corAttrId, CL_COR_INVALID_ATTR_IDX, pVarInfo->pVal, pVarInfo->len);
                    if(CL_OK != rc)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor attribute set failed, OID[%s] AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        clCorTxnSessionFinalize(txnSessionId);
                        goto exitOnError;
                    }
                    break;
                }

                if(attrType <= CL_COR_SIMPLE_ATTR)
                {
                    rc  = clCorObjectAttributeSet(&txnSessionId, objHdl, NULL, corAttrId, CL_COR_INVALID_ATTR_IDX,
                                                  pVarInfo->pVal, pVarInfo->len);		
                    if(CL_OK != rc)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor attribute set failed, OID[%s]  AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        clCorTxnSessionFinalize(txnSessionId);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        goto exitOnError;
                    }
                }
                else if(attrType == CL_COR_ARRAY_ATTR)
                {
                    rc = clCorObjectAttributeSet(&txnSessionId, objHdl, NULL, corAttrId, pVarInfo->index, pVarInfo->pVal, pVarInfo->len);
                    if(CL_OK != rc)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor attribute set failed, OID[%s] AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        clCorTxnSessionFinalize(txnSessionId);
                        goto exitOnError;
                    }
                }
                else if(attrType == CL_COR_ASSOCIATION_ATTR)
                {
                    rc  = clCorObjectAttributeSet(&txnSessionId, objHdl, NULL, corAttrId, pVarInfo->index, pVarInfo->pVal, pVarInfo->len);
                    if(CL_OK != rc)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor attribute set failed, OID[%s] AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        clCorTxnSessionFinalize(txnSessionId);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        goto exitOnError;
                    }
                }
                    clLogTrace(CL_MED_AREA, NULL," Successfully added to COR for set");
            }
            break;
            case CL_COR_GET:
            {
                ClCorMOServiceIdT svcId         = clCorMoIdServiceGet(pMoId);
                ClCorMoIdClassGetFlagsT flag    = CL_COR_MO_CLASS_GET;
                ClCorAttrValueDescriptorT       attrDesc = {0};
                ClCorAttrValueDescriptorListT   attrList = {0};
                ClCorBundleConfigT              bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};

                clLogTrace(CL_MED_AREA, NULL," Received GET op");


                if(svcId == CL_COR_INVALID_SVC_ID)
                    flag = CL_COR_MO_CLASS_GET;
                else
                    flag = CL_COR_MSO_CLASS_GET;
                pVarInfo->index = 0; /*TBD - Remove this when SNMP/CLI can pass correct index */

                if((rc = clCorMoIdToClassGet(pMoId, flag, &classId)) == CL_OK)
                {
                    /* Get the attribute type first */
                    if( (rc = clCorClassAttributeTypeGet(classId, corAttrId, &attrType)) != CL_OK)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor attribute get failed, OID[%s] AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        goto exitOnError;
                    }
                }
                else
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                          "Cor attribute get failed, OID[%s] AttributeID[%d] ", (char*)pAgntId->id, corAttrId);
                    CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    goto exitOnError;
                }
                if( (rc = clCorObjectHandleGet(pMoId, &objHdl)) != CL_OK)
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Could not get object handle RC : [%x]",rc);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    goto exitOnError;
                }

                if (bundleHandle == CL_HANDLE_INVALID_VALUE)
                {
                    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
                    if (CL_OK != rc)
                    {
                        clLogError(CL_MED_AREA, CL_MED_CTX_CFG, 
                                "Failed while initializing the bundle. rc[0x%x]",
                                rc);
                        retVal = rc; 
                        break;
                    }
                }

                if(attrType == CL_COR_CONTAINMENT_ATTR)
                {
                    ClUint32T i = 0;
                    while(pIdXlation->tgtId->info.corId.attrId[i + 1] != - 1)
                    {
                        i += 2; /*Skip all the containment attributes*/
                    }
                    corAttrId = pIdXlation->tgtId->info.corId.attrId[i];
                    attrDesc.pAttrPath = pContainedPath;
                    attrDesc.index = CL_COR_INVALID_ATTR_IDX;
                    break;
                }
                else if(attrType == CL_COR_ARRAY_ATTR || (attrType == CL_COR_ASSOCIATION_ATTR))
                {
                    attrDesc.index = pVarInfo->index;
                    attrDesc.pAttrPath = NULL;
                }
                else
                {
                    attrDesc.index = CL_COR_INVALID_ATTR_IDX;
                    attrDesc.pAttrPath = NULL;
                }

                attrDesc.attrId = corAttrId;
                attrDesc.bufferPtr = pVarInfo->pVal;
                attrDesc.bufferSize = pVarInfo->len;
                attrDesc.pJobStatus = &(pVarInfo->errId);

                attrList.numOfDescriptor = 1;
                attrList.pAttrDescriptor = &attrDesc;
                
                clLogDebug(CL_MED_AREA, CL_MED_CTX_CFG, 
                        "Adding the attrId [0x%x] to the bundle for GET operation. ", corAttrId);

                rc = clCorBundleObjectGet(bundleHandle, 
                                        &objHdl, &attrList);
                if(rc != CL_OK)
                {
                    clLogError(CL_MED_AREA, CL_MED_CTX_CFG, 
                          "Failed while adding the job to bundle : "
                          "AttrId [0x%x], pVarInfo->index [%d] pVarInfo->len [%d]. rc[0x%x]",  
                          corAttrId, pVarInfo->index, pVarInfo->len, rc);
                    break;
                }

                clLogDebug(CL_MED_AREA, NULL," Successfully added to COR for get");
            }
            break;

            case CL_COR_DELETE: 
                clCorMoIdServiceSet(pMoId, CL_COR_INVALID_SVC_ID);
                rc =  clCorUtilMoAndMSODelete(pMoId);
                if(rc != CL_OK)
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                          "Cor Object delete failed OID[%s] RC : [%x] ", (char*)pAgntId->id, rc);
                    CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    retVal = rc;
                    continue;
                }
                break;
            case CL_COR_CREATE:
                rc = clCorObjectHandleGet(pMoId, &objHdl);
                if(rc != CL_OK)
                {
                    ClCorMOServiceIdT origSvcId = clCorMoIdServiceGet(pMoId);
                    if(origSvcId != CL_COR_INVALID_SVC_ID)
                    {
                        clCorMoIdServiceSet(pMoId, CL_COR_INVALID_SVC_ID);
                        txnSessionId = 0;
                        rc = clCorObjectCreate(&txnSessionId, pMoId, &objHdl);
                        if(rc != CL_OK)
                        {
                            clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                                  "Cor Object create failed OID[%s] RC : [%x] ", (char*)pAgntId->id, rc);
                            CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                            clCorTxnSessionFinalize(txnSessionId);
                            rc = CL_GET_ERROR_CODE(rc);
                            medErrIdXlate(pMm, rc, pVarInfo);
                            retVal = rc;
                            continue;
                        }
                    }

                    clCorMoIdServiceSet(pMoId, origSvcId);
                    rc = clCorObjectCreateAndSet(&txnSessionId, pMoId, pCorAttrValDescList, &objHdl);
                    if(rc != CL_OK)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Cor Object create failed OID[%s] RC : [%x] ", (char*)pAgntId->id, rc);
                        CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                        clCorTxnSessionFinalize(txnSessionId);
                        rc = CL_GET_ERROR_CODE(rc);
                        medErrIdXlate(pMm, rc, pVarInfo);
                        retVal = rc;
                        continue;
                    }
                }
                else
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Object already exists!");
                    CL_MED_DEBUG_MO_PRINT(pMoId, CL_MED_CTX_CFG);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    retVal = rc;
                }
                break;
            case CL_COR_CONTAINMENT_SET:
            {
                /*Sachin : curently its very specific to alarm.*/
                ClUint32T i = 0;
                while(pIdXlation->tgtId->info.corId.attrId[i + 1] != CL_MED_ATTR_VALUE_END)
                {
                    i += 2; /*Skip all the containment attributes*/
                }
                corAttrId = CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY;
                pVarInfo->len = 4;
                rc  = clCorObjectAttributeSet(&txnSessionId, objHdl, pContainedPath, corAttrId,
                                              CL_COR_INVALID_ATTR_IDX, pVarInfo->pVal, pVarInfo->len);		
                if(CL_OK != rc)
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Cor attribute set failed.");
                    clCorTxnSessionFinalize(txnSessionId);
                    rc = CL_GET_ERROR_CODE(rc);
                    medErrIdXlate(pMm, rc, pVarInfo);
                    goto exitOnError;
                }
            }
            break;
            /* @SVREDDY - get all services to a perticular mo */
            case CL_COR_GET_SVCS:
                services = 0;
                i = 1;
                for(j = 0; j < CL_MED_MAX_SERVICES; j++)
                {
                    rc = clCorMoIdServiceSet(pMoId, j+1);
                    if (rc == CL_OK)
                    {
                        rc = clCorObjectHandleGet(pMoId, &objHdl);
                        if (rc == CL_OK)
                        {
                            services |= i;
                        }
                        i = i << 1;
                    }
                }
                *((ClInt32T *)(pVarInfo->pVal)) = services;
                pVarInfo->len = sizeof(ClInt32T);
                rc = CL_OK;
                break;
            default:
                break;
        }
        /* Copy the MOID for next iteration */
        pPrevMoId = &prevMoId; 
        memcpy(pPrevMoId,pMoId, sizeof(ClCorMOIdT));
        /* Revert the ClCorMOId Instances*/
        if (0 != instModified )
        {
            for (count =0; count < pMoId->depth; count++)
                pMoId->node[count].instance = CL_COR_INSTANCE_WILD_CARD;
        }

        pVarInfo++; /* Get to the next varInfo */
    } /* End of for */
    if (retVal != CL_OK)
    {
        /* No point in executing any further operations */
        rc = CL_GET_ERROR_CODE(retVal);
        goto exitOnError;
    }

    if (pOpInfo->opCode == CL_COR_GET)
    {
        if (bundleHandle != CL_HANDLE_INVALID_VALUE)
        {
            clLogDebug(CL_MED_AREA, CL_MED_CTX_CFG, 
                    "Applying the GET operation using the COR bundle");

            rc = clCorBundleApply(bundleHandle);
            if (CL_OK != rc)
            {
                clLogError(CL_MED_AREA, CL_MED_CTX_CFG, 
                        "Failed while submitting the GET bundle. rc[0x%x]", rc);
                rc = CL_GET_ERROR_CODE(rc);
            }

            pVarInfo = pOpInfo->varInfo;
            for (tmpCount = 0; tmpCount < pOpInfo->varCount; tmpCount++, pVarInfo++)
            {
                if (pVarInfo->errId !=  CL_OK)
                {
                    clLogError(CL_MED_AREA, CL_MED_CTX_CFG,
                            "Failed with error [0x%x] in for GET on attr [0x%x]",
                             pVarInfo->errId, txnInfo[tmpCount].attrId);
                    medErrIdXlate(pMm, CL_GET_ERROR_CODE(pVarInfo->errId) , pVarInfo);
                }
            }

            clCorBundleFinalize(bundleHandle);
            CL_FUNC_EXIT();
            goto exitOnError;
        }
    }
    else 
    {
        if (txnSessionId != 0) 
        {
            ClCorTxnInfoT txnNext;
            clCorAttrPathInitialize(&txnNext.attrPath);
            if (CL_OK != (rc = clMedTxnFinish(txnSessionId)))
            {
                if(pCorAttrValDescList && pCorAttrValDescList->pAttributeValue)
                {
                    for(count = 0; count < pCorAttrValDescList->numOfValues;count++)
                    {
                        ClCorAttributeValuePtrT pAttributeValue = &(pCorAttrValDescList->pAttributeValue[count]);
                        if(pAttributeValue == NULL)
                        {
                            clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "NULL Attribute Descriptor, count = %d", count);
                            continue;
                        }
                        else
                            clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                                  "Attr ID = 0x%x, Job status = NULL",pAttributeValue->attrId);
                        if(pAttributeValue->pAttrPath)
                        {
                            clCorAttrPathShow(pAttributeValue->pAttrPath);
                        }
                    }
                }
                if(CL_OK == (rc = clCorTxnFailedJobGet(txnSessionId, NULL, &txnNext) ) )
                {
                    pVarInfo = pOpInfo->varInfo;
                    for (tmpCount =0;tmpCount < pOpInfo->varCount; tmpCount++,pVarInfo++)
                    {
                        if(txnInfo[tmpCount].attrId == txnNext.attrId)
                        {
                            if(clCorMoIdCompare(&txnInfo[tmpCount].moId, &txnNext.moId ) == CL_OK)
                            {
                                if(clCorAttrPathCompare(&txnInfo[tmpCount].attrPath,  &txnNext.attrPath)  == CL_OK)
                                {
                                    rc = CL_ERR_BAD_OPERATION; /* This needs to map to COR's job status. Update the errDb as well */
                                    medErrIdXlate(pMm, rc, pVarInfo);
                                    clCorTxnSessionFinalize(txnSessionId);
                                    goto exitOnError;
                                }
                            }
                        }
                    }
                    if(tmpCount == pOpInfo->varCount)
                    {
                        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                              "Information regarding failed entry in bulk set doesn't match with information returned.");
                        rc = CL_ERR_BAD_OPERATION;
                        pVarInfo = pOpInfo->varInfo;
                        medErrIdXlate(pMm, rc, pVarInfo);
                        clCorTxnSessionFinalize(txnSessionId);
                        goto exitOnError;
                    }
                }
                else
                {
                    clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, 
                          "Could not get the information of failed entry in bulk set. Error is 0x%x",rc);
                    rc = CL_GET_ERROR_CODE(rc);
                    clCorTxnSessionFinalize(txnSessionId);
                    goto exitOnError;
                }
            }
        }    
    }
    

exitOnError:

    /* Error exit block */
    {
        clLogTrace(CL_MED_AREA, CL_MED_CTX_CFG,
                "Entering exit block");
        /* Clean up of COR attribute value descriptor list. */
        if(pCorAttrValDescList) 
        {
            if(pCorAttrValDescList->pAttributeValue)
            {
                for(count = 0; count < pCorAttrValDescList->numOfValues;count++)
                {
                    ClCorAttributeValuePtrT pAttributeValue = &(pCorAttrValDescList->pAttributeValue[count]);
                    if(pAttributeValue == NULL)
                        continue;
                    clHeapFree(pAttributeValue->pAttrPath);
                    clHeapFree(pAttributeValue->bufferPtr);
                }
                clHeapFree(pCorAttrValDescList->pAttributeValue);
            }
            clHeapFree(pCorAttrValDescList);
        }

        clCorMoIdFree(pMoId);
        clCorAttrPathFree(pContainedPath);
        clHeapFree(txnInfo);

        CL_FUNC_EXIT();
        return rc;
    }
}


/* 
 *   Fill in instances 
 */

ClRcT clMedInstFill(ClMedIdXlationT    *pIdx,   /* Identifier Translation */
                   ClMedCorOpEnumT        opCode,            
                   void*         pInst,   /* User supplied instance*/
                   ClCorMOIdPtrT         pMoId,   /* Result(O/P) ClCorMOId */
                   ClCorAttrPathPtrT    pContainedPath) /*Result(O/P) containment path*/
{
    ClMedInstXlationT    *pInstXln = NULL; /* Actual instance translation  */
    ClUint32T       count=0; /* Loop index */
    ClRcT           rc = CL_OK;  /* Return code */
    ClUint8T* pChar = NULL;

    CL_FUNC_ENTER();
    if (0 != pIdx->instXlationTbl) /* Instance table is not NULL*/
    {
        ClCntNodeHandleT     hNode = 0; /* Handle to the node */
        pChar = (ClUint8T *)pInst;
        clLogDebug(CL_MED_AREA, NULL,
                "Getting node for instance oid[%s]", (ClCharT*)pIdx->nativeId.id);

        rc = clCntNodeFind(pIdx->instXlationTbl,
                (ClCntKeyHandleT) pChar,
                (ClCntDataHandleT*)&hNode);

        if (opCode == CL_COR_GET_FIRST ) /* Get the first index for the same ClCorMOId*/
        {
            if(rc != CL_OK)
            {
                if (CL_OK != (rc = clCntFirstNodeGet(pIdx->instXlationTbl, 
                                &hNode)))
                {
                    CL_FUNC_EXIT();
                    clLogError(CL_MED_AREA, NULL, 
                            "Op GET FIRST, clCntFirstNodeGet returned rc = 0x%x. Cannot find node for instance", 
                            rc);
                    return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
                }
                if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->instXlationTbl,
                                hNode, (ClCntDataHandleT*)&pInstXln)))
                {
                    CL_FUNC_EXIT();
                    clLogError(CL_MED_AREA, NULL, 
                            "Op GET FIRST, clCntNodeUserDataGet returned rc = 0x%x. Failed to get user data", 
                            rc);
                    return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
                }
            }
            else
            {
                clLogDebug("med", NULL, "Node found in the instXlationTbl.");
                return rc;
            }
        }
        else
        {
            clLogInfo(CL_MED_AREA, NULL,
                    "Received GET NEXT op");
            if( rc != CL_OK)
            {
                CL_FUNC_EXIT();
                clLogError(CL_MED_AREA, NULL, 
                        "Op GET NEXT, clCntNodeFind returned rc [0x%x]", rc);
                return CL_MED_SET_RC(rc);
            }
            /* Get the  instance from translation table */
            if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->instXlationTbl,
                            hNode, (ClCntDataHandleT*)&pInstXln)))
            {
                CL_FUNC_EXIT();
                clLogError(CL_MED_AREA, NULL,
                        "Op GET NEXT, clCntNodeUserDataGet returned error [0x%x], no instance found", rc);
                return CL_MED_SET_RC(rc);
            }

            if (opCode == CL_COR_GET_NEXT )
            {
                ClCntNodeHandleT     hNxtNode =0; /* Handle to the next node */

                if (CL_OK != (rc = clCntNextNodeGet(pIdx->instXlationTbl,
                            hNode, &hNxtNode)))
                {
                    CL_FUNC_EXIT();
                    clLogDebug(CL_MED_AREA, NULL, 
                            "Op GET NEXT, next node get failed with error[0x%x]", rc);
                    return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);	
                }

                if (CL_OK != (rc = clCntNodeUserDataGet(pIdx->instXlationTbl,
                                hNxtNode, (ClCntDataHandleT*)&pInstXln)))
                {
                    CL_FUNC_EXIT();
                    clLogError(CL_MED_AREA, NULL, 
                            "Op GET NEXT, no Instance translation found. User data get failed with error [0x%x]", 
                            rc);
                    return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
                }
            }
        }
        memcpy(pMoId, &pInstXln->moId, sizeof(ClCorMOIdT));
        memcpy(pChar, pInstXln->pAgntInst, pInstXln->instLen);

        clCorAttrPathInitialize(pContainedPath);
        if(pInstXln->attrId[0] == CL_MED_ATTR_VALUE_END ||
                pInstXln->attrId[1] == CL_MED_ATTR_VALUE_END)
        {
            pContainedPath = NULL;
        }
        else
        {
            count = 0;
            while(pInstXln->attrId[count + 1] != CL_MED_ATTR_VALUE_END)
            {
                clCorAttrPathAppend(pContainedPath, pInstXln->attrId[count], pInstXln->attrId[count+1]);
                count += 2;
            }
        }
    }
    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 *   Free an instance translation
 */
void medInstanceXlnDel(ClCntKeyHandleT key, ClCntDataHandleT pData)
{
    ClMedInstXlationT    *pInst = NULL;

    CL_FUNC_ENTER();
    pInst = (ClMedInstXlationT *) pData;
    clHeapFree(pInst->pAgntInst);
    clHeapFree(pInst);
    CL_FUNC_EXIT();
    return;
}
void clMedMoIdDelete(ClCntKeyHandleT key, ClCntDataHandleT pData)
{

    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return;
}

void clMedAllWatchAttrDel(ClCntKeyHandleT key, ClCntDataHandleT pData)
{
    CL_FUNC_ENTER();
    ClMedWatchAttrT *pWatchAttr = (ClMedWatchAttrT *)pData;
    clHeapFree(pWatchAttr);
    CL_FUNC_EXIT();
    return;
}

ClRcT clMedCorIndexValueChangeHdlr(ClCorTxnIdT corTxnId, void* pCookie)
{
    ClRcT rc = CL_OK;
    ClMedIdXlationT* pIdXln = NULL;
    ClCorMOIdT moId = {{{0}}}; 
    ClCorMOIdPtrT pMoId = NULL;
    ClCntNodeHandleT currentNode = NULL;
    ClCntNodeHandleT nextNode = NULL;
    ClInt32T cmp = -1;
    ClBoolT existsFlag = CL_FALSE;
    ClCorMOIdT cmpMoId1 = {{{0}}};
    ClCorMOIdT cmpMoId2 = {{{0}}};

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INDEX", "Failed to get MoId from cor txn id. rc [0x%x]", rc);
        return rc;
    }

    pIdXln = (ClMedIdXlationT *) pCookie;
    if (!pIdXln)
    {
        clLogError("SNMP", "INDEX", "NULL cookie passed in the event handler.");
        return CL_MED_SET_RC(CL_ERR_NULL_POINTER); 
    }

    if (pIdXln->pMed->fpInstCompare != NULL)
    {
        rc = clCntFirstNodeGet(pIdXln->moIdTbl, &currentNode);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "INDEX", "Failed to get first node from moIdTbl container. rc [0x%x]", rc);
            return rc; 
        }

        nextNode = currentNode;
        do
        {
            currentNode = nextNode;

            rc = clCntNodeUserDataGet(pIdXln->moIdTbl, currentNode, (ClCntDataHandleT *) &pMoId);
            if (rc != CL_OK)
            {
                clLogError("SNMP", "INDEX", "Failed to get node user data from the container. rc [0x%x]", rc);
                return rc; 
            }

            /* Ignore the service-ids, since when the index attribute change event comes,
             * it would be part of prov mso, but we need to update the attributes of pm mso 
             * (and any other mso services) also for it to be accessible from north bound.
             */
            memcpy(&cmpMoId1, &moId, sizeof(ClCorMOIdT));
            memcpy(&cmpMoId2, pMoId, sizeof(ClCorMOIdT));
            clCorMoIdServiceSet(&cmpMoId1, CL_COR_INVALID_SVC_ID);
            clCorMoIdServiceSet(&cmpMoId2, CL_COR_INVALID_SVC_ID);

            cmp = clCorMoIdCompare(&cmpMoId1, &cmpMoId2);
            if (cmp >= 0)
            {
                /* Exact or wild card match. */
                existsFlag = CL_TRUE;
                break;
            }

            rc = clCntNextNodeGet(pIdXln->moIdTbl, currentNode, &nextNode);
        } while (rc == CL_OK);

        if (rc != CL_OK)
        {
            clLogError("SNMP", "INDEX", "Failed to get node from the container. rc [0x%x]", rc);
            return rc; 
        }

        if (existsFlag == CL_FALSE)
        {
            clLogError("SNMP", "INDEX", "MoId doesn't match with the registered resources in moid table. rc [0x%x]", rc);
            return CL_MED_SET_RC(CL_ERR_NOT_EXIST);
        }
    }

    rc = clMedInstXlnReplace(pIdXln, &moId, NULL, 0);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INDEX", "Failed to replace index values into instance xlation table. rc [0x%x]", rc);
        return rc; 
    }

    return CL_OK; 
}

/*
 *  Notify handler for instance translation  
 */
ClRcT clMedCorObjChangeHdlr(ClCorTxnIdT corTxnId, ClCorTxnJobIdT  jobId, void  *pCookie)
{
    ClMedIdXlationT     *pIdXln; /* Id translation associated with this instance */
    ClCorMOIdPtrT          hMoId = NULL;  /* MOID of the object being changed*/
    ClCorOpsT      operation;
    ClRcT          rc = CL_OK; /* Return code */

    CL_FUNC_ENTER();

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &operation);

    clLog(CL_LOG_DEBUG, CL_MED_AREA, CL_MED_CTX_UTL, "Object life event CREATE[%d]/DELETE[%d] : Current Operation[%d]",
          CL_COR_OP_CREATE, CL_COR_OP_DELETE, operation);

    clCorMoIdAlloc(&hMoId);
    if (CL_OK != (rc = clCorTxnJobMoIdGet(corTxnId,  hMoId))) 
    {
        /* Things are real bad.   Return failure */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Could not get the MOD from Event")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_NOTIFY_EVENT_TO_MOID_GET_FAILED,  rc);
        clCorMoIdFree(hMoId);
        CL_FUNC_EXIT();
        return rc;
    }

    pIdXln = (ClMedIdXlationT *)pCookie;
    /*
    clLogDebug(CL_MED_AREA, "NTF",
            "Received event for instance [%s]", pIdXln->nativeId.id);
            */

    if (pIdXln == NULL)
    {
        /* Things are real bad.   Return failure */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNull Cookie")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_INVALID_PARAMETER);         
        clCorMoIdFree(hMoId);
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NULL_POINTER);
    }
    if (pIdXln->pMed->fpInstCompare != NULL)
    {
        /* Compare the MOIds */
        ClCorMOIdPtrT    xlnId = NULL;
        ClUint16T  indx =0; 
        ClUint32T idx = 0;
        ClUint32T found = 0;
        ClCntNodeHandleT     currentNode = 0;
        ClCntNodeHandleT     nextNode = 0;
        ClCorMOIdPtrT hTempMoid = NULL;
        ClUint32T containerSize = 0;
        if ((rc = clCntFirstNodeGet(pIdXln->moIdTbl, &currentNode)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_GET_FAILED, rc);            
            clCorMoIdFree(hMoId);
            CL_FUNC_EXIT();
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
        }

        clCntSizeGet(pIdXln->moIdTbl, &containerSize);
        for(idx = 0; idx < containerSize; idx++)
        {
            if (CL_OK != (rc = clCntNodeUserDataGet(pIdXln->moIdTbl, currentNode, (ClCntDataHandleT*)&hTempMoid)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);                
                clCorMoIdFree(hMoId);
                CL_FUNC_EXIT();
                return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
            }
            xlnId = hTempMoid;  
            if (xlnId->depth != hMoId->depth)
            {
                found = 0;
                clCntNextNodeGet(pIdXln->moIdTbl, currentNode, &nextNode);
                currentNode = nextNode;				
                continue;
            }
            for (indx = 0; indx < xlnId->depth; indx++)
            {
                if (xlnId->node[indx].type !=  hMoId->node[indx].type) 
                {
                    found = 0;
                    break;
                }
                found = 1;
            }
            if(found == 1)
                break;
            clCntNextNodeGet(pIdXln->moIdTbl, currentNode, &nextNode);
            currentNode = nextNode;
        }
        if(found == 0)
        {
            clCorMoIdFree(hMoId);
            return CL_OK;
        }

    }
    switch (operation)
    {
        case CL_COR_OP_CREATE:
            {
                clCorMoIdShow(hMoId);
                clLogDebug(CL_MED_AREA, NULL,
                        "Calling clMedInstXlnAdd");
                if (CL_OK != (rc = clMedInstXlnAdd(pIdXln, hMoId,0, 0)))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNull Cookie")); 
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_INSERT_FAILED, rc);             
                    CL_FUNC_EXIT();
                    return rc;
                }
                break;
            } /* End of case CL_COR_OP_CREATE*/
        case  CL_COR_OP_SET:
            {
                ClCorAttrPathPtrT pAttrPath = NULL;
                ClCorObjectHandleT handle;
                ClUint32T value = 0;
                ClUint32T size = 4; 

                if( (rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &pAttrPath) ) != CL_OK) 
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rCould not get Attribute Path from notify event.")); 
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_EVENT_TO_ATTR_PATH_GET_FAILED, rc);
                    clCorMoIdFree(hMoId);
                    return rc;
                }

                if( (rc = clCorTxnJobObjectHandleGet(corTxnId, &handle) ) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rCould not get Object handle.")); 
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_OBJ_HANDLE_GET_FAILED, rc);
                    clCorMoIdFree(hMoId);
                    return rc;
                }

                if( (rc = clCorObjectAttributeGet(handle, pAttrPath,
                                CL_CONTAINMENT_ATTR_VALID_ENTRY, CL_COR_INVALID_ATTR_IDX,
                                (void *)(&value), &size ) ) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rCould not get Attribute value.")); 
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_ATTR_GET_FAILED, rc);
                    clCorAttrPathShow(pAttrPath);
                    clCorMoIdFree(hMoId);
                    return rc;
                }

                if(value)
                {
                    if (CL_OK != (rc = clMedInstXlnAdd(pIdXln, hMoId, pAttrPath, 0)))
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not add new instance of object on notification"));
                        clCorMoIdShow(hMoId);
                        clCorAttrPathShow(pAttrPath);
                        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_INSERT_FAILED, rc);
                        clCorMoIdFree(hMoId);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                else
                {
                    if (CL_OK != (rc = clMedInstXlnDelete(pIdXln, hMoId, pAttrPath, 0)))
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not delete instance of the object"));
                        clCorMoIdShow(hMoId);
                        clCorAttrPathShow(pAttrPath);
                        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_DELETE_FAILED, rc);
                        clCorMoIdFree(hMoId);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                break;
            }

        case CL_COR_OP_DELETE:
            {
                if (CL_OK != (rc = clMedInstXlnDelete(pIdXln, hMoId,0, 0)))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNull Cookie"));
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_DELETE_FAILED, rc);                
                    CL_FUNC_EXIT();
                    return rc;
                }
                break;
            } /* End of case CL_COR_OP_DELETE*/
        default :
            {
                break;
            }
    } /* End of switch (operation) */
    clCorMoIdFree(hMoId);
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clMedInstXlnDataGet(ClMedIdXlationT  *pIdXln,
                        ClCorMOIdPtrT  hMoId,
                        ClCorAttrPathPtrT pAttrPath,
                        ClMedInstXlationOpEnumT  instXlnOp,
                        ClMedInstXlationT   **pInstXln)
{
    ClMedAgentIdT       *pAgntId = NULL;      /* Identifier in agent language */
    ClCorAttrPathPtrT   pTmpAttrPath = NULL;
    ClUint32T           instLen = 0;
    void                *pInst = NULL;
    ClUint32T           count = 0;
    ClUint32T           depth = 0; 
    ClRcT               rc = CL_OK; /* Return value */ 

    if ((hMoId == NULL) || ( pIdXln == NULL))
        return CL_ERR_NULL_POINTER;

    pAgntId  = &pIdXln->nativeId;

    if(pAttrPath)
    {
        clCorAttrPathClone(pAttrPath, &pTmpAttrPath);
    }
    rc = pIdXln->pMed->fpInstXlator(pAgntId, hMoId, pTmpAttrPath, &pInst, &instLen, NULL, instXlnOp, NULL);
    if(CL_OK != rc && CL_ERR_TRY_AGAIN != rc)
    {
        if(rc != CL_ERR_NOT_EXIST)
        {
            /* Things are real bad.   Return failure */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Instance Translator failed "));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_INSTANCE_XLN_CALLBACK_FAILED, rc);
        }
        clCorAttrPathFree(pTmpAttrPath);
        CL_FUNC_EXIT();
        if(pInst)
            clHeapFree(pInst); /* If the translator malloc'd and then returned error */
        return rc;
    }
    if (pInst == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Null pointer")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_0_ID_XLN_INDEX_NULL);       
        clCorAttrPathFree(pTmpAttrPath);
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NULL_POINTER);	   
    }
    if (instLen == 0)
    {
        clHeapFree(pInst);
        clCorAttrPathFree(pTmpAttrPath);
        return CL_OK; 
    }
    if (NULL == (*pInstXln = (ClMedInstXlationT *)clHeapAllocate(sizeof(ClMedInstXlationT))))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r No Memory")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);       
        clHeapFree(pInst);
        clCorAttrPathFree(pTmpAttrPath);
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);
    }

    if (NULL == ( (*pInstXln)->pAgntInst = (void *)clHeapAllocate(instLen)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r No Memory")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);       
        clHeapFree(pInst);
        clHeapFree(*pInstXln);
        clCorAttrPathFree(pTmpAttrPath);
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);
    }
    /* Copy the Instance pointer contents */
    memcpy( (*pInstXln)->pAgntInst, pInst, instLen);
    clHeapFree(pInst);      /*pInst is not required after this point*/
    (*pInstXln)->instLen = instLen;
    clCorMoIdInitialize(&(*pInstXln)->moId);
    memcpy(&(*pInstXln)->moId, hMoId, sizeof(ClCorMOIdT));

    if(pTmpAttrPath != NULL)
    {
        depth = clCorAttrPathDepthGet(pTmpAttrPath);
        count = (depth << 1) - 1;
        while(depth--)
        {
            (*pInstXln)->attrId[count--] = clCorAttrPathIndexGet(pTmpAttrPath);
            (*pInstXln)->attrId[count--] = clCorAttrPathToAttrIdGet(pTmpAttrPath);
            clCorAttrPathTruncate(pTmpAttrPath, (clCorAttrPathDepthGet(pTmpAttrPath) - 1) );

        }
        depth = 0;
        while(pIdXln->tgtId->info.corId.attrId[depth + 1] != - 1)
        {
            depth += 2; /*Skip all the containment attributes*/
        }
    }
    (*pInstXln)->attrId[depth] = pIdXln->tgtId->info.corId.attrId[depth];
    (*pInstXln)->attrId[depth + 1] = CL_COR_INVALID_ATTR_IDX;
    clCorAttrPathFree(pTmpAttrPath);
    CL_MED_DEBUG_XLATE_INFO_PRINT(instXlnOp, pAgntId, (*pInstXln), &((*pInstXln)->moId), CL_MED_CTX_CFG);
    return rc;
}

ClRcT clMedInstXlnReplace(ClMedIdXlationT* pIdXln,
                    ClCorMOIdPtrT pMoId,
                    ClCorAttrPathPtrT pAttrPath,
                    ClMedInstXlationOpEnumT instXlnOp)
{
    ClMedInstXlationT* pInstXln = NULL;
    ClMedInstXlationT* pTempInstXln = NULL;
    ClRcT rc = CL_OK;
    ClCntNodeHandleT currentNode = NULL;
    ClCntNodeHandleT nextNode = NULL;

    rc = clMedInstXlnDataGet(pIdXln, pMoId, pAttrPath, instXlnOp, &pInstXln);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INSTREP", "Failed to get instance translation data. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntFirstNodeGet((ClCntHandleT) pIdXln->instXlationTbl, &currentNode);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INSTREP", "Failed to get first node from instance Xlation table. rc [0x%x]", rc);
        return rc;
    }

    nextNode = currentNode;

    do
    {
        currentNode = nextNode;

        rc = clCntNodeUserDataGet((ClCntHandleT) pIdXln->instXlationTbl, currentNode, (ClCntDataHandleT *) &pTempInstXln);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "INSTREP", "Failed to get user data from instance Xlation table. rc [0x%x]", rc);
            return rc;
        }

        if (! clCorMoIdCompare(&pInstXln->moId, &pTempInstXln->moId))
        {
            break;
        }

        rc = clCntNextNodeGet((ClCntHandleT) pIdXln->instXlationTbl, currentNode, &nextNode);
    } while (rc == CL_OK);

    if (rc != CL_OK)
    {
        clLogError("SNMP", "INSTREP", "Failed to get next node from the instance Xlation table. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntNodeDelete((ClCntHandleT) pIdXln->instXlationTbl, currentNode);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INSTREP", "Failed to delete node in the instance translation table. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntNodeAdd((ClCntHandleT) pIdXln->instXlationTbl,
            (ClCntKeyHandleT) pInstXln->pAgntInst,
            (ClCntDataHandleT) pInstXln,
            NULL);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "INSTREP", "Failed to add node to instance translation table. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

/*
 *     Add instance translation
 */
ClRcT clMedInstXlnAdd(ClMedIdXlationT  *pIdXln, 
                     ClCorMOIdPtrT  hMoId,
                     ClCorAttrPathPtrT pAttrPath,
                     ClMedInstXlationOpEnumT  instXlnOp)
{
    ClMedInstXlationT   *pInstXln;     /* Instance Translation */
    ClRcT          rc = CL_OK; /* Return value */ 

    if( (rc = clMedInstXlnDataGet(pIdXln, hMoId, pAttrPath, instXlnOp, &pInstXln) ) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Could not get data for inserting in Instance translation table"));
        return rc;
    }
    clLogDebug(CL_MED_AREA, NULL,
            "Adding to instXlationTbl");
    if (CL_OK != (rc = clCntNodeAdd(
                    (ClCntHandleT)(pIdXln->instXlationTbl),
                    (ClCntKeyHandleT)pInstXln->pAgntInst,
                    (ClCntDataHandleT)pInstXln,
                    NULL)))
    {


        if ( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
        {
            clLogInfo(CL_MED_AREA, NULL, 
                    "Failed to add instance translation as the entry is duplicate."); 
            clHeapFree( pInstXln->pAgntInst);
            clHeapFree( pInstXln);
            return CL_OK;
        }

        /* Container Lib failed!!!.   Return failure */
        clLogError(CL_MED_AREA, NULL, CL_LOG_MESSAGE_1_CNT_NODE_ADD_FAILED, rc);       
        CL_FUNC_EXIT();
        return rc;
    }
    return rc;
}


/*
 *     Delete instance translation
 */
ClRcT clMedInstXlnDelete(ClMedIdXlationT  *pIdXln, 
                     ClCorMOIdPtrT  hMoId,
                     ClCorAttrPathPtrT pAttrPath,
                     ClMedInstXlationOpEnumT  instXlnOp)
{
    ClMedInstXlationT       *pInstXln;     /* Instance Translation */
    ClRcT               rc = CL_OK; /* Return value */
    ClCntNodeHandleT     hNode = 0, hNxtNode = 0;

    if (CL_OK != (rc = clCntFirstNodeGet(pIdXln->instXlationTbl, 
                    &hNode)))
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_WARN, ("clCntFirstNodeGet failed, rc = 0x%x", rc));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
    }

    hNxtNode = hNode;

    while(1)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(pIdXln->instXlationTbl,
                        hNxtNode, (ClCntDataHandleT*)&pInstXln)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_WARN, ("clCntNodeUserDataGet failed, rc = 0x%x", rc));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED,  rc);
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
        }

        if(clCorMoIdCompare(hMoId, &pInstXln->moId) == 0)
        {
            CL_MED_DEBUG_XLATE_INFO_PRINT( instXlnOp, &(pIdXln->nativeId), pInstXln, &((pInstXln)->moId), CL_MED_CTX_CFG );
            /* Match found. Break while to Delete node. */
            break;
        }
        if (CL_OK != (rc = clCntNextNodeGet(pIdXln->instXlationTbl,
                        hNode, &hNxtNode)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_WARN, ("clCntNextNodeGet failed, rc = 0x%x", rc));
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);	
        }
        hNode = hNxtNode;
    }

    /* All Set!! Now delete this translation from the translation table */
    if (CL_OK != (rc = clCntNodeDelete( (ClCntHandleT)(pIdXln->instXlationTbl), hNxtNode) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Could not delete node. rc = 0x%x", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_DELETE_FAILED, rc);
        return rc;
    }
    return rc;
}

ClRcT clMedCorObjWalkHdler(void*  pData , void *objCookie)
{
    ClCorObjectHandleT   hObj;
    ClEoExecutionObjT*   hEO = NULL;
    ClCorMOIdPtrT        pMoId;
    ClCorMOIdPtrT        tmpMoId;
    ClCorServiceIdT      tmpSvc = CL_COR_INVALID_SRVC_ID;
    ClMedIdXlationT          *pIdXlation = NULL; /* Associated Id translation */
    ClRcT                rc = CL_OK;
    ClUint32T idx = 0;
    ClCntNodeHandleT     currentNode = 0;
    ClCntNodeHandleT     nextNode = 0;
    ClCorMOIdPtrT temphMoid = NULL;
    ClUint32T containerSize = 0;
    ClCorObjAttrWalkFilterT  attrWalkFilter; 
    void *cookie = NULL;


    if (NULL == pData)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r NULL Pointer")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);       
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER ;
    }
    hObj = *(ClCorObjectHandleT*)pData;

    if (CL_OK != (rc = clEoMyEoObjectGet(&hEO)))
    {
        /* Failed to store pMM in EO Specific data */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to store Mediation object ")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);       
        CL_FUNC_EXIT();
        return rc;
    }
    /* Get the medObject */
    if (CL_OK != (rc =clEoClientDataGet(hEO, CL_MED_IDXLN, (ClEoDataT*)&pIdXlation)))
    {
        /* Failed to get pMM from EO Specific data */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to Read Mediation object ")); 
        CL_FUNC_EXIT();
        return rc;
    }

    clCorMoIdAlloc(&pMoId);
    if (CL_OK != (rc = clCorObjectHandleToMoIdGet(hObj, pMoId,  &tmpSvc)))
    {

        /* Failed to get pMM from EO Specific data */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to get MOID from obj handle")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_OBJ_HANDLE_TO_MOID_GET_FAILED, rc);       
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return rc;
    }
    clCorMoIdClone(pMoId, &tmpMoId);

    /* Reset the instances in the MOID */
    {
        ClUint16T   count=0;
        for (count =0; count <  tmpMoId->depth; count++)
        {
            tmpMoId->node[count].instance = CL_COR_INSTANCE_WILD_CARD;
        }
        tmpMoId->svcId = pIdXlation->tgtId->info.corId.moId->svcId;
    }

    if (clCntFirstNodeGet(pIdXlation->moIdTbl, &currentNode) != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_GET_FAILED, rc);        
    }
    clCntSizeGet(pIdXlation->moIdTbl, &containerSize);
    for(idx = 0; idx < containerSize; idx++)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(pIdXlation->moIdTbl, currentNode, (ClCntDataHandleT*)&temphMoid)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        }


        rc = clCorMoIdCompare(tmpMoId, temphMoid);
        if (0 == rc || 1 == rc)
        {
            if(pIdXlation->tgtId->info.corId.attrId[0] != CL_MED_ATTR_VALUE_END &&
                    pIdXlation->tgtId->info.corId.attrId[1] != CL_MED_ATTR_VALUE_END)
            {
                ClInt32T i = 0;
                ClCorAttrPathPtrT pAttrPath = NULL;

                rc = clCorAttrPathAlloc(&pAttrPath);
                if (rc != CL_OK)
                {
                    clLogError("MED", "", "Failed to allocate attribute path. rc [0x%x]", rc);
                    clHeapFree(pMoId);
                    clHeapFree(tmpMoId);
                    return rc;
                }

                while(pIdXlation->tgtId->info.corId.attrId[i+1] != CL_MED_ATTR_VALUE_END)
                {
                    clCorAttrPathAppend(pAttrPath, pIdXlation->tgtId->info.corId.attrId[i], pIdXlation->tgtId->info.corId.attrId[i+1]);
                    i += 2;
                }
                pMoId->svcId = tmpMoId->svcId;

                /** COR attribute walk filter ***/
                attrWalkFilter.baseAttrWalk =  CL_FALSE;
                attrWalkFilter.pAttrPath    =  pAttrPath;
                attrWalkFilter.contAttrWalk =  CL_TRUE;
                attrWalkFilter.attrId       =  CL_COR_INVALID_ATTR_ID; /* CL_CONTAINMENT_ATTR_VALID_ENTRY; */
                attrWalkFilter.index        =  CL_COR_INVALID_ATTR_IDX;
                attrWalkFilter.cmpFlag      =  CL_COR_ATTR_CMP_FLAG_INVALID; /* CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO; */
                attrWalkFilter.attrWalkOption  =  CL_COR_ATTR_WALK_ALL_ATTR; /* CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR;  */
                attrWalkFilter.value        =  0 ; 
                attrWalkFilter.size         = 0 ;
                cookie = pMoId;

                rc = clCorObjectHandleGet(pMoId, &hObj);
                if (rc == CL_OK)
                {
                    rc = clCorObjectAttributeWalk(hObj, &attrWalkFilter, clMedCorConaintedAttrWalkHdler, cookie);
                    if (rc != CL_OK)
                    {
                        clLogError("MED", "", "Failed to do object attribute walk. rc [0x%x]", rc);
                    }
                }

                clCorMoIdFree(pMoId);
                clCorMoIdFree(tmpMoId);
                clCorAttrPathFree(pAttrPath);

                return CL_OK;
            }
            else
            {
                if (CL_OK != (rc = clMedInstXlnAdd(pIdXlation, pMoId, NULL, 0)))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to Add translation ")); 
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_INSERT_FAILED, rc);
                    continue;
                }
            }
            /*todo : once this is successful, we whould break from the loop.*/
        }
        clCntNextNodeGet(pIdXlation->moIdTbl, currentNode, &nextNode);
        currentNode = nextNode;			
    }

    clCorMoIdFree(pMoId);
    clCorMoIdFree(tmpMoId);
    return CL_OK;
} /*End of function  */

/*
 *      Bulk Notification for the pre-created objects
 */

ClRcT clMedPreCreatedObjNtfy(ClMedObjT     *pMm, 
                            ClMedIdXlationT  *pIdXlation)
{
    ClRcT            rc = CL_OK;
    ClEoExecutionObjT* hEO = NULL;
    ClCorMOIdPtrT      tmpMoId = NULL; 

    /*
       1.  Store the medObj in EO SPecific data (will be used later)
       2.  Do a walk on all objects 
     */
    if (CL_OK != (rc = clEoMyEoObjectGet(&hEO)))
    {
        /* Failed to store pMM in EO Specific data */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to store Mediation object ")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);       
        CL_FUNC_EXIT();
        return rc;
    }

    if (CL_OK != (rc = clEoClientDataSet(hEO, CL_MED_IDXLN, (ClEoDataT)pIdXlation)))
    {
        /* Failed to store pMM in EO Specific data */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to store Mediation object ")); 
        CL_FUNC_EXIT();
        return rc;
    }
    /* Bug ID : 6151
     * Requirement : Mediation library construct its OID to MOID mapping while it comes up. 
     * Problem     : It walks through entire COR object to construct this mapping. Which is really costly.
     *               Because of this CPU utilization increasing too much.
     * Solution    : We should optimize this walk logic. So just walk only MO tree which Mediation is interested.
     */

    /* Copy the MOID which is interested by Mediation. And set the Service ID to INVALID(-1) for walking that object */
    
    rc = clCorMoIdClone( pIdXlation->tgtId->info.corId.moId, &tmpMoId );
    tmpMoId->svcId = CL_COR_INVALID_SRVC_ID;

    rc = clCorObjectWalk(NULL, tmpMoId, clMedCorObjWalkHdler, CL_COR_MO_WALK, NULL);
    if (CL_OK != rc)
    {
       /* Walk failed */
        clCorMoIdFree( tmpMoId);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to walk the pre-created objects")); 
        CL_FUNC_EXIT();
        return rc;
    }
    clCorMoIdFree( tmpMoId);
    return rc;
}


ClRcT clMedTransJobWalk(ClCorTxnIdT    corTxnId,
                       ClCorTxnJobIdT  jobId,
                       void           *cookie)
{
    ClMedCorJobInfoT   *pJobInfo;
    ClUint32T  value=0;
    ClCorOpsT  op; 

    /* if (NULL != jobPtr) */
    if (0 != jobId)
    {
        clCorTxnJobOperationGet(corTxnId, jobId, &op);
        if (CL_COR_OP_SET == op)
        {
            ClCorAttrIdT        attrId;
            ClInt32T            index;
            void               *pValue;
            ClUint32T           size;

            clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &size);

            if (cookie != 0)
            {
                pJobInfo  = (ClMedCorJobInfoT *)clHeapAllocate(sizeof(ClMedCorJobInfoT) +  size);
                pJobInfo->attrId = attrId;
                pJobInfo->size   = size;
                memcpy(pJobInfo->value, pValue, size);
                memcpy(&value, pValue, size);
                *(ClMedCorJobInfoT **)cookie = pJobInfo;
            }
        }
    }
    return CL_OK;
}

/* 
 *     Commit an existing transaction and start a new one
 */

ClRcT  clMedTxnFinish(ClCorTxnSessionIdT txnSessionId)
{
    ClRcT   rc  = CL_OK;

    if (txnSessionId != 0)
    {
        if ((rc = clCorTxnSessionCommit(txnSessionId)) != CL_OK)
        {
            /* Over my head. Just pass the error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nclCorTxnSessionCommit : Failed\n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_TXN_COMMIT_FAILED, rc);            
            return rc;
        }
        clLogTrace(CL_MED_AREA, NULL," Successfully committed");
    }
    return rc;
}

ClRcT clMedContainmentAttrNotify(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void *pCookie)
{
    ClMedObjT *pMm;
    ClMedIdXlationT  *pIdXlation;
    ClCorMOIdPtrT   pMoId = NULL;
    ClCorAttrPathPtrT pAttrPath = NULL;
    ClCorObjectHandleT objHandle;
    ClUint32T index = 0; /* alarmId */
    ClAlarmSpecificProblemT specProb = 0;
    ClUint32T attr = 0; /* attr value */
    ClUint32T value = 0;
    ClUint32T size = sizeof(value);
    ClRcT retCode = 0;

    CL_FUNC_ENTER();

    pIdXlation    = (ClMedIdXlationT *)pCookie;
    if(pIdXlation == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clMedContainmentAttrNotify :: cookie is NULL"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_INVALID_PARAMETER);        
        return CL_ERR_NULL_POINTER;
    }
    pMm = pIdXlation->pMed;

    /* get moid from event */
    clCorMoIdAlloc(&pMoId);
    if ((retCode = clCorTxnJobMoIdGet(corTxnId, pMoId)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorNotifyEventToMoIdGet failed"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_NOTIFY_EVENT_TO_MOID_GET_FAILED, retCode);        
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return retCode;
    }

    if ((retCode = clCorTxnJobAttrPathGet(corTxnId,
                    jobId, &pAttrPath)) != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_EVENT_TO_ATTR_PATH_GET_FAILED, retCode); 
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return retCode;
    }
    if ((retCode = clCorTxnJobObjectHandleGet(corTxnId,
                    &objHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorTxnJobObjectHandleGet failed"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_OBJ_HANDLE_GET_FAILED, retCode);        
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return retCode;
    }

    /*
     * Get the alarm Id - (prob cause and spec problem)
     */
    if ((retCode = clCorObjectAttributeGet(objHandle,
                    pAttrPath,
                    CL_ALARM_PROBABLE_CAUSE,
                    CL_COR_INVALID_ATTR_IDX, 
                    &value,
                    &size)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorObjectSimpleAtributeGet failed"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_ATTR_GET_FAILED, retCode);        
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return retCode;
    }
    index = value;
    
    size = sizeof(ClAlarmSpecificProblemT);

    if ((retCode = clCorObjectAttributeGet(objHandle,
                    pAttrPath,
                    CL_ALARM_SPECIFIC_PROBLEM,
                    CL_COR_INVALID_ATTR_IDX, 
                    &specProb,
                    &size)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorObjectSimpleAtributeGet failed"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_ATTR_GET_FAILED, retCode);        
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return retCode;
    }

    /* get attr value */
    if ((retCode = clCorObjectAttributeGet(objHandle,
                    pAttrPath,
                    CL_ALARM_ACTIVE,
                    CL_COR_INVALID_ATTR_IDX, 
                    &value,
                    &size)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorObjectSimpleAttributeGet failed"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_COR_ATTR_GET_FAILED, retCode);        
        clCorMoIdFree(pMoId);
        /*   clCorAttrPathFree(pAttrPath); */
        CL_FUNC_EXIT();
        return retCode;
    }
    attr = value;

    pMm->fpNotifyHdler(pMoId, index, specProb, (ClCharT *)&attr, sizeof(attr));

    clCorMoIdFree(pMoId);
    return(retCode);
}

ClRcT clMedCorConaintedAttrWalkHdler(ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId, ClCorAttrTypeT attrType,
              ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT attrData, void *cookie)
{
    ClRcT            rc = CL_OK;
    ClEoExecutionObjT* hEO = NULL;
    ClMedIdXlationT      *pIdXlation = NULL; /* Associated Id translation */
    ClCorMOIdPtrT     pMoId = (ClCorMOIdPtrT)cookie;


    if(attrId == CL_CONTAINMENT_ATTR_VALID_ENTRY)
    {	
        if(*(ClUint8T *)value == CL_TRUE)
        {
            if (CL_OK != (rc = clEoMyEoObjectGet(&hEO)))
            {
                /* Failed to store pMM in EO Specific data */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to store Mediation object ")); 
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);       
                CL_FUNC_EXIT();
                return rc;
            }
            /* Get the medObject */
            if (CL_OK != (rc =clEoClientDataGet(hEO, CL_MED_IDXLN, (ClEoDataT*)&pIdXlation)))
            {
                /* Failed to get pMM from EO Specific data */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to Read Mediation object ")); 
                CL_FUNC_EXIT();
                return rc;
            }
            if (CL_OK != (rc = clMedInstXlnAdd(pIdXlation, pMoId, pAttrPath, 0)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rFailed to Add translation ")); 
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_INSERT_FAILED, rc);        
            }
        }
    }
    return rc;
}

ClRcT clMedTgtIdGet(CL_IN  ClMedHdlPtrT pMedHdl, 
                    CL_IN ClMedAgentIdT *pAgntId, 
                    CL_OUT ClMedTgtIdT **ppTgtId)
{
    ClRcT rc = CL_OK;
    ClMedObjT * pMm = (ClMedObjT *) pMedHdl;

    ClCntNodeHandleT  nodeHdl = 0;
    ClMedIdXlationT   *pIdXlation = NULL;

    if(pMm == NULL || pAgntId == NULL || ppTgtId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received."));
        return CL_ERR_NULL_POINTER;
    }

    if (CL_OK != (rc = clCntNodeFind(pMm->idXlationTbl, (ClCntKeyHandleT)pAgntId,  &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("clCntNodeFind returned error rc = 0x%x for id [%.*s]", (ClUint32T)rc,
                        pAgntId->len, pAgntId->id));
        return rc;
    }

    if(CL_OK != (rc = clCntNodeUserDataGet(pMm->idXlationTbl, nodeHdl,  (ClCntDataHandleT*)&pIdXlation)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntNodeUserDataGet returned error rc = 0x%x", (ClUint32T)rc));
        return rc;
    }

    *ppTgtId = (ClMedTgtIdT *) clHeapAllocate(sizeof(ClMedTgtIdT));
    if(*ppTgtId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory... size = %zd", sizeof(ClMedTgtIdT)));
        return CL_ERR_NO_MEMORY;
    }

    memcpy(*ppTgtId, pIdXlation->tgtId, sizeof(ClMedTgtIdT));

    return CL_OK;
}




