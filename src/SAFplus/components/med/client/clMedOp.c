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
 * ModuleName  : med
 * File        : clMedOp.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * FIXME: This module implements Mediation component
 *
 *
 *****************************************************************************/

/** @pkg med*/
#include <clMedApi.h>
#include <clMedDs.h>
#include <clCommonErrors.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBitApi.h>

#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include<string.h>
#include <clEventExtApi.h>
#include <clAlarmApi.h>
#include <clMedDebug.h>
#ifdef MORE_CODE_COVERAGE
#include <clCodeCovStub.h>
#endif

#define CL_CONTAINMENT_ATTR_VALID_ENTRY        1
#define CL_MED_NOTIFICATION_SUBS_ID	0xff4321

#include <clAlarmDefinitions.h>
#include <clLogApi.h>
#include <clMedLog.h>

#include <ipi/clAlarmIpi.h>

typedef struct funcPtr
{
    ClMedNotifyHandlerCallbackT fpMedTrapHdler;
}ClMedNotifyT;

ClMedNotifyT  *gNotifyHandler;
ClEventInitHandleT gMedEevtHandle;
ClEventChannelHandleT gCorEvtChannelHandle;
ClEventChannelHandleT gNotificationEvtChannelHandle;

/**
 *
 * Initialise the mediation component 
 *
 * Arguments:
 *  @param       ClMedHdlPtrT       *Output parameter. Handle to mediation
 *  @param       ClMedNotifyHandlerCallbackT  Function pointer to handle the notifications 
 *  @param       instXlator     Function pointer to handle the
 *                                     instance translations 
 *
 *  @param       fpInstCompare  Function pointer to compare two 
 *                                      instances of objects 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */


ClRcT  clMedInitialize(ClMedHdlPtrT             *medHdl, 
                       ClMedNotifyHandlerCallbackT        fpTrapHdler,
                       ClMedInstXlatorCallbackT        fpInstXlator,
                       ClCntKeyCompareCallbackT fpInstCompare)
{
    ClMedObjT        *pMm;   /* Pointer to Mediation object*/
    ClRcT          rc=CL_OK ;     /* Return code */
    ClEoExecutionObjT*  hEoObj; /* Eo Obj Pointer */
	ClEventCallbacksT  evtCallBack;
    ClNameT channelName;
    ClNameT notificationChannelName;
    ClVersionT emVersion = CL_EVENT_VERSION;
    ClMedNotifyT  *notifyHandler;
    ClUint32T     eventType = CL_BIT_H2N32(CL_ALARM_EVENT);


#ifdef DEBUG
    static ClUint8T medDbgInit = CL_FALSE;
    if(CL_FALSE== medDbgInit)
    {
        rc= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
            CL_FUNC_EXIT();
            return rc;
        }
        medDbgInit=CL_TRUE ;
    }
#endif


    CL_FUNC_ENTER();
    memset(&channelName, '\0', sizeof(channelName));

    if (medHdl == NULL) 
    {
        /* Invalid Argument. Return failure */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Null pinter passed");
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NULL_POINTER);
    }

    if ((pMm = (ClMedObjT *)clHeapAllocate(sizeof (ClMedObjT))) == NULL)
    {
        /* Things are real bad.   Return failure */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Malloc failed to allocate memory");
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 
    }
    notifyHandler = (ClMedNotifyT *)clHeapAllocate(sizeof(ClMedNotifyT) );
    if(NULL == notifyHandler)
    {
        /* Things are real bad.   Return failure */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Malloc failed to allocate memory");
        CL_FUNC_EXIT();
        return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 
    }
    gNotifyHandler = notifyHandler; /* This is required to free during finalization */
    memset(pMm, '\0', sizeof(ClMedObjT)); 

    /* Initialise idXlationTbl */
    if ( CL_OK != (rc = clCntLlistCreate(clMedAttrIdCompare,
                                         clMedDummyCallback,clMedDummyCallback,
                                         CL_CNT_UNIQUE_KEY,
                                         (ClCntHandleT *)&pMm->idXlationTbl)))
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Container Lib returned failure : RC=[%x]",rc);
        goto exitOnError;
    }

    /* Initialise the opCode translation table */ 
    if (CL_OK != (rc = clCntLlistCreate(clMedOpCodeCompare,
                                        clMedDummyCallback,clMedDummyCallback,
                                        CL_CNT_UNIQUE_KEY,
                                        (ClCntHandleT *)&pMm->opCodeXlationTbl)))
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Container Lib returned failure : RC=[%x]",rc);
        goto exitOnError;
    }

    /* Initialise the Error code translation table */ 
    if (CL_OK != (rc = clCntLlistCreate(clMedErrorIdCompare,
                                        clMedDummyCallback,clMedDummyCallback,
                                        CL_CNT_UNIQUE_KEY,
                                        (ClCntHandleT *)&pMm->errIdXlationTbl)) )
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Container Lib returned failure : RC=[%x]",rc);
        goto exitOnError;
    }
    /* Initialise the Watch attribute list */ 
    if (CL_OK != (rc = clCntLlistCreate(clMedWatchIdCompare,
                                        clMedAllWatchAttrDel,
                                        clMedAllWatchAttrDel,
                                        CL_CNT_UNIQUE_KEY,
                                        (ClCntHandleT *)&pMm->watchAttrList)))
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Container Lib returned failure : RC=[%x]",rc);
        goto exitOnError;
    }
    /* Assign the Notify handler */ 
    pMm->fpNotifyHdler = fpTrapHdler;

    /* Assign the Instance translator */ 
    pMm->fpInstXlator = fpInstXlator;

    /* Assign the Instance translator */ 
    pMm->fpInstCompare = fpInstCompare;

    if ( CL_OK != (rc = clEoMyEoObjectGet(&hEoObj)))
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "clEoMyEoObjectGet failed : RC=[%x]",rc);
        goto exitOnError;
    }

    evtCallBack.clEvtChannelOpenCallback = NULL;
    evtCallBack.clEvtEventDeliverCallback = clMedNotificationHandler;

    clLogTrace(CL_MED_AREA, CL_MED_CTX_INT, "Initalize Event client");
    
    /*TO DO change version to appropriate */	
    if(CL_OK!=( rc =  clEventInitialize( &gMedEevtHandle, &evtCallBack, & emVersion)))
    {
        /* Over my head. Just pass the error */
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "not able to initialize event library :  RC=[%x]",rc);
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("not able to initialize event library"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, CL_MED_LIB_NAME, CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED,"event");          
        goto exitOnError;	
    }



    clLogTrace(CL_MED_AREA, CL_MED_CTX_INT, "Open Event Channel [%s]", clCorEventName);
    channelName.length = strlen(clCorEventName);
    strncpy(channelName.value, clCorEventName, channelName.length);
    rc = clEventChannelOpen(gMedEevtHandle, &channelName, CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, -1, &gCorEvtChannelHandle);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "not able to open event channle : RC=[%x]", rc);
        goto exitOnError;	
    }

    notificationChannelName.length = strlen(ClAlarmEventName);
    strncpy(notificationChannelName.value, ClAlarmEventName, notificationChannelName.length);

    clLogTrace(CL_MED_AREA, CL_MED_CTX_INT, "Open Event Channel [%s]", ClAlarmEventName);
    rc = clEventChannelOpen(gMedEevtHandle, &notificationChannelName, CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL, -1, &gNotificationEvtChannelHandle);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "not able to open event channle , rc = 0x%x", rc);
        goto exitOnError;	
    }

    clLogTrace(CL_MED_AREA, CL_MED_CTX_INT, "Subscribe Alarm event");
    notifyHandler->fpMedTrapHdler = fpTrapHdler;
    rc = clEventExtSubscribe(gNotificationEvtChannelHandle, eventType, CL_MED_NOTIFICATION_SUBS_ID, (void *)notifyHandler);
    if(CL_OK != rc)
    {
        clLogError(CL_MED_AREA, CL_MED_CTX_INT, "Event subscription failed for Notification : RC=[%x]", rc);
        goto exitOnError;	
    }

    /* Create a semaphore to guard against multiple thread access */
    clOsalMutexCreate(&pMm->mmSem);
    *medHdl = (ClMedHdlPtrT)pMm; 

    clMedDebugRegister(*medHdl);

    CL_FUNC_EXIT();
    return CL_OK;

 exitOnError:
    {
        if (NULL != pMm)
        {
            if (0 != pMm->idXlationTbl) clCntDelete(pMm->idXlationTbl);
            if (0 != pMm->opCodeXlationTbl) clCntDelete(pMm->opCodeXlationTbl);
            if (0 != pMm->errIdXlationTbl) clCntDelete(pMm->errIdXlationTbl);
            if (0 != pMm->watchAttrList) clCntDelete(pMm->watchAttrList);
            clHeapFree(pMm);
        }
        CL_FUNC_EXIT();
        return rc;
    }
}

