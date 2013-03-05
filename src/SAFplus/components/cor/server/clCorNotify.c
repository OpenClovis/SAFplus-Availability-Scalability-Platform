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
 * File        : clCorNotify.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * COR Attribute change notification internal functions
 *****************************************************************************/

/* FILES INCLUDED */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clCorErrors.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>

/** Internal Headers **/
#include <clCorNotify.h>
#include <clCorDmProtoType.h>
#include <clCorRmDefs.h>
#include <clCorLog.h>
#include <clCorPvt.h>

#include <xdrClCorOpsT.h>
#include <xdrClCorMOIdT.h>
#include <xdrClCorAttrPathT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALS */
ClIocPhysicalAddressT corAddr;

#define COR_EVT_TIMEOUT CL_RMD_DEFAULT_TIMEOUT
#define CL_COR_COMP_DEATH_SUBSCRIPTION_ID 1
#define CL_COR_NODE_DEATH_SUBSCRIPTION_ID 2
#define CL_COR_COMP_DEPARTURE_SUBSCRIPTION_ID 3
#define CL_COR_NODE_DEPARTURE_SUBSCRIPTION_ID 4

extern ClCorInitStageT    gCorInitStage;

/*Forward declarations for Event related functions*/
void corEvtChannelOpenCallBack( ClInvocationT invocation, 
        ClEventChannelHandleT channelHandle, ClRcT error );

void corEvtEventDeliverCallBack( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize );

/* callbacks for Events */
ClEventCallbacksT corEvtCallbacks =
{
	corEvtChannelOpenCallBack,
	corEvtEventDeliverCallBack
};

ClEventInitHandleT corEventHandle;
ClEventChannelHandleT corEventChannelHandle;
ClEventChannelHandleT cpmEventChannelHandle;
ClEventChannelHandleT cpmEventNodeChannelHandle;


/* FORWARD DECLARTIONS */
void corNotifyShowAll(char**);
ClRcT corMOIdExprSet(ClCorMOIdPtrT moId, ClRuleExprT* pRbeExpr);
ClRcT corAttrPathExprSet(ClCorAttrPathPtrT pAttrPath, ClRuleExprT* pRbeExpr);
int clCorDoesPathContainWildCard(ClCorMOClassPathPtrT moClsPath);
int clCorDoesAttrPathContainWildCard (ClCorAttrPathPtrT pAttrPath);

/** 
 *  Initialize COR notification module.
 *
 *  This routine initializes COR notification module. It creates
 * the COR event channel in EM. This routine is called during
 * COR initialization from corInit. This is not an external API.
 *
 *  @param  N/A
 *
 *  @returns CL_OK  on success<br>
 *       value returned by #emEventCreate on failure.
 */
