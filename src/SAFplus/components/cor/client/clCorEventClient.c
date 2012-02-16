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
 * File        : clCorEventClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains exported cor Event APIs
 *****************************************************************************/

/* FILES INCLUDED */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
                                                                                                                             
#include <clCommon.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clLogApi.h>
#include <clBitApi.h>
#include <clEventApi.h>
#include <clEventExtApi.h>
#include <clCorUtilityApi.h>
#include <clCorNotifyApi.h>
#include <clCorErrors.h>
#include <clCorTxnClientIpi.h>
#include <clCorRMDWrap.h>
#include <clCorTxnApi.h>
#include <clLogApi.h>

/* Internal Headers*/
#include <clCorClient.h>
#include <clCorLog.h>
#include <clCorNotifyCommon.h>

#include <xdrClCorMOIdT.h>
#include <xdrClCorAttrPathT.h>

/* Externs */
extern ClRcT corAttrPathExprH2NSet(ClCorAttrPathPtrT pAttrPath,
               ClRuleExprT* pRbeExpr);

extern ClRcT corMOIdExprH2NSet(ClCorMOIdPtrT moId,
               ClRuleExprT* pRbeExpr);


ClRcT
clCorEventRbeExprPrepare (ClCorMOIdListPtrT pMoIdList,
                        ClCorAttrPathPtrT pAttrPath,
                        ClCorAttrListPtrT attrList, 
                        ClCorOpsT ops,
                        ClRuleExprT* *pRbeExpr);

ClRcT
corEventSubscribe(ClEventChannelHandleT channelHandle,
                    ClCorMOIdListPtrT   pMoIdList,
                    ClCorAttrPathPtrT   pAttrPath,
                    ClCorAttrListPtrT   pAttrList,
                    ClCorOpsT           flags,
                    void*               cookie,
                    ClEventSubscriptionIdT subscriptionID);

/**
 *  Return TXN handle given COR notification data.
 *
 *  This function returns reference to the transaction handle
  *   within the COR event data.
 *
 *  @param evtH     Handle to the event data which is passed to the subscription
 *                   callout.
 *         pTransH  [OUT] TXN handle is retunrd here.
 *
 *  @returns CL_OK on success<br>
 *       CL_NU
 */
ClRcT
clCorEventHandleToCorTxnIdGet(ClEventHandleT evtH, ClSizeT size,  ClCorTxnIdT *pTxnId)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT   msgHdl;
    ClCorTxnJobHeaderNodeT  *pJobHdrNode;
    ClUint8T                *pEvtPayload;

    CL_FUNC_ENTER();
 
    if(!pTxnId || !evtH || !size)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid Parameter  \n"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    /* Allocate memory */
     pEvtPayload = (ClUint8T *)clHeapAllocate((ClUint32T)size);
     if(pEvtPayload == NULL)
     {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
			CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
        CL_FUNC_EXIT();
	return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
     }

    /*Get the data from the event */
      rc = clEventDataGet(evtH, (void *)pEvtPayload, &size);
      if(rc!=CL_OK)
      {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get Event Data"));
	   clHeapFree(pEvtPayload);
           CL_FUNC_EXIT();
	   return rc;
      } 


      if((rc = clBufferCreate(&msgHdl)) != CL_OK)
      {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed rc:0x%x", rc));
	   clHeapFree(pEvtPayload);
           CL_FUNC_EXIT();
           return (rc);
      }

      if((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *)pEvtPayload, size)) != CL_OK)
      {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Write Failed   rc:0x%x", rc));
	   clHeapFree(pEvtPayload);
           CL_FUNC_EXIT();
           return (rc);
      }
     
     /* We dont need payLoad anymore. It has been consumed */
         clHeapFree(pEvtPayload);

      if((rc = clCorTxnJobStreamUnpack(msgHdl, &pJobHdrNode)) != CL_OK)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Cor-Txn Job Stream Unpack Failed rc:0x%x", rc));
        clBufferDelete(&msgHdl);
        CL_FUNC_EXIT();
        return (rc);
      }
 
    /* Delete the buffer. */
       clBufferDelete(&msgHdl);
 
   
    /* Update the Tid */
    *pTxnId =  (ClCorTxnIdT)pJobHdrNode;
  

      CL_FUNC_EXIT();
  return (rc);
}