/**
 *
 * Destroy the mediation component 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT   Handle returned as part of init
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
*/
ClRcT  clMedFinalize(ClMedHdlPtrT myHdl)
{
    ClMedObjT   *pMm; /* Main object*/
    ClRcT     rc;   /* Return code */
   
    CL_FUNC_ENTER(); 
    /* Routine validations */ 
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_INVALID_HANDLE);         
         CL_FUNC_EXIT(); 
        return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 
    }

    clMedDebugDeregister();
     
    clOsalMutexLock(pMm->mmSem); /* Lock!  */
   
    clCntDelete(pMm->watchAttrList);

    /* Delete all Error Id ttranslations*/
    if (pMm->errIdXlationTbl != 0)
    {
        if (CL_OK != (rc = clMedAllErrIdXlationDel(pMm)))
        {
             clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
             clOsalMutexDelete(pMm->mmSem);
             CL_FUNC_EXIT();
             return rc;
        }
    }

    /* Delete all opCode translations*/
    if (pMm->opCodeXlationTbl != 0)
    {
        if (CL_OK != (rc = clMedAllOpCodeXlationDel(pMm)))
        {
             clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
             clOsalMutexDelete(pMm->mmSem);
             CL_FUNC_EXIT();
             return rc;
        }
    }

    /* Delete all Id translations*/
    if (pMm->idXlationTbl != 0)
    {
        if ( CL_OK != (rc = clMedAllIdXlationDel(pMm)))
        {
             clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
             clOsalMutexDelete(pMm->mmSem);
             CL_FUNC_EXIT();
             return rc;
        }
    }
    clHeapFree(gNotifyHandler); /* Freeing notify handler */
    clEventChannelClose(gNotificationEvtChannelHandle);
    clEventChannelClose(gCorEvtChannelHandle);
    clEventFinalize(gMedEevtHandle);
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    clOsalMutexDelete(pMm->mmSem);
    clHeapFree(pMm);
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Execute an operation 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT       Handle returned as part of init
 *  @param          ClMedVarBindT*       Pointer to an array of type ClMedOpT
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedOperationExecute(ClMedHdlPtrT       myHdl,
                            ClMedOpT        *pOpInfo)
{
    ClMedObjT              *pMm;
    ClMedAgntOpCodeT       tmpOpCode;
    ClMedOpCodeXlationT    *pOpXlation = NULL;
    ClUint32T              rc = CL_OK;

    CL_FUNC_ENTER(); 

    /* Routine validations */ 
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
        /* Things are real bad.   Return failure */
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Invalid Handle passed");
        return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 		 
    }

    if (pOpInfo == NULL)
    {
         /* Things are real bad.   Return failure */
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "Invalid OpInfo");
        return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 		 
    }

    memset(&tmpOpCode, '\0', sizeof(ClMedAgntOpCodeT));
    tmpOpCode.opCode  = pOpInfo->opCode; 

    clOsalMutexLock(pMm->mmSem);  

    /* Get the opCode from translation table */
    if (CL_OK != (rc = clCntDataForKeyGet (pMm->opCodeXlationTbl,
                                  (ClCntKeyHandleT) &tmpOpCode,
                                  (ClCntDataHandleT*)&pOpXlation)))
    {
        clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
        CL_FUNC_EXIT();
        clLog(CL_LOG_ERROR, CL_MED_AREA, CL_MED_CTX_CFG, "No OpCode found for operation %d",tmpOpCode.opCode);
        return CL_MED_SET_RC(CL_MED_ERR_NO_OPCODE);		 		 		
    }
    rc = clMedOpPerform(pMm, pOpXlation, pOpInfo);
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return rc;
}