ClRcT 
corEventInit(void)
{
	ClRcT rc;
	ClNameT evtChannelName;
	ClVersionT ver = CL_EVENT_VERSION;
    ClUint32T                 compDeathPattern   = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClUint32T                 compDeparturePattern   = htonl(CL_CPM_COMP_DEPART_PATTERN);

    ClUint32T                 nodeDeathPattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
    ClUint32T                 nodeDeparturePattern = htonl(CL_CPM_NODE_DEPART_PATTERN);

    ClEventFilterT            compDeathFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(compDeathPattern), (ClUint8T*)&compDeathPattern}}
    };
    ClEventFilterArrayT       compDeathFilterArray = {sizeof(compDeathFilter)/sizeof(compDeathFilter[0]), 
                                                     compDeathFilter
    };

    ClEventFilterT            compDepartureFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(compDeparturePattern), (ClUint8T*)&compDeparturePattern}}
    };
    ClEventFilterArrayT       compDepartureFilterArray = {sizeof(compDepartureFilter)/sizeof(compDepartureFilter[0]), 
                                                     compDepartureFilter
    };

    ClEventFilterT            nodeDeathFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeathPattern),
                                                                (ClUint8T*)&nodeDeathPattern}}
    };
    ClEventFilterArrayT       nodeDeathFilterArray = {sizeof(nodeDeathFilter)/sizeof(nodeDeathFilter[0]),
                                                          nodeDeathFilter 
    };

    ClEventFilterT            nodeDepartureFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeparturePattern),
                                                                (ClUint8T*)&nodeDeparturePattern}}
    };
    ClEventFilterArrayT       nodeDepartureFilterArray = {sizeof(nodeDepartureFilter)/sizeof(nodeDepartureFilter[0]),
                                                          nodeDepartureFilter 
    };

	CL_FUNC_ENTER();
	
	/* First call the function to initialize COR events */
	rc = clEventInitialize(&corEventHandle,&corEvtCallbacks, &ver);

	if(CL_OK != rc)
	{
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_2_INIT, "Event Client", rc);
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Event Client init failed rc => [0x%x]\n", rc));
		return rc;
	}


	/**** Open Publish Channel for COR */
	evtChannelName.length = strlen(clCorEventName);
	memcpy(evtChannelName.value, clCorEventName, evtChannelName.length+1);
		
	rc = clEventChannelOpen(corEventHandle, &evtChannelName, 
			CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, 
			COR_EVT_TIMEOUT, 
			&corEventChannelHandle);
	
	if(CL_OK != rc)
	{
               clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
					CL_LOG_MESSAGE_1_EVENT_PUBLISH_CHANNEL_OPEN, rc); 
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("\n Event Publish Channel Open failed for COR rc => [0x%x]\n", rc));
		return rc;
	}

	/**
     * Open component event channel of CPM 
     */
	evtChannelName.length = strlen(CL_CPM_EVENT_CHANNEL_NAME);
        strcpy(evtChannelName.value, CL_CPM_EVENT_CHANNEL_NAME);
		
	rc = clEventChannelOpen(corEventHandle, &evtChannelName, 
			CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, 
			(ClTimeT)-1, 
			&cpmEventChannelHandle);
	if(CL_OK != rc)
	{
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
						CL_LOG_MESSAGE_1_CHANNEL_OPEN_COMP_TERMINATION, rc);
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("\n Event Subscribe Channel Open failed for CPM rc => [0x%x]\n", rc));
		return rc;
	}

    /*
     * Subscribe for component death event.
     */
    rc = clEventSubscribe(cpmEventChannelHandle, &compDeathFilterArray, CL_COR_COMP_DEATH_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        clLogError("NOTIFY", "SUBSCRIBE", "Failed to subscribe for component death event. rc [0x%x]", rc);
        return rc;
    }

	/*
     * Subscribe for component departure event. 
     */
    rc = clEventSubscribe(cpmEventChannelHandle, &compDepartureFilterArray, CL_COR_COMP_DEPARTURE_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        clLogError("NOTIFY", "SUBSCRIBE", "Failed to subscribe for component departure event. rc [0x%x]", rc);
        return rc;
    }
	
    /**
     * Open node event channel of CPM.
     */
	evtChannelName.length = strlen(CL_CPM_NODE_EVENT_CHANNEL_NAME);
    strcpy(evtChannelName.value, CL_CPM_NODE_EVENT_CHANNEL_NAME);
		
	rc = clEventChannelOpen(corEventHandle, &evtChannelName, 
			CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, 
			(ClTimeT)-1, 
			&cpmEventNodeChannelHandle);
	if(CL_OK != rc)
	{
             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
					CL_LOG_MESSAGE_1_CHANNEL_OPEN_NODE_ARRIVAL, rc);
             CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("\n Event Subscribe Channel Open failed for CPM rc => [0x%x]\n", rc));
	     return rc;
	}

    /*
     * Subscribe for node death event.
     */
    rc = clEventSubscribe(cpmEventNodeChannelHandle, &nodeDeathFilterArray, 
                          CL_COR_NODE_DEATH_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        clLogError("NOTIFY", "SUBSCRIBE", "Failed to subscribe for node death event. rc [0x%x]", rc);
        return rc;
    }

    /*
     * Subscribe for node departure event.
     */
    rc = clEventSubscribe(cpmEventNodeChannelHandle, &nodeDepartureFilterArray, 
                          CL_COR_NODE_DEPARTURE_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        clLogError("NOTIFY", "SUBSCRIBE", "Failed to subscribe for node departure event. rc [0x%x]", rc);
        return rc;
    }

    CL_FUNC_EXIT();
    return (rc);
}