/**
 *  Subscribe for attribute change notification.
 *
 *  This routine allows subscription to attribute change notification
 *
 *  @param eoId     ID of the EO requesting the subscription (refer to as subscriber).
 *    changedObj    Subscriber (eoID) is interested in changes to this
 *                   object (refer to as publisher). This is full path to the object.
 *                   Wild cards can be used to specify a 'class' or 'subtree'
 *                   of objects. See the corPath documentation for more details.
 *        svcId    Service ID of the changedObj (publisher). This is used to
 *                   subscribe changes from an MSO. A wildcard can be used to
 *                   subscribe from multiple MSO's which qualify the changedObj path.
 *                   Value of -1 indicates subscription from a non-MSO(s).
 *     attrList    If the subscriber is interested in notification only if certain
 *                   attribute(s) of the object change, list of these attribute Ids
 *                   can be specified here. NULL can be passed to indicate a
 *                   attribute Id wildcard, in which case subscriber is notified
 *                   on change to any attribute. Following are some of the very
 *                   important usage restrictions (may be obvious) for this parameter: *                  i) This parameter is interpreted only if #ops contains one or
 *                     more _SET_ operations. See #ClCorOpsT for all the possible
 *                     operations types.
 *                 ii) For attribure Ids in attrList to make sense, changedObj and
 *                     svcId combination have to resolve in exactly one object class.
 *                     This means if you are interested at attribute level change:
 *                      a) You can not use wildcard for svcId.
 *                      b) You can not use wildcard for any class value in the
 *                         changedObj.
 *                      c) It is ok to use wildcard for instance value in changedObj.
 *        rmdId    Fully qualified RMD function number of the change
 *                 notification callback.
 *        ops      Operations (CREATE/DELETE/SET) in which subscriber is interested
 *                 in. See #ClCorOpsT for more details.
 *       cookie    Subscriber can store an arbitrary cookie value. It is
 *                 returned as is to the change notification callback.
 *   pSubHandle    [OUT] Subscription handle is returned here. Caller can
 *                 used this to unsubscribe later.
 *
 *  @returns CL_OK on success<br>
 *       value returned by #clEvtSubscribe on failure.
 */