ClRcT  clMedInstanceTranslationInsert(ClMedHdlPtrT        myHdl,
                                      ClMedAgentIdT     nativeId,
                                      ClMedTgtIdT      *sysId,
                                      ClUint32T      sysIdCount)
{
    ClMedObjT        *pMm;               /* Mediation Obj */
    ClMedIdXlationT     *pIdXlation = NULL; /* Pointer to id translation */
    ClRcT           rc = CL_OK;         /* Return value */
    ClCntNodeHandleT   nodeHdl = 0;
    ClMedTgtIdT     *pCorId =NULL;      /* COR Identifier */
    ClCorAttrPathPtrT pAttrPath = NULL;
    ClCorAttrIdT     corAttrId = 0;
    ClUint32T i = 0;

    CL_FUNC_ENTER(); 

    clLogInfo(CL_MED_AREA, CL_MED_CTX_INT, "Inserting NBI-id --> SBI-id instance mapping");

    /* Routine validations*/
    if ((( pMm = (ClMedObjT *)myHdl) == NULL) || 
        (sysId == NULL) ||
        (nativeId.len == 0))
    {
        /* Far-fetched arguments. Cry foul!! */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Parameters"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
        return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    if( (rc = clCntNodeFind(pMm->idXlationTbl, (ClCntKeyHandleT)&nativeId,  &nodeHdl) ) != CL_OK)
    {
        clLogError("MED", "INSERT", "Mediation identifier get failed for id [%.*s] with rc [%#x]",
                   nativeId.len, (ClCharT*)nativeId.id, rc);
        return rc;
    }

    rc = clCntNodeUserDataGet(pMm->idXlationTbl, nodeHdl,  (ClCntDataHandleT*)&pIdXlation);
    if(rc != CL_OK)
    {
        clLogError("MED", "INSERT", "Mediation identifier data get failed for id [%.*s] with rc [%#x]",
                   nativeId.len, (ClCharT*)nativeId.id, rc);
        return rc;
    }
    
    pCorId = pIdXlation->tgtId;

    if ((pCorId->type == CL_MED_XLN_TYPE_COR) &&
        (pMm->fpInstCompare!= NULL))
    {

        if(pIdXlation->tgtId->info.corId.attrId[0] != CL_MED_ATTR_VALUE_END &&
           pIdXlation->tgtId->info.corId.attrId[1] != CL_MED_ATTR_VALUE_END)
        {
            ClInt32T absoluteContainedPath = 1;

            rc = clCorAttrPathAlloc(&pAttrPath);
            if (rc != CL_OK)
            {
                clLogError("MED", "INSERT", "Failed to allocate attribute path. rc [0x%x]", rc);
                return rc;
            }
            
            while(pIdXlation->tgtId->info.corId.attrId[i+1] != CL_MED_ATTR_VALUE_END)
            {
                clCorAttrPathAppend(pAttrPath, sysId->info.corId.attrId[i], 
                                    sysId->info.corId.attrId[i+1]);
                if(sysId->info.corId.attrId[i+1] == CL_COR_INDEX_WILD_CARD)
                    absoluteContainedPath = 0; 
                i += 2;
            }
            /*If the user provides absolute value of attribute path, add it directly
              to the translation table.*/
            if(absoluteContainedPath)
            {
                clMedInstXlnAdd(pIdXlation,
                                sysId->info.corId.moId,
                                pAttrPath,
                                0);
                clCorAttrPathFree(pAttrPath);
                return CL_OK;
            }
        }

        corAttrId = pIdXlation->tgtId->info.corId.attrId[i];
        rc = clMedEventSubscribe(pIdXlation, pAttrPath, corAttrId);
        if(CL_OK != rc)
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not Subscribe for MOID changes"));
        }

        if (CL_OK != (rc = clMedPreCreatedObjNtfy(pMm, pIdXlation)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to read pre-created objects"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_PRE_CREATED_OBJ_NOTIFY_FAILED, rc);             
        }
    }

    if(pAttrPath)
        clCorAttrPathFree(pAttrPath);

    return rc;
}

/**
 *
 * Add an entry to the identifier translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT       Handle returned as part of init
 *  @param          ClMedAgentIdT    Identifier in agent language
 *  @param          ClMedTgtIdT     Array of  COR Identifiers
 *  @param          ClUint32T     Number of elements 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedIdentifierTranslationInsert(ClMedHdlPtrT        myHdl,
                        ClMedAgentIdT     nativeId,
                        ClMedTgtIdT      *sysId,
                        ClUint32T      sysIdCount)
{
    ClMedObjT        *pMm;               /* Mediation Obj */
    ClMedIdXlationT     *pIdXlation = NULL; /* Pointer to id translation */
    ClRcT           rc = CL_OK;         /* Return value */
    ClCntNodeHandleT   nodeHdl = 0;

    CL_FUNC_ENTER(); 

    clLogInfo(CL_MED_AREA, CL_MED_CTX_INT, "Inserting NBI-id --> SBI-id mapping");

    /* Routine validations*/
    if ((( pMm = (ClMedObjT *)myHdl) == NULL) || 
         (sysId == NULL) ||
         (nativeId.len == 0))
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Parameters"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         rc = CL_ERR_INVALID_PARAMETER;
         goto exitOnError;
    }
    
    if (CL_OK == (rc = clCntNodeFind(pMm->idXlationTbl, (ClCntKeyHandleT)&nativeId,  &nodeHdl)))
    {
        rc = clCntNodeUserDataGet(pMm->idXlationTbl, nodeHdl,  (ClCntDataHandleT*)&pIdXlation);
        if(rc != CL_OK)
        {
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("not able to get idXlation from container , rc = 0x%x", rc));        
             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);              
             return rc;
    
        }

        rc = clCntNodeAdd(pIdXlation->moIdTbl, 
                          (ClCntKeyHandleT)sysId->info.corId.moId,(ClCntDataHandleT) sysId->info.corId.moId, NULL);	
        /*
         * Ignore return code
         */
        return CL_OK;
    }

    /* Allocate memory for the translation */
    if ((pIdXlation = (ClMedIdXlationT*)clHeapCalloc(1, sizeof(ClMedIdXlationT)))== NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         rc = CL_ERR_NO_MEMORY;
         goto exitOnError;
    }

    pIdXlation->pMed = pMm;
    
    /* Assign the passed in parameters */
    if ((pIdXlation->nativeId.id = (ClUint8T *)clHeapCalloc(1, nativeId.len)) == NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         rc = CL_ERR_NO_MEMORY;
         goto exitOnError;
    }
    /* Copy the native ID*/
    memcpy(pIdXlation->nativeId.id, nativeId.id, nativeId.len); 
    pIdXlation->nativeId.len  = nativeId.len;
    pIdXlation->tgtIdCount = sysIdCount;
    pIdXlation->tgtId = (ClMedTgtIdT*)clHeapCalloc(sysIdCount, sizeof(ClMedTgtIdT));
    if (pIdXlation->tgtId == NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         rc = CL_ERR_NO_MEMORY;
         goto exitOnError;
    }
    /* Copy all the target Ids */
    memcpy(pIdXlation->tgtId, sysId, (sysIdCount * sizeof(ClMedTgtIdT))); 
    /* Create the list for Instance translation */
    if (pMm->fpInstCompare != NULL)
    {
       if (CL_OK != (rc = clCntLlistCreate(pMm->fpInstCompare,
                                               medInstanceXlnDel,
                                               medInstanceXlnDel,
	                                           CL_CNT_UNIQUE_KEY,
                                               &pIdXlation->instXlationTbl)))
       {
            /* Over my head. Just pass the error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create Instance Translation Table"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
            goto exitOnError;
       }
    }
    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    
    if (CL_OK != (rc = clCntLlistCreate(clMedMoIdCompare,
                                        clMedMoIdDelete,
                                        clMedMoIdDelete,
                                        CL_CNT_UNIQUE_KEY,
                                        &pIdXlation->moIdTbl)))
    {
        clOsalMutexUnlock(pMm->mmSem);       
        /* Over my head. Just pass the error */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create Instance Translation Table"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);        
        goto exitOnError;
    }
    
    rc = clCntNodeAdd(pIdXlation->moIdTbl, (ClCntKeyHandleT)sysId->info.corId.moId,(ClCntDataHandleT) sysId->info.corId.moId, NULL);

    /* Add the translation to the table */
    clLogTrace("MED", "INSERT", "Adding id translation table entry for [%.*s]",
               pIdXlation->nativeId.len, pIdXlation->nativeId.id);
    if (CL_OK != (rc = clCntNodeAdd(pMm->idXlationTbl,
                              (ClCntKeyHandleT) &pIdXlation->nativeId,
                              (ClCntDataHandleT) pIdXlation,
                              NULL)))
    {
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         /* Over my head. Just pass the error */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);         
         goto exitOnError;
    }

    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;