/* Finalize function for Event */
ClRcT corEventFinalize(void)
{

 /* Close all the event channels */
  clEventChannelClose(corEventChannelHandle);
  clEventChannelClose(cpmEventChannelHandle);
  clEventChannelClose(cpmEventNodeChannelHandle);

  return (clEventFinalize(corEventHandle));
}



ClRcT VDECL(_CORNotifyGetAttrBits) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                        ClBufferHandleT  outMsgHandle)
{
	ClRcT             rc = CL_OK;
	ClCorMOIdT        moId;
 	ClCorClassTypeT   classId;
	ClCorMOClassPathT moClsPath;
	ClUint32T         attrCnt;
    ClCorAttrPathT    attrPath;
	ClCorAttrListT   *pAttrList = NULL;
    ClCorAttrBitsT    attBits = {{0}};
	ClVersionT version;

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("NTF", "ATB", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    /* Adding the version related check */
	if((rc = clXdrUnmarshallClVersionT(inMsgHandle, &version )) != CL_OK)
	     return (rc);
	/* Client to Server Version Check */
	clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
	{
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

	if((rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(inMsgHandle, &moId )) != CL_OK)
	     return (rc);

	 if((rc = VDECL_VER(clXdrUnmarshallClCorAttrPathT, 4, 0, 0)(inMsgHandle, &attrPath )) != CL_OK)
	     return (rc);
 
	 if((rc = clXdrUnmarshallClUint32T(inMsgHandle, &attrCnt )) != CL_OK)
	     return (rc);
	

        clCorMoIdToMoClassPathGet (&moId, &moClsPath);
        /* can not use wild card in moID when requesting specific attribute change */
	if ((moId.svcId == CL_COR_SVC_WILD_CARD) || (clCorDoesPathContainWildCard(&moClsPath)))
    	{
                 CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not use wild cards for class type/svcId \
		                                      when using a list of attribute Ids\n"));
        	CL_FUNC_EXIT();
        	return (CL_COR_SET_RC(CL_COR_NOTIFY_ERR_CANNOT_RESOLVE_CLASS));
    	}

        /* get the class for MO/MSO */
        if (moId.svcId == CL_COR_INVALID_SRVC_ID)
           {
	     classId = moId.node[moId.depth - 1].type;
	   }
           else{
			if ((rc = corMOTreeClassGet (&moClsPath, moId.svcId, &classId)) != CL_OK)
			{
				CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not get the classId \
				   for the supplied moId/svcId combination\n"));
				CL_FUNC_EXIT();
				return (rc);
			}
		}

              /* Get the class Id from attribute path. */
              if(attrPath.depth != 0)
              {
                  if (clCorDoesAttrPathContainWildCard(&attrPath))
                  {
                    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not use wild cards for AttrId in AttrPath \
                                                       when using a list of attribute Ids\n"));
                    CL_FUNC_EXIT();
                     return (CL_COR_SET_RC(CL_COR_NOTIFY_ERR_CANNOT_RESOLVE_CLASS));
                   }
                  if((rc = dmAttrPathToClassIdGet(classId, &attrPath, &classId)) != CL_OK)
                   {
                    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Invalid Attribute Path !! \n"));
                    clCorAttrPathShow(&attrPath);
                    CL_FUNC_EXIT();
                    return (rc);
                   }
              }

             /* Allocate memory and get the list of attributes*/
              pAttrList = clHeapAllocate(sizeof(ClCorAttrListT) + (attrCnt * sizeof(ClUint32T)));
              if(pAttrList == NULL)
              {
                 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
	                 CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                 CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
	         return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	      }
              pAttrList->attrCnt = attrCnt;

             if((rc = clXdrUnmarshallArrayClUint32T(inMsgHandle, &pAttrList->attr, attrCnt)) != CL_OK) 
              {
                 clHeapFree(pAttrList);
                 return (rc);
              }

		/* Convert attribute list into the attribute bits */
              if ((rc = dmClassBitmap (classId,
	                     &pAttrList->attr[0],
                             pAttrList->attrCnt,
                             &attBits.attrBits[0],
                             sizeof(ClCorAttrBitsT))) != CL_OK)
               {
                        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Attribute list to atrribute bits \
                           conversion failed rc => [0x%x]\n", rc));
                        clHeapFree(pAttrList);
                        CL_FUNC_EXIT();
                        return (rc);
               }
              rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *)&attBits, sizeof(ClCorAttrBitsT));
   clHeapFree(pAttrList);
  return (rc);
}