ClRcT
clCorEventSubscribe(ClEventChannelHandleT channelHandle,
                                ClCorMOIdPtrT      changedObj,
                                ClCorAttrPathPtrT  pAttrPath,
                                ClCorAttrListPtrT attrList,
                                ClCorOpsT    flags,
                                void* cookie,
                                ClEventSubscriptionIdT subscriptionID)
{
    ClRcT           rc = CL_OK;
    ClCorMOIdListT  moIdList;

    CL_FUNC_ENTER();

    if (changedObj == NULL)
    {
        clLogError("COR", "EVT", "MoId passed is NULL.");
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    moIdList.moIdCnt = 1;
    moIdList.moId[0] = *changedObj;

    rc = corEventSubscribe(channelHandle, 
            &moIdList,
            pAttrPath,
            attrList,
            flags,
            cookie,
            subscriptionID);
    if (rc != CL_OK)
    {
        clLogError("COR", "EVT", "Failed to subscribe on the COR_EVT_CHANNEL. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
clCorMOListEventSubscribe(ClEventChannelHandleT     channelHandle,
                                ClCorMOIdListPtrT   pMoIdList,
                                ClCorAttrPathPtrT   pAttrPath,
                                ClCorAttrListPtrT   pAttrList,
                                ClCorOpsT           ops,
                                void*               cookie,
                                ClEventSubscriptionIdT subscriptionID)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    rc = corEventSubscribe(channelHandle,
            pMoIdList,
            pAttrPath,
            pAttrList,
            ops,
            cookie,
            subscriptionID);
    if (rc != CL_OK)
    {
        clLogError("COR", "EVT", "Failed to subscribe on the COR_EVT_CHANNEL. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
corEventSubscribe(ClEventChannelHandleT channelHandle,
                    ClCorMOIdListPtrT   pMoIdList,
                    ClCorAttrPathPtrT   pAttrPath,
                    ClCorAttrListPtrT   pAttrList,
                    ClCorOpsT           flags,
                    void*               cookie,
                    ClEventSubscriptionIdT subscriptionID)
{
    ClRcT  rc = CL_OK;
    ClRuleExprT* rbeExp;
                                                                                
    if (pMoIdList == NULL) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL Argument"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    else if ((!(flags & CL_COR_OP_ALL)) || (flags & ~CL_COR_OP_ALL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid operation"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_NOTIFY_ERR_INVALID_OP));
    }

    /* Prepare the rbe expression based on the changedObj, pAttrPath and attrList */
    if ((rc = clCorEventRbeExprPrepare (pMoIdList, pAttrPath, 
                                      pAttrList, flags, &rbeExp)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Could not create rbe expression rc = [0x%x]\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }
 
    rc = clEventExtWithRbeSubscribe(channelHandle, rbeExp, subscriptionID, cookie);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("clEvtSubscribe failed, rc = [0x%x]\n", rc));
        return rc;
    } 
	
#ifdef DEBUG 
   /* clEvtStatsPrint (corAddr.nodeAddress, 0); */
#endif

    clRuleExprDeallocate (rbeExp);
    CL_FUNC_EXIT();
    return (rc);
}



/**
 *  Un-subsribe for attribute change notification.
 *
 *  This routine allows un-subscription to attribute change notification
 *
 *  @param subHandle Subscription handle. This handle is returned by the
 *                   #clCorEventSubscribe API.
 *
 *  @returns CL_OK on success<br>
 *       value returned by #emEventUnSubscribe on failure.
 */

ClRcT
clCorEventUnsubscribe(ClEventChannelHandleT channelHandle, ClEventSubscriptionIdT subscriptionID)
{

	return( clEventUnsubscribe(channelHandle, subscriptionID));
	
}

/*
 * Function to get attribute bits from cor server. 
 * It makes RMD call to get the attrbits. 
 */
ClRcT
clCorObjAttrBitsGet(ClCorMOIdPtrT        changedObj,
                    ClCorAttrPathPtrT    pAttrPath,
                    ClCorAttrListPtrT    pAttrList,
                    ClCorAttrBitsT      *pAttrBits)
{
   ClRcT                     rc;
   ClCorAttrPathT            tempAttrPath;
   ClCorAttrPathT           *pCorAttrPath;
   ClBufferHandleT    inMessageHandle;
   ClBufferHandleT    outMessageHandle;
   ClUint32T                 length = sizeof(ClCorAttrBitsT);
   ClVersionT				 version;

   /* Version Set */
   CL_COR_VERSION_SET(version);

   CL_FUNC_ENTER();

    if ((changedObj == NULL) || (pAttrList == NULL) || (pAttrBits == NULL))
    { 
    CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nNULL Pointer Passed\n", CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clBufferCreate(&inMessageHandle);

   if(rc != CL_OK)
   	{
   	CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Could not create Input message buffer\n"));
	return rc;
   	}

  /* Start writing on the input message buffer */
   if((rc = clXdrMarshallClVersionT(&version, inMessageHandle, 0)) != CL_OK) 
   { 
       clBufferDelete(&inMessageHandle);
       return (rc);
   }

   if((rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)(changedObj, inMessageHandle, 0)) != CL_OK) 
   { 
       clBufferDelete(&inMessageHandle);
       return (rc);
   }

   if(pAttrPath == NULL)
   {  
        pCorAttrPath = &tempAttrPath;
        clCorAttrPathInitialize(pCorAttrPath);
   }
   else
    pCorAttrPath = pAttrPath;

        
   if((rc = VDECL_VER(clXdrMarshallClCorAttrPathT, 4, 0, 0)(pAttrPath, inMessageHandle, 0)) != CL_OK)
   {
       clBufferDelete(&inMessageHandle);
        return (rc);
   }

   if((rc = clXdrMarshallClUint32T(&pAttrList->attrCnt, inMessageHandle, 0)) != CL_OK)
   {
       clBufferDelete(&inMessageHandle);
       return (rc);     
   }

   if((rc = clXdrMarshallArrayClUint32T(&pAttrList->attr, pAttrList->attrCnt, inMessageHandle, 0)) != CL_OK)
   {  
       clBufferDelete(&inMessageHandle);
       return (rc);     
   }

   rc = clBufferCreate(&outMessageHandle);
   if(rc != CL_OK)
   {
       clBufferDelete(&inMessageHandle);
       return (rc);     
   }

   COR_CALL_RMD_SYNC_WITH_MSG(COR_NOTIFY_GET_RBE, inMessageHandle, outMessageHandle, rc);

   if(rc == CL_OK)
   {
      rc = clBufferNBytesRead(outMessageHandle, (ClUint8T *)pAttrBits, &length);
   }

    clBufferDelete(&inMessageHandle);
    clBufferDelete(&outMessageHandle);
  return rc;
}

/** 
 *  Prepare an RBE expression based on the ClCorMOId, serviceId and attrBits.
 *
 *  This routine is the heart of the COR notification support. It 
 * allocates and builds an RBE expression based on the ClCorMOId
 * serviceId and attrBits. This expression is passed to the EM in the 
 * subscribe call. The RBE expression is very tightly coupled with the
 * data published in the event by #clEvtPublish. Any changes here
 * or in the format of the data published requires a careful consideration.
 *
 *  @param changedObj  Changes to this object is of interest to the subscriber.
 *                   This is full path to the object.
 *                   Wild cards can be used to specify a 'class' or 'subtree'
 *                   of objects. See the corPath documentation for more details.
 *        svcId    Service ID of the changedObj. This is used to subscribe
 *                   changes from an MSO. A wildcard can be used to subscribe
 *                   from multiple MSO's which qualify the changedObj path.
 *                   CL_COR_INVALID_SRVC_ID indicates subscription for an MO.
 *     attrList    If the subscriber is interested in notification only if certain
 *                   attribute(s) of the object change, list of these attribute Ids
 *                   can be specified here. NULL can be passed to indicate a 
 *                   attribute Id wildcard, in which case subscriber is notified
 *                   on change to any attribute. Following are some of the very
 *                   important usage restrictions (may be obvious) for this parameter:
 *                  i) This parameter is interpreted only if #ops contains one or
 *                     more _SET_ operations. See #ClCorOpsT for all the possible
 *                     operations types.
 *                 ii) For attribure Ids in attrList to make sense, changedObj and
 *                     svcId combination have to resolve in exactly one object class.
 *                     This means if you are interested at attribute level change:
 *                      a) You can not use wildcard for svcId. 
 *                      b) You can not use wildcard for any class value in the
 *                         changedObj.
 *                      c) It is ok to use wildcard for instance value in changedObj.
 *         ops     Operations (CREATE/DELETE/SET) in which subscriber is interested
 *                   in. See #ClCorOpsT for more details.
 *    pRbeExpr     [OUT] Handle to rbe expression is returned here.
 *
 *  @returns CL_OK on success<br>
 *       value returned by #RBE library.
 *  WARNING : This code depends upon the format and the structure of the ClCorMOId and
 * the way event data are packed by #clEvtPublish. If you make any change to
 * either, no matter how insignificant, revisit this code.
 */
ClRcT
clCorEventRbeExprPrepare (ClCorMOIdListPtrT  pMoIdList,
                          ClCorAttrPathPtrT  pAttrPath,
                          ClCorAttrListT*    pAttrList, 
                          ClCorOpsT          ops,
                          ClRuleExprT      **pRbeExpr)
{
   ClRcT             rc = CL_OK;
   ClUint32T         i;
   ClUint8T          expLen; 
   ClCorAttrPathT    attrPath;
   ClRuleExprT*      moIdExpr;
   ClRuleExprT*      attrPathExpr;
   ClRuleExprT*      attrExpr;
   ClUint32T         size;
   ClUint32T        *pValue;

    CL_FUNC_ENTER();
    if ((pRbeExpr == NULL) || (pMoIdList == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* == Prepare the expression for operation. == */
    size = (sizeof (ClCorOpsT))/sizeof (ClUint32T);
    if ((rc = clRuleExprAllocate (size, pRbeExpr)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("clRuleExprAllocate failed rc => [0x%x]\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }
    
    /* Setup for attribute change */
    clRuleExprMaskSet(*pRbeExpr, 0, CL_BIT_H2N32(ops));

    /* set the value */
    clRuleExprValueSet(*pRbeExpr, 0, 0xffffffff);

    /* set the initial offset */
    clRuleExprOffsetSet(*pRbeExpr, COR_NOTIFY_OPS_OFFSET/sizeof (ClUint32T));

    /* set the flags */
    clRuleExprFlagsSet(*pRbeExpr, CL_RULE_NON_ZERO_MATCH|CL_RULE_EXPR_CHAIN_AND);


    /* Add the list MoIds to the expression. */
    for (i=0; i<pMoIdList->moIdCnt; i++)
    {
        /* == Prepare expression to match the ClCorMOId == */
        expLen = sizeof (ClCorMOIdT)/sizeof (ClUint32T);

        if ((rc = clRuleExprAllocate (expLen, &moIdExpr)) != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clRuleExprAllocate failed rc => [0x%x]\n", rc));
            clRuleExprDeallocate (*pRbeExpr);
            CL_FUNC_EXIT();
            return (rc);
        }
        
        corMOIdExprH2NSet(&(pMoIdList->moId[i]), moIdExpr);

        /* Set the initial offset */
        clRuleExprOffsetSet(moIdExpr, COR_NOTIFY_MOID_OFFSET/sizeof(ClUint32T));
        
        clRuleExprFlagsSet(moIdExpr, CL_RULE_MATCH_EXACT|CL_RULE_EXPR_CHAIN_GROUP_OR);

        /* Both expressions are read, link them */
        clRuleExprAppend (*pRbeExpr, moIdExpr);
    }


    if( !(ops & CL_COR_OP_DELETE) && !(ops & CL_COR_OP_CREATE))
    {
        if (!pAttrPath)
        {
           clCorAttrPathInitialize(&attrPath);
           pAttrPath = &attrPath;
        }
 
        /* == Prepare expression to match the ClCorAttrPathT == */
        expLen = sizeof(ClCorAttrPathT)/sizeof (ClUint32T);

        if ((rc = clRuleExprAllocate (expLen, &attrPathExpr)) != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clRuleExprAllocate failed rc => [0x%x]\n", rc));
                 clRuleExprDeallocate (*pRbeExpr);
            CL_FUNC_EXIT();
            return (rc);
        }

        corAttrPathExprH2NSet(pAttrPath, attrPathExpr);
        /* Set the initial offset */
        clRuleExprOffsetSet(attrPathExpr, COR_NOTIFY_ATTRPATH_OFFSET/sizeof(ClUint32T));
        clRuleExprFlagsSet(attrPathExpr, CL_RULE_MATCH_EXACT|CL_RULE_EXPR_CHAIN_AND);

        /* All three expressions are read, link them */
        clRuleExprAppend (*pRbeExpr, attrPathExpr);

        /* == Prepare the expression for attribute change, IF NECESSARY == */
        if ((ops & CL_COR_OP_SET) && (pAttrList != NULL))
        {
    	    ClCorAttrBitsT attrBits;

            /* Get the attribute bits from Server */
            rc =  clCorObjAttrBitsGet(&(pMoIdList->moId[0]), pAttrPath,  pAttrList, &attrBits);

            /* TODO: check error */
 
	        /* == Prepare the expression for attrBits bits. == */
	        size = (sizeof (ClCorAttrBitsT))/sizeof (ClUint32T);
	        if ((rc = clRuleExprAllocate (size, &attrExpr)) != CL_OK)
	        {
	            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clRuleExprAllocate failed rc => [0x%x]\n", rc));
		        clRuleExprDeallocate (*pRbeExpr);
	            CL_FUNC_EXIT();
	            return (rc);
	        }
            pValue = (ClUint32T *)&attrBits;
 
	        for (i = 0; i < sizeof (ClCorAttrBitsT)/sizeof (ClUint32T); i++)
	        {
	            clRuleExprMaskSet(attrExpr, i, *pValue++);
	        }

            clRuleExprValueSet(attrExpr, 0, 0xffffffff);

	        /* set the initial offset */
	        clRuleExprOffsetSet(attrExpr, COR_NOTIFY_ATTR_BITS_OFFSET/sizeof (ClUint32T));
	
		    /* set the flags */
	        clRuleExprFlagsSet(attrExpr, CL_RULE_NON_ZERO_MATCH|CL_RULE_EXPR_CHAIN_AND);
            /* chain it with previous Rbe expression. */
	        clRuleExprAppend (*pRbeExpr, attrExpr);
	    }
    }

#ifdef DEBUG 
    /* clRuleExprPrint (*pRbeExpr); */
#endif
    /*  clRuleExprPrint (*pRbeExpr); */

    CL_FUNC_EXIT();
    return (CL_OK);
}