exitOnError:  /* Error exit block */
    {
        if (pIdXlation != NULL)
        {
            if (pIdXlation->nativeId.id != NULL) clHeapFree(pIdXlation->nativeId.id);
            if (pIdXlation->tgtId != NULL) clHeapFree(pIdXlation->tgtId);
            clHeapFree(pIdXlation);
        }
        CL_FUNC_EXIT(); 
        return CL_MED_SET_RC(rc);		 		 				
    }
}

/**
 *
 * Subscribe to the cor events
 *
 * Arguments:
 *  @param          ClMedIdXlationT   Translator of native id to target id.
 *  @param          ClCorAttrPathPtrT Attribute path for containment attributes
 *  @param          ClCorAttrIdT      Attribute id representation in cor.
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Sachin Mishra
 * @version 1.0
 */
ClRcT clMedEventSubscribe(ClMedIdXlationT *pIdXlation, 
                          ClCorAttrPathPtrT pAttrPath, 
                          ClCorAttrIdT     corAttrId)
{
    ClRcT           rc = CL_OK;         /* Return value */
    ClEventSubscriptionIdT subsId = 0;
    ClCorAttrListT   attrList;
    ClEoExecutionObjT*   hEoObj;
    static ClEventSubscriptionIdT subscriptionId = 1;
    ClCorClassTypeT classId = 0;
    ClCharT className[CL_MAX_NAME_LENGTH] = {0};
    ClUint32T classNameSize = 0;
    ClCorAttrListT* pAttrList = NULL;
    ClCorAttrDefT* pAttrDef = NULL;
    ClUint32T attrCount = 0;
    ClUint32T i = 0;
   
    CL_FUNC_ENTER(); 

    if(NULL == pIdXlation)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Paramater passed as NULL."));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT, rc);
         CL_FUNC_EXIT();
         return CL_ERR_INVALID_PARAMETER;
    }
    if ( (rc = clEoMyEoObjectGet(&hEoObj)) != CL_OK)
    {
         /* Over my head. Just pass the error */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clEoMyEoObjectGet failed "));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_MY_EO_OBJ_GET_FAILED, rc);             
         CL_FUNC_EXIT();
         return rc;
    }
    else
    {
        if(pIdXlation->tgtId->info.corId.attrId[0] == CL_MED_ATTR_VALUE_END || 
                pIdXlation->tgtId->info.corId.attrId[1] == CL_MED_ATTR_VALUE_END)
        {
            ClCorMOIdT provMoId = {{{0}}};

            subsId = subscriptionId++;
            if (CL_OK != (clCorEventSubscribe(gCorEvtChannelHandle,
                            pIdXlation->tgtId->info.corId.moId,
                            NULL,
                            NULL,
                            (ClCorOpsT)(CL_COR_OP_CREATE | CL_COR_OP_DELETE),
                            (void *)pIdXlation,
                            subsId)))
            {
                CL_FUNC_EXIT();
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not Subscribe for MOID changes"));
                clLogError("SNP", "INT", "Could not Subscribe for MOID changes");
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_EVT_SUBSCRIBE_FAILED, rc);
            }
            else
            {
                clLogDebug("SNP", "INT", "Subscribed event for MOID ");
                clCorMoIdShow(pIdXlation->tgtId->info.corId.moId);
            }

            /* Index attributes are part of prov Mso in COR.
             * So the PM (or any other services) attributes also 
             * needs to be updated when index attributes (which is in prov Mso) changes.
             */
            memcpy(&provMoId, pIdXlation->tgtId->info.corId.moId, sizeof(ClCorMOIdT));
            clCorMoIdServiceSet(&provMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);

            rc = clCorMoIdToClassGet(&provMoId,
                    CL_COR_MSO_CLASS_GET, &classId);
            if (rc != CL_OK)
            {
                clLogError("SNP", "INT", "Failed to get classId from MoId. rc [0x%x]", rc);
                return rc;
            }

            rc = clCorClassNameGet(classId, className, &classNameSize);
            if (rc != CL_OK)
            {
                clLogError("SNP", "INT", "Failed to get className from classId. rc [0x%x]", rc);
                return rc;
            }

            rc = clCorClassAttrListGet(className, CL_COR_ATTR_INITIALIZED, &attrCount, &pAttrDef);
            if (rc != CL_OK)
            {
                clLogError("SNP", "INT", "Failed to get initialized attribute list from classId. rc [0x%x]", rc);
                return rc;
            }

            pAttrList = (ClCorAttrListT *) clHeapAllocate(sizeof(ClCorAttrListT) + 
                                                attrCount * sizeof(ClInt32T));
            if (!pAttrList)
            {
                clLogError("SNP", "INT", "Failed to allocate memory.");
                clHeapFree(pAttrDef);
                return CL_MED_SET_RC(CL_ERR_NO_MEMORY); 
            }

            pAttrList->attrCnt = attrCount;

            clLogDebug("SNP", "INT", "No. of Index attributes : [%u] of cor class [%s]", 
                    attrCount, className);

            for (i=0; i<attrCount; i++)
            {
                clLogDebug("SNP", "INT", "Index attribute [%d] subscribed for change notification of cor class : [%s]",
                        (pAttrDef + i)->attrId, className);
                pAttrList->attr[i] = (pAttrDef + i)->attrId;
            }

            /* Subscribe for index attribute change event */
            rc = clCorEventSubscribe(gCorEvtChannelHandle,
                    &provMoId,
                    NULL,
                    pAttrList,
                    CL_COR_OP_SET,
                    (void *) pIdXlation,
                    subscriptionId++);
            if (rc != CL_OK)
            {
                clLogError("SNMP", "INT", "Failed to subscribe for index attribute "
                        "change notification. rc [0x%x]", rc);
            }

            clHeapFree(pAttrDef);
            clHeapFree(pAttrList);
        }
        else       /*Subscribe for change in the state attribute of contained object*/
        {
            attrList.attrCnt = 1;
            attrList.attr[0] = CL_CONTAINMENT_ATTR_VALID_ENTRY;
            subsId = subscriptionId++;
            if (CL_OK != (clCorEventSubscribe(gCorEvtChannelHandle,
                                pIdXlation->tgtId->info.corId.moId,
                                pAttrPath,
                                &attrList,
                                CL_COR_OP_SET,
                                (void *)pIdXlation,
                                subsId)))
            {
                CL_FUNC_EXIT();
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not Subscribe for MOID changes"));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_EVT_SUBSCRIBE_FAILED, rc); 
                clCorMoIdShow(pIdXlation->tgtId->info.corId.moId);
                clCorAttrPathShow(pAttrPath);
            }
            if(corAttrId == CL_ALARM_ACTIVE)
            {
                attrList.attr[0] = corAttrId;
                subsId = subscriptionId++;
                if (CL_OK != (clCorEventSubscribe(gCorEvtChannelHandle,
                                   pIdXlation->tgtId->info.corId.moId,
                                   pAttrPath,
                                   &attrList,
                                   CL_COR_OP_SET,
                                   (void *)pIdXlation,
                                   subsId)))
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_EVT_SUBSCRIBE_FAILED, rc);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not Subscribe for Alarm active changes"));
                }
            }
            subsId = subscriptionId++;
            if (CL_OK != (clCorEventSubscribe(gCorEvtChannelHandle,
                               pIdXlation->tgtId->info.corId.moId,
                               NULL,
                               NULL,
                               CL_COR_OP_DELETE,
                               (void *)pIdXlation,
                               subsId)))
            {
                 clCorMoIdShow(pIdXlation->tgtId->info.corId.moId);
                 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_EVT_SUBSCRIBE_FAILED, rc);
                 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not Subscribe for Alarm Delete "));
            }
        }
    }
    return rc;
}