/**
 *  Build event data and publish on COR channel.
 *
 *  This routine prepares the event data and publishes the change event.
 *
 *  @param txnStream    transaction stream. IT IS ASSUMED THAT IT
 *                       IS PACKED, i.e., ALL THE TXN DATA IS CONTIGUOUS.
 *              op       type of change (create/delete/set etc.)
 *        changedBits    Bits corresponding to the changed attributes.
 *
 *  @returns CL_OK on success<br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on failure.
 */
ClRcT
corEventPublish (ClCorOpsT                  op,
                 ClCorMOIdPtrT              pMoId,
                 ClCorAttrPathPtrT          pAttrPath,
                 ClCorAttrBitsT            *pChangedBits,
                 ClUint8T                  *pPayload,
                 ClUint32T                  payloadSize)
{
    ClRcT          rc = CL_OK;
	ClUint8T* corEventOp = NULL;
	ClUint8T* corEventMOId = NULL;
	ClUint8T* corEventAttrPath = NULL;
	ClUint8T* corEventAttrBits = NULL;
	ClEventPatternArrayT eventPatternArray;
	ClNameT corEventChannelName;
	ClEventIdT corPublishEvtId;
	ClUint32T	patternSize;
	ClEventPatternT eventPattern[4];
    ClEventHandleT corPublishEvtHandle;

    if ((pPayload == NULL) || (pChangedBits == NULL))
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT", "NULL argument passed.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* Construct the event pattern for OPs */
	patternSize= sizeof(ClCorOpsT);
	if((corEventOp = (void*)clHeapAllocate(patternSize)) == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT", "Failed to allocate memory");
		return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
	}

	memcpy(corEventOp, &op, sizeof(ClCorOpsT));

    /* Construct the first element of event pattern array*/
	eventPattern[0].allocatedSize = 0;
	eventPattern[0].patternSize = patternSize;
	eventPattern[0].pPattern = corEventOp;

    /* Construct the event pattern for MOId */
	patternSize= sizeof(ClCorMOIdT);
	if((corEventMOId = (void*)clHeapAllocate(patternSize)) == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT",  CL_COR_ERR_STR(CL_COR_ERR_NO_MEM));
        clHeapFree(corEventOp);
		return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
	}

	memcpy(corEventMOId, pMoId, patternSize);

    /* Construct the first element of event pattern array*/
	eventPattern[1].allocatedSize = 0;
	eventPattern[1].patternSize = patternSize;
	eventPattern[1].pPattern = corEventMOId;


    /* Construct the event pattern for Attribute Path */
	patternSize= sizeof(ClCorAttrPathT);
	if((corEventAttrPath = (void*)clHeapAllocate(patternSize)) == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT",  CL_COR_ERR_STR(CL_COR_ERR_NO_MEM));
        clHeapFree(corEventOp);
        clHeapFree(corEventMOId);
		return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
	}

    memcpy(corEventAttrPath, pAttrPath, patternSize);

     /* Construct the first element of event pattern array*/
	eventPattern[2].allocatedSize = 0;
	eventPattern[2].patternSize = patternSize;
	eventPattern[2].pPattern = corEventAttrPath;


    /* Construct the event pattern for attribute bits*/	
    patternSize= sizeof(ClCorAttrBitsT);
    if((corEventAttrBits = (void*)clHeapAllocate(patternSize)) == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT",  CL_COR_ERR_STR(CL_COR_ERR_NO_MEM));
        
        clHeapFree(corEventOp);
        clHeapFree(corEventMOId);
        clHeapFree(corEventAttrPath);

		return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
	}

	memcpy(corEventAttrBits, pChangedBits, patternSize);
	eventPattern[3].allocatedSize = 0;
	eventPattern[3].patternSize = patternSize;
	eventPattern[3].pPattern = corEventAttrBits;

     /** Fill the pattern related info */
	eventPatternArray.allocatedNumber = 0;
	eventPatternArray.patternsNumber = sizeof(eventPattern)/sizeof(ClEventPatternT);
	eventPatternArray.pPatterns = eventPattern;

     /* Construct the event channel */
	corEventChannelName.length = strlen(clCorEventName);
	memcpy(corEventChannelName.value, clCorEventName, corEventChannelName.length+1);

    rc = clEventAllocate(corEventChannelHandle, &corPublishEvtHandle);
	if(CL_OK != rc)
	{     
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL, 
            CL_LOG_MESSAGE_1_EVENT_MESSAGE_ALLOCATION, rc);
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Failed to allocate event. rc => [0x%x]\n", rc));

        /* Free all the corEvent filter buffers */
        clHeapFree(corEventOp);
         clHeapFree(corEventMOId);
        clHeapFree(corEventAttrPath);
        clHeapFree(corEventAttrBits);

        CL_FUNC_EXIT();
        return (rc);
	}

	rc = clEventAttributesSet(corPublishEvtHandle, &eventPatternArray, 0, 0, &corEventChannelName);
	if(CL_OK != rc)
	{
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT", "Failed to set event attributes. rc [0x%x]", rc);

        /* Free all the corEvent filter buffers */
        clHeapFree(corEventOp);
        clHeapFree(corEventMOId);
        clHeapFree(corEventAttrPath);
        clHeapFree(corEventAttrBits);
        clEventFree(corPublishEvtHandle);

        CL_FUNC_EXIT();
        return rc;
	}

    if(CL_OK == rc)
    {
        rc = clEventPublish(corPublishEvtHandle, (void *)pPayload, 
	            payloadSize, &corPublishEvtId);

        if(CL_OK != rc)
	    {
		    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
					CL_LOG_MESSAGE_1_EVENT_PUBLISH, rc);
            clLog(CL_LOG_SEV_ERROR, "TXN", "EVT", "Failed to publish event. rc [0x%x]", rc);
            
            /* Free all the corEvent filter buffers */
            clHeapFree(corEventOp);
            clHeapFree(corEventMOId);
            clHeapFree(corEventAttrPath);
            clHeapFree(corEventAttrBits);
            clEventFree(corPublishEvtHandle);

            return rc;
        }
    }
 
    /* Free the Cor event handle */
    rc = clEventFree(corPublishEvtHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to free the event handle. rc [0x%x]\n", rc));
    }

    /* Free all the corEvent filter buffers */
    clHeapFree(corEventOp);
    clHeapFree(corEventMOId);
    clHeapFree(corEventAttrPath);
    clHeapFree(corEventAttrBits);

    CL_FUNC_EXIT();
    return (rc);
}


void corEvtChannelOpenCallBack( ClInvocationT invocation, 
        ClEventChannelHandleT channelHandle, ClRcT error )
{

}

#if 0
/* As CPM is creating the IM on start up, we need not create the MO/MSO here. */


/*
   * This function creates node objects when a node is inserted.  The CPM 
   * publishes an event when node is inserted. Upon receiving this event
   * the node objects are created.
   *
   * THIS IS A TEMPORARY FIX. THIS IS NOT A FUNCTIONALITY OF COR
   * GOING FORWARD IT NEEDS TO BE THOUGHT ABOUT WHO IS SUPPOSED
   * TO DO THIS OPERATION.
*/
ClRcT corNodeObjectCreate(ClUint32T logicalSlot)
{
	ClRcT rc;
	 ClCorMOIdT moId;
	 ClCorObjectHandleT objH;
	 ClInt16T depth;

	 /* First get MOId from the IOC address */
	 /* @toDo : We can assume for now that logical slot is same as IOC address*/
	 rc = clCorLogicalSlotToMoIdGet(logicalSlot, &moId);

	 if(CL_OK != rc)
	 	{
	 	CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nCould not get MOId from logical slot\n", rc);
	 	}

	 depth = clCorMoIdDepthGet(&moId);

	 if(CL_OK != rc)
	 	{
	 	CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nCould not get MOId from logical slot", rc);
	 	}

	 for(moId.depth = 1;moId.depth <= depth; moId.depth++)
	 	{
		clCorMoIdServiceSet(&moId, CL_COR_INVALID_SRVC_ID);
	 	rc = clCorUtilMoAndMSOCreate(&moId, &objH);
		if(CL_OK != rc)
			{
			/* Check if the object is already present. If so just continue */
			if( (rc == CL_COR_SET_RC(CL_COR_INST_ERR_MSO_ALREADY_PRESENT)) ||
			        (rc == CL_COR_SET_RC(CL_COR_INST_ERR_MO_ALREADY_PRESENT)) )
				{
				/* The MO or MSO is already present. Just continue */
				continue;
				}
			else
				{
				CL_COR_RETURN_ERROR(CL_DEBUG_ERROR,"\nCould not create objects ", rc);
				}
			}
	 	}
	 return CL_OK;

}
#endif