/**
 *
 * Delete an entry from the identifier translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT       Handle returned as part of init
 *  @param          ClMedAgentIdT    Identifier in agent language
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedIdentifierTranslationDelete(ClMedHdlPtrT       myHdl,
                        ClMedAgentIdT    mgmtId)
{
    ClMedObjT          *pMm;        /* Mediation Obj */
    ClMedIdXlationT       *pIdXlation = NULL;  /* Pointer to the Translation*/
    ClRcT            rc;          /* Return code */

    CL_FUNC_ENTER(); 

    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
	return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 
		 
    }
    
    /* Get the id translation */
    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    if (CL_OK != (rc = clCntDataForKeyGet(pMm->idXlationTbl,
                              (ClCntKeyHandleT) &mgmtId,
                              (ClCntDataHandleT *)&pIdXlation)))
    {
         /* Over my head. Just pass the error */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
         CL_FUNC_EXIT();
         return rc;
    }
    
    if (pIdXlation == NULL)
    {
         /* No element!! Must be wrong parameters*/
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No error translation found"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_IS_NOT_PRESENT, rc);         
	return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 
    }

    /* Destroy the container(instance translation table) */
    if (pIdXlation->instXlationTbl != 0)
    {
        /* Delete All instance translations */
        if (CL_OK != (rc = clCntAllNodesDelete(pIdXlation->instXlationTbl)))
        {
            /* Some thing wrong!! */
            clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("All instance translations Delete Failed!!"));
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED, rc);            
            return rc;
        }
        if (CL_OK != (rc = clCntDelete(pIdXlation->instXlationTbl)))
        {
            /* Some thing wrong!! */
            clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("All instance translations Delete Failed!!"));
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED, rc);            
            return rc;
        }
    }

    if (CL_OK !=  (rc = clCntAllNodesForKeyDelete(pMm->idXlationTbl,
                                                  (ClCntKeyHandleT) &mgmtId)))
    {
        /* Some thing wrong!! */
        clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Node Delete Failed!!"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED, rc);        
        return rc;
    }
    /* Free this translation */
    clMedIdXlationFree(pIdXlation);
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Add an entry to the operation translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT          Handle returned as part of init
 *  @param          ClMedAgntOpCodeT   Operation identifier in agent terminology 
 *  @param          ClMedTgtOpCodeT    *Operation identifier in COR terminology 
 *  @param          ClUint32T        Operation count 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedOperationCodeTranslationAdd(ClMedHdlPtrT           myHdl,
                            ClMedAgntOpCodeT    nativeOpCode,
                            ClMedTgtOpCodeT     *tgtOpCode,
                            ClUint32T         opCount)