void corEvtEventDeliverCallBack( ClEventSubscriptionIdT subscriptionId,
                                 ClEventHandleT eventHandle, ClSizeT eventDataSize)
{

    /* The event handling code goes here */
    ClRcT rc = CL_OK;
          
    /* Event demultiplexing using subscription id.
       (we might have to compare channel name, if there are differnt event
       types coming from diffrent publishers)*/

    switch(subscriptionId)
    {
        case CL_COR_COMP_DEATH_SUBSCRIPTION_ID:
        case CL_COR_COMP_DEPARTURE_SUBSCRIPTION_ID:
        {
            ClCpmEventPayLoadT payLoad;
            ClCorAddrT   stationAdd;

            if((rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, CL_CPM_COMP_EVENT, &payLoad)) != CL_OK)
            {
    		    clEventFree(eventHandle);
	            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Event Data Extra failed !!!!  rc => [0x%x]\n", rc));
				return;
            }
            
            if( (payLoad.operation != CL_CPM_COMP_DEATH) &&
                (payLoad.operation != CL_CPM_COMP_DEPARTURE))
            {
                clEventFree(eventHandle);
                return;
            }

            stationAdd.nodeAddress = payLoad.nodeIocAddress;
            stationAdd.portId =     payLoad.eoIocPort;

            /** Disable all the rules associated with the Station in Route list **/ 
            rc =  _clCorStationDisable(stationAdd);              
            clLogInfo("COMP", "DEATH", "Event received for component death/departure." \
                      "nodeAddress 0x%x, portId 0x%x", stationAdd.nodeAddress, stationAdd.portId);
        }
        break;

        case CL_COR_NODE_DEATH_SUBSCRIPTION_ID:
        case CL_COR_NODE_DEPARTURE_SUBSCRIPTION_ID:
        {
            ClCpmEventNodePayLoadT payLoad;
            ClIocNodeAddressT nodeAddr = 0; 
            
            if((rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, CL_CPM_NODE_EVENT, &payLoad)) != CL_OK)
            {
                clEventFree(eventHandle);
                CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Event Data Extra failed !!!!  rc => [0x%x]\n", rc));
                return;
            }

            if((payLoad.operation != CL_CPM_NODE_DEATH) &&
               (payLoad.operation != CL_CPM_NODE_DEPARTURE))
            {
                clEventFree(eventHandle);
                return;
            }

            /* In the case of node death/departure, we need to remove the 
             * entries from the cor List as well as all the components
             * for that node from the route list.
             */
            nodeAddr = payLoad.nodeIocAddress;
            rc = _clCorRemoveStations(nodeAddr);
            if( rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to remove entries for the Node Address 0x%x", nodeAddr));

            clLogInfo("NODE", "DEATH", "Receiving Node death/departure event. nodeAddress 0x%x", nodeAddr);
        }
        break;

    default:
        /* nothing to be done*/
        break;
    }

    clEventFree(eventHandle);
}