{
    ClMedObjT        *pMm;          /* Mediation Obj */
    ClMedOpCodeXlationT *pOpCode;      /* Ptr to opCode translation to be added */
    ClUint32T      tmpSize =0;    /* To avoid lengthy statements*/
    ClRcT          rc;            /* Return value */

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);        
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 
    }

    if ((tgtOpCode == NULL) || (opCount == 0) )
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid parameters"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_INVALID_PARAMETER);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 
    }

    /* Allocate memory for the translation */
    if ((pOpCode = (ClMedOpCodeXlationT*) clHeapAllocate(sizeof(ClMedOpCodeXlationT)))== NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 
    }
    memset(pOpCode, '\0', sizeof(ClMedOpCodeXlationT ) ); 

    /* Construct the opCode translation */
    pOpCode->nativeOpCode = nativeOpCode;
    pOpCode->tgtOpCount  = opCount;
    tmpSize = opCount * sizeof(ClMedTgtOpCodeT);
    if ((pOpCode->tgtOpCode = (ClMedTgtOpCodeT *)clHeapAllocate(tmpSize))== NULL)
    {
         /* Things are real bad.   Return failure */
         clHeapFree(pOpCode);
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 		 
    }
    memset(pOpCode->tgtOpCode, '\0', tmpSize);
    memcpy(pOpCode->tgtOpCode, tgtOpCode, tmpSize);

    /* Add this entry to the table */
    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    if (CL_OK != (rc = clCntNodeAdd(pMm->opCodeXlationTbl,
                              (ClCntKeyHandleT) &pOpCode->nativeOpCode,
                              (ClCntDataHandleT) pOpCode,
                              NULL)))
    {
         /* Over my head. Just pass the error */
         clHeapFree(pOpCode->tgtOpCode);
         clHeapFree(pOpCode);
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);         
         CL_FUNC_EXIT();
         return rc;
    }
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Delete an entry to the operation translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClMedAgntOpCodeT   Operation identifier in agent terminology 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedOperationCodeTranslationDelete(ClMedHdlPtrT         myHdl,
                            ClMedAgntOpCodeT  agntOpCode)
{
    ClMedObjT          *pMm;        /* Mediation Obj */
    ClMedOpCodeXlationT   *pOpCodeXlation = NULL;  /* Pointer to the Translation*/
    ClCntNodeHandleT   nodeHdl = 0; /* Node hnadle for opcode translation */
    ClRcT            rc;          /* Return code */

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 
    }
    
    /* Get the id translation */
    clOsalMutexLock(pMm->mmSem); /* Lock!  */

    if (CL_OK != (rc = clCntNodeFind(pMm->opCodeXlationTbl,
                              (ClCntKeyHandleT) &agntOpCode,
                              &nodeHdl)))
    {
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_FIND_FAILED, rc);         
         CL_FUNC_EXIT();
         return rc;
    }
    if (CL_OK != (rc = clCntDataForKeyGet(pMm->opCodeXlationTbl,
                              (ClCntKeyHandleT) &agntOpCode,
                              (ClCntDataHandleT *)&pOpCodeXlation)))
    {
         /* Over my head. Just pass the error */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
         CL_FUNC_EXIT();
         return rc;
    }

    if (CL_OK != (rc = clCntNodeDelete(pMm->opCodeXlationTbl, nodeHdl)))
    {
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED, rc);         
         CL_FUNC_EXIT();
         return rc;
    }
    
    if (pOpCodeXlation == NULL)
    {
         /* No element!! Must be wrong parameters*/
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No error translation found"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_OP_CODE_IS_NOT_PRESENT, rc);         
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 
    }
    
    /* Free this translation */
    clMedOpCodeXlationFree(pOpCodeXlation);
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Add an entry to the Error Id translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClUint32T   System error type 
 *  @param          ClUint32T   System error ID 
 *  @param          ClUint32T   Agent Error Id 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedErrorIdentifierTranslationInsert(ClMedHdlPtrT    myHdl,
                             ClUint32T  sysErrorType,
                             ClUint32T  sysErrorId,
                             ClUint32T  agntErrorId)
{
    ClMedObjT        *pMm;         /* Mediation Obj */
    ClMedErrIdXlationT  *pErrXlation; /* Pointer to translation to be added */
    ClRcT          rc;           /* Return value */

    CL_FUNC_ENTER(); 

    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 
    }

    /* Allocate memory for the translation */
    if ((pErrXlation = (ClMedErrIdXlationT*)clHeapAllocate(sizeof(ClMedErrIdXlationT)))== NULL)
    {
         /* Things are real bad.   Return failure */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 		 		 
    }
    memset(pErrXlation, '\0', sizeof( ClMedErrIdXlationT ) ); 
    /* Assign the passed in parameters */
    pErrXlation->tgtError.type = sysErrorType;
    pErrXlation->tgtError.id   = sysErrorId;
    pErrXlation->nativeErrorId   = agntErrorId;
 
    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    /* Add the translation to the table */ 
    if (CL_OK != (rc = clCntNodeAdd(pMm->errIdXlationTbl,
                               (ClCntKeyHandleT) &pErrXlation->tgtError,
                                (ClCntDataHandleT) pErrXlation,
                                NULL)))
    {
         /* Over my head. Just pass the error */
         clHeapFree(pErrXlation);
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure. rc = 0x%x",rc));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);         
         return rc;
    }
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK; /* Finally :)*/
}

/**
 *
 * Delete an entry from the Error Id translation table 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClUint32T   System error type 
 *  @param          ClUint32T   System error ID 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedErrorIdentifierTranslationDelete(ClMedHdlPtrT    myHdl,
                             ClUint32T  sysErrorType,
                             ClUint32T  sysErrorId)
{
    ClMedObjT          *pMm;        /* Mediation Obj */
    ClMedTgtErrInfoT      errorKey;    /* Key for the search */
    ClMedErrIdXlationT    *pErrXlation = NULL;  /* Pointer to the Translation*/
    ClCntNodeHandleT   nodeHdl=0;   /* Node handle from the container */
    ClRcT            rc;          /* Return code */

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 
    }
    
    /* Get the error translation */
    errorKey.type   =  sysErrorType;
    errorKey.id     =  sysErrorId;

    clOsalMutexLock(pMm->mmSem); /* Lock!  */

    if (CL_OK != (rc = clCntNodeFind(pMm->errIdXlationTbl, 
                              (ClCntKeyHandleT) &errorKey,
                              &nodeHdl)))
    {
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_NODE_FIND_FAILED, rc);         
         return rc;
    }

    if (CL_OK != (rc = clCntDataForKeyGet(pMm->errIdXlationTbl,
                              (ClCntKeyHandleT) &errorKey,
                              (ClCntDataHandleT *)&pErrXlation)))
    {
         /* Over my head. Just pass the error */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
         return rc;
    }

    if (CL_OK != (rc = clCntNodeDelete(pMm->errIdXlationTbl, nodeHdl)))
    {
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_DELETE_FAILED, rc);         
         return rc;
    }
    
    if (pErrXlation == NULL)
    {
         /* No element!! Must be wrong parameters*/
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No error translation found"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ERROR_CODE_IS_NOT_PRESENT, rc);         
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 
    }
    
    /* Free this translation */
    clHeapFree(pErrXlation);
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Subscribe for change notifications 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClMedAgentIdT  Identifier in agent terminology 
 *  @param          ClUint32T   Session Identifier (optional)
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @note : This API assumes that the Identifier translation is already added
 *          for the identifier being subscribed.
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedNotificationSubscribe(ClMedHdlPtrT      myHdl,
                           ClMedAgentIdT   nativeId,
                           ClUint32T    sessionId)
{
    ClMedObjT          *pMm;        /* Mediation Obj */
    ClMedIdXlationT       *pIdXlation; /* Id translatoin */
    ClMedWatchAttrT       *pWatchAttr = NULL; /* Attribute to be watched */
    ClMedTgtIdT        *pTmpTgtId;  /* Temporary pointer */
    ClUint32T        count = 0;       /* Temp count */
    ClRcT            rc;          /* Return code */

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 
    }

    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    if (CL_OK != (rc = clCntDataForKeyGet(pMm->idXlationTbl,
                              (ClCntKeyHandleT) &nativeId,
                              (ClCntDataHandleT *)&pIdXlation)))
    {
         /* Over my head. Just pass the error */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
         return rc;
    }

    if (pIdXlation == NULL)
    {
         /* No element!! Must be wrong parameters*/
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No error translation found"));
         clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_IS_NOT_PRESENT, rc);         
         return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 
    }

    /* Check if this attribute is already being watched
       (probably for another sesssion) */

    if (CL_OK != (rc = clCntDataForKeyGet(pMm->watchAttrList,
                              (ClCntKeyHandleT) &pIdXlation->tgtId,
                              (ClCntDataHandleT *)&pWatchAttr)))
    {
         if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
         {
              /* Things are real bad.   Return failure */
              clOsalMutexUnlock(pMm->mmSem);       
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No more memory"));
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);              
              CL_FUNC_EXIT(); 
              return CL_MED_SET_RC(CL_ERR_NO_MEMORY);		 		 				 		 		 		 		 			  
         }
    }
    
    if (pWatchAttr != NULL)
    {
         /* Already existing attribute. Just add this session  */
         if (CL_OK != (rc = clCntNodeAdd(pWatchAttr->sessionList,
                                   (ClCntKeyHandleT)(ClWordT) sessionId,
                                   (ClCntDataHandleT)(ClWordT)sessionId,
                                   NULL)))
         {
              /* Over my head. Just pass the error */
              clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
              CL_FUNC_EXIT();
              CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);              
              return rc;
         } /* End of container Add */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         return CL_OK;
    }
    
    /* A new node!*/
    pTmpTgtId = pIdXlation->tgtId;
    for (count =0; count <  pIdXlation->tgtIdCount; count++)
    {    
         if (CL_OK != (rc =  clMedWatchAttrCreate(pMm, 
                                                  &nativeId,
                                                  pTmpTgtId, 
                                                  sessionId )))
         {
              clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
              CL_FUNC_EXIT();
              return rc;
         } 
         pTmpTgtId += sizeof(ClMedTgtIdT);
    } /* End of for loop  */
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Unsubscribe for change notifications 
 *
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClMedAgentIdT  Identifier in agent terminology 
 *  @param          ClUint32T   Session Identifier 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedNotificationUnsubscribe(ClMedHdlPtrT      myHdl,
                             ClMedAgentIdT   nativeId,
                             ClUint32T    sessionId)

{
    ClMedObjT          *pMm;        /* Mediation Obj */
    ClMedIdXlationT       *pIdXlation = NULL; /* Id translatoin */
    ClMedTgtIdT        *pTmpTgtId;  /* Temporary pointer */
    ClUint32T        count;       /* Temp count */
    ClRcT            rc;          /* Return code */

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 

	return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 			  		 
    }

    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    if (CL_OK != (rc = clCntDataForKeyGet(pMm->idXlationTbl,
                              (ClCntKeyHandleT) &nativeId,
                              (ClCntDataHandleT *)&pIdXlation)))
    {
         /* Over my head. Just pass the error */
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Lib returned failure"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);         
         return rc;
    }

    if (pIdXlation == NULL)
    {
         /* No element!! Must be wrong parameters*/
         clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
         CL_FUNC_EXIT();
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Id translation found"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_MED_LOG_1_ID_XLN_IS_NOT_PRESENT,  rc);         
	return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 			  		 		 
    }
   
    pTmpTgtId = pIdXlation->tgtId;
    for (count = 0; count >= pIdXlation->tgtIdCount; count++)
    {    
         if (CL_OK != (rc = clMedWatchStop(pMm, pTmpTgtId, sessionId)))
         {
              clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
              CL_FUNC_EXIT();
              return rc;
         } 
         pTmpTgtId += sizeof(ClMedTgtIdT);
    } /* End of for loop  */
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 *
 * Unsubscribe for all change notifications 
 * Arguments:
 *  @param          ClMedHdlPtrT     Handle returned as part of init
 *  @param          ClUint32T   Session Identifier 
 *
 *  @returns
 *      CL_OK if every thing is successful
 *      ERROR for any other errors
 * @author Hari Krishna G
 * @version 0.0
 */
ClRcT  clMedAllNotificationUnsubscribe(ClMedHdlPtrT      myHdl,
                                ClUint32T    sessionId)
{  
    ClMedObjT                *pMm;        /* Mediation Obj */
    ClCntNodeHandleT         watchHdl;
    ClCntNodeHandleT         tmpNodeHdl;
    ClMedWatchAttrT             *pWatchAttr;
    ClRcT                  rc = CL_OK;

    CL_FUNC_ENTER(); 
    /* Routine validations*/
    if (( pMm = (ClMedObjT *)myHdl) == NULL)
    {
         /* Far-fetched arguments. Cry foul!! */
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Handle"));
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_0_NULL_ARGUMENT);         
         CL_FUNC_EXIT(); 
	return CL_MED_SET_RC(CL_ERR_INVALID_PARAMETER);		 		 				 		 		 		 		 			  		 
    }

    clOsalMutexLock(pMm->mmSem); /* Lock!  */
    if (CL_OK != clCntFirstNodeGet (pMm->watchAttrList, &watchHdl))
    {
         goto exitOnError;
    }
    while (watchHdl != 0)
    {
         if (CL_OK != (rc = clCntNodeUserDataGet(pMm->watchAttrList,
                                            watchHdl,
                            (ClCntDataHandleT *) &pWatchAttr)))
         {
              goto exitOnError;
         }

         if (CL_OK !=  (rc = clMedWatchSessionDel(pMm, pWatchAttr, sessionId)))
         {
              goto exitOnError;
         } 
         if (CL_OK != (rc = clCntNextNodeGet(pMm->watchAttrList, 
                                        watchHdl, &tmpNodeHdl)))
         {
            if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
                goto exitOnError;
            else 
                break;
         }
         if (CL_OK != (rc = clCntNodeDelete(pMm->watchAttrList, watchHdl)))
         {
              goto exitOnError;
         }
         watchHdl = tmpNodeHdl;
    }
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT(); 
    return CL_OK;

exitOnError:
    clOsalMutexUnlock(pMm->mmSem);       /* Unlock! Good citizen */
    CL_FUNC_EXIT(); 
    return rc;
}

void clMedNotificationHandler( ClEventSubscriptionIdT subscriptionId, ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClRcT      rc = CL_OK;
    ClUint32T  eventType = 0;
    ClEventPriorityT priority = 0;
    ClTimeT  retentionTime = 0;
    ClNameT  publisherName = {0};
    ClTimeT  publishTime = 0;
    ClEventIdT eventId = 0;
    ClAlarmHandleInfoT* pAlarmHandleInfo = NULL;
    ClUint8T * pData = NULL;
    ClCorOpsT  operation;
    ClCorAttrPathPtrT pAttrPath = NULL;
    ClCorTxnIdT       corTxnId = 0;
    ClCorTxnJobIdT    jobId = 0;
    /* ClMedAttrPathInfoT attrPathInfo = {0}; */
    void *pCookie;
    ClMedNotifyT  *notifyHandler = NULL;
    
    /*Generate trap only if it running on active SysController*/
    clLogDebug("SNP", "EVT", "Received event");
    if((rc = clEventCookieGet(eventHandle, (void * *) &pCookie)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Could not get the cookie  from Event")); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_MED_LIB_NAME, CL_LOG_MESSAGE_1_EVENT_COOKIE_GET_FAILED,  rc); 
        CL_FUNC_EXIT();
    }

    if(subscriptionId == CL_MED_NOTIFICATION_SUBS_ID)
    {
        if( (rc = clEventExtAttributesGet( eventHandle,
                &eventType, &priority,
                &retentionTime, &publisherName,
                &publishTime, &eventId ) ) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rCould not get attribute information from event rc:0x%x",rc)); 
            clEventFree(eventHandle);
            CL_FUNC_EXIT();
            return;
        }

        if(CL_BIT_N2H32(eventType) == CL_ALARM_EVENT)
        {
        
            pData = (ClUint8T*)clHeapAllocate(eventDataSize);
            if(NULL == pData)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nMemory Allocation failed with rc:0x%x",rc)); 
                clEventFree(eventHandle);
                CL_FUNC_EXIT();
                return;
            }   
            rc = clEventDataGet (eventHandle, (void*)pData,  &eventDataSize);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\r Could not get event data information from Event rc:0x%x",rc)); 
                clEventFree(eventHandle);
                clHeapFree(pData);
                CL_FUNC_EXIT();
                return;
            }

            pAlarmHandleInfo = (ClAlarmHandleInfoT*)clHeapAllocate(sizeof(*pAlarmHandleInfo) + eventDataSize);
            if(NULL == pAlarmHandleInfo)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nMemory Allocation failed with rc:0x%x",rc)); 
                clEventFree(eventHandle);
                clHeapFree(pData);
                CL_FUNC_EXIT();
                return;
            }   

            rc = clAlarmEventDataGet(pData, eventDataSize, pAlarmHandleInfo);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nclAlarmEventDataGet get failed with rc:0x%x",rc)); 
                clEventFree(eventHandle);
                CL_FUNC_EXIT();
                return;
            }
            
            CL_MED_DEBUG_ALARM_DATA_PRINT(pAlarmHandleInfo);
            clHeapFree(pData);

            notifyHandler = (ClMedNotifyT *)pCookie;
            if(NULL == notifyHandler || NULL == notifyHandler->fpMedTrapHdler)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n\rNotification handler is not provided")); 
                clEventFree(eventHandle);
                clHeapFree(pAlarmHandleInfo);
                CL_FUNC_EXIT();
                return;
            }
            else
            {
                notifyHandler->fpMedTrapHdler(&pAlarmHandleInfo->alarmInfo.moId, pAlarmHandleInfo->alarmInfo.probCause, 
                        pAlarmHandleInfo->alarmInfo.specificProblem, (ClCharT *)pAlarmHandleInfo, eventDataSize);
            }
            clHeapFree(pAlarmHandleInfo);
        }
    }
    else
    {
        clCorEventHandleToCorTxnIdGet(eventHandle, eventDataSize, &corTxnId);
        /* TODO - COR-TXN */
        rc = clCorTxnFirstJobGet(corTxnId, &jobId);
        do{
            clCorTxnJobOperationGet(corTxnId, jobId, &operation);
            switch(operation)
            {
                case CL_COR_OP_CREATE:
                case CL_COR_OP_DELETE:
                    clLog(CL_LOG_DEBUG, CL_MED_AREA, CL_MED_CTX_CFG, "Create=[%d]/Delete=[%d] MO: Current Operation[%d] ",\
                          CL_COR_OP_CREATE, CL_COR_OP_DELETE, operation);
                    clMedCorObjChangeHdlr(corTxnId, jobId,  pCookie);
                break;

                case CL_COR_OP_SET:
                    clCorTxnJobAttrPathGet(corTxnId, jobId, &pAttrPath);

                    if(pAttrPath == NULL)
                    {
                        clMedNotifyHandler(corTxnId, jobId, pCookie); 
                    }
                    else if (!pAttrPath->depth)
                    {
                        rc = clMedCorIndexValueChangeHdlr(corTxnId, pCookie);
                        if (rc != CL_OK)
                        {
                            clLogError(CL_MED_AREA, CL_MED_CTX_CFG, "Failed to handle index value change notification. rc [0x%x]", rc);
                        }
                    }
                    else/*Notification for containement set*/
                    {
                        clMedSetOpTxnJobWalk(corTxnId, jobId, pCookie);
                    }
                    break;

                default:
                break;
            }
        }while(clCorTxnNextJobGet(corTxnId, jobId, &jobId) == CL_OK);
        clCorTxnIdTxnFree(corTxnId);
    }
	clEventFree(eventHandle);
}


ClRcT clMedSetOpTxnJobWalk(ClCorTxnIdT       corTxnId,
                            ClCorTxnJobIdT   jobId,
                            void            *pCookie)
{
        /*
        ClMedAttrPathInfoT *attrPathInfo = (ClMedAttrPathInfoT *)cookie;
        ClEventHandleT eventHandle = attrPathInfo->eventHandle;
        ClSizeT eventDataSize = attrPathInfo->eventDataSize;
         */
       ClCorAttrIdT        attrId;
       ClInt32T            index;
       void               *pValue;
       ClUint32T           size;
 


        clCorTxnJobSetParamsGet(corTxnId,
                                  jobId,
                                   &attrId,
                                     &index,
                                        &pValue,
                                          &size);

        switch(attrId)
            {
                    case CL_CONTAINMENT_ATTR_VALID_ENTRY:
                        /* clMedCorObjChangeHdlr(eventHandle, eventDataSize, CL_COR_OP_SET);*/
                         clMedCorObjChangeHdlr(corTxnId, jobId, pCookie);
                        break;
                    case CL_ALARM_ACTIVE:
                        /*  clMedContainmentAttrNotify(eventHandle, eventDataSize); */
                         clMedContainmentAttrNotify(corTxnId, jobId, pCookie); 
                        break;
                    default:
                        break;
            }
    return CL_OK;
}
/* This validates whether the instance is present in the instance table */
ClRcT clMedInstValidate( ClMedHdlPtrT      pMm,
                        ClMedVarBindT   *pVarInfo)
{
    ClRcT               rc = CL_OK;
    ClMedObjT           *pMedObj = (ClMedObjT *)pMm;
    ClMedAgentIdT       *pAgntId = &pVarInfo->attrId;
    ClMedIdXlationT     *pIdXlation;  /* Pointer to the Translation*/

    if(!pMedObj)
    {
        clLogError(CL_MED_AREA, NULL,
                "Null pointer passed");
        return CL_ERR_NULL_POINTER;
    }
    if (CL_OK != (rc = clCntDataForKeyGet(pMedObj->idXlationTbl,
                                          (ClCntKeyHandleT) pAgntId,
                                          (ClCntDataHandleT *)&pIdXlation)))
    {
        clLogError(CL_MED_AREA, NULL,
                "Failed to find entry in idXlationTbl for oid[%s]", 
                pAgntId->id);
        medErrIdXlate(pMedObj, rc, pVarInfo);
        return rc;
    }

    if (0 != pIdXlation->instXlationTbl) /* Instance table is not NULL*/
    {
        ClCntNodeHandleT     hNode = 0; /* Handle to the node */
        ClUint8T *pChar = (ClUint8T *)pVarInfo->pInst;

        rc = clCntNodeFind(pIdXlation->instXlationTbl,
                (ClCntKeyHandleT) pChar,
                (ClCntDataHandleT*)&hNode);
        if(CL_OK != rc)
        {
            rc = CL_GET_ERROR_CODE(rc);
            medErrIdXlate(pMedObj, rc, pVarInfo);
            clLogError(CL_MED_AREA, NULL,
                    "Failed to find entry in instXlationTbl. Instance is invalid. Setting error[0x%x]",
                    pVarInfo->errId);
        }
    }
    return rc;
}
