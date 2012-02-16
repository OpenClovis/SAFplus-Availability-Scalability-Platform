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
 * File        : clCorSync.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the code for COR's synchronization
 *****************************************************************************/

#include <clCorErrors.h>
#include <string.h>
#include <clLogApi.h>
#include <clIocErrors.h>

/* Internal Headers */
#include "clCorSync.h"
#include "clCorNiIpi.h"
#include <clCorRMDWrap.h>
#include <clCorDmProtoType.h>
#include <clCorRmDefs.h>
#include "clCorLog.h"
#include "clCorClient.h"
#include "clCorPvt.h"
#include <sys/time.h>

#include <xdrCorSyncPkt_t.h>

#define CL_COR_SERVER_SYNC_RETRIES 5
/* GLOBALS */

typedef ClRcT (*sendReqFnT)(void);
extern ClCorSyncStateT pCorSyncState;
extern ClCorInitStageT gCorInitStage ;

ClVersionT gCorServerToServerVersionT[] = {{'B', 0x1, 0x1}};
ClVersionDatabaseT gCorServerToServerVersionDb = {1, gCorServerToServerVersionT}; 
extern ClUint32T gCorSlaveSyncUpDone;
extern ClUint32T      gCorTxnRequestCount;
extern _ClCorServerMutexT gCorMutexes;
extern _ClCorOpDelayT   gCorOpDelay;

ClRcT sendHelloReq(void);
ClRcT sendConfigDataReq(void);
ClRcT sendDmDataReq(void);
ClRcT sendCorListReq(void);
ClRcT sendMoTreeReq(void);
ClRcT sendObjectTreeReq(void);
ClRcT sendRmDataReq(void);
ClRcT sendSlotToMoIdMapTableReq(void);
ClRcT sendNiTableReq(void);

sendReqFnT reqFuncTable[] =
{
        sendHelloReq,
	sendConfigDataReq,
	sendDmDataReq, 
        sendCorListReq,      
	sendMoTreeReq, 
	sendObjectTreeReq,
	sendRmDataReq,
        sendNiTableReq
	/*sendSlotToMoIdMapTableReq */   /* this needs to be broadcasted....everytime a slot info is filled*/
};

/* This is a top level function which shall be called to initiate COR Sync
 *
 *  This API shall be called when the slave decides that it needs to
 *  get all his data from the Master. Slave can obtain its data from 
 *  master or from its own persistent DB or can rebuild it from config.
 *                                                                        
 *  NO NEED TO CHANGE THIS FUNCTION WHEN ADDITIONAL DATA
 *  TYPES ARE ADDED.
 *
 *  @param    None
 *
 *  @returns CL_OK  - Success<br>
 *           
 */
ClRcT synchronizeWithMaster()
{
	ClRcT rc = CL_OK;
	ClInt8T i = 0;
	ClUint8T numFunctions = sizeof(reqFuncTable)/sizeof(sendReqFnT);
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
	
    gettimeofday(&delay[0], NULL);
#endif

	/* Go through the list of data that needs to be obtained from the master */
	for(i=0; i< numFunctions; i++)
	{
		/* send request for given datatype */
  	   rc = reqFuncTable[i]();
        /**
         *  If master is sending this error that means the active COR is
         *  processing the transaction requests so synup should retry till
         *  it is done with that.
        */
        if(CL_OK != rc)
        {
            if(CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN) == rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("COR Server Active is not in a position to sync-up. So retrying.. "));
                i = i - 1;
                usleep(CL_COR_SYNC_UP_RETRY_TIME);
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while synchronizing with the master. rc[0x%x], index %d", rc, i));
                return rc;
            }
        } 
	}
#ifdef CL_COR_MEASURE_DELAYS
    gettimeofday(&delay[1], NULL);
    gCorOpDelay.corStandbySyncup = (delay[1].tv_sec * 1000000 + delay[1].tv_usec)  - 
                                        (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
    	clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
	pCorSyncState = CL_COR_SYNC_STATE_COMPLETED;
    	clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
	return rc;
}

/**
 *  Send request to Master.
 *
 *  This API shall send request to Master and get data from Master.
 *  The data is unpacked in this function. If any operation fails,
 *  appropriate error code is returned.
 *                                                                        
 *  @param    None
 *
 *  @returns CL_OK  - Success<br>
 *           
 */
        

ClRcT sendHelloReq(void)
{
        ClRcT rc = CL_OK;
	corSyncPkt_t* pkt = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
    ClInt32T tries = 0;
    ClTimerTimeOutT timeout = {.tsSec = 1, .tsMilliSec = 0};

	pktSize = sizeof(corSyncPkt_t);
	
	pkt = (corSyncPkt_t*)clHeapAllocate(pktSize);
	if(pkt == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/*First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pkt, COR_SYNC_HELLO_REQUEST);

	/* Now send the packet to the destination */
    while(tries++ < CL_COR_SERVER_SYNC_RETRIES)
    {
        SEND_PKT_TO_MASTER(inMessageHandle,
                           VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0),
                           outMessageHandle,
                           pkt,
                           pktSize,
                           rc);
        if(CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_COMP_UNREACHABLE)
            break;
        else
        {
            clLogInfo("SYN", "REQ", "COR service not up. Retrying after 1 sec.");
            clOsalTaskDelay(timeout); 
            clBufferDelete(&inMessageHandle);
            clBufferDelete(&outMessageHandle);
        }	
    }

    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_1_HELLO_REQUEST, rc);
    }
    clHeapFree(pkt);

	/* Packet has been sent to the destination and if we are here, we would have got reply from
	      destination */	

	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);
	
	return (rc);
}

ClRcT sendConfigDataReq()
{

	return CL_OK;
}

ClRcT sendDmDataReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
    corSyncPkt_t *pSyncPktRcvd = NULL;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif

	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
	    clLogError("SYN", "DMU", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/*First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_DM_DATA_REQUEST);

	/* Now send the packet to the destination */
	SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);
    clHeapFree(pSyncPktSend);

    if(rc == CL_OK)
    {
       
        /*We would have got the packed DM data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "DMU", "Failed while allocating the sync packet for DM unpack");
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "DMU", "Failed to Unmarshall DM DATA");
        }
        else if(pSyncPktRcvd->dlen == 0)
        {
            clLogError("SYN", "DMU", "The data length of the dm data received from active COR is zero.");
        }
        else
        { 
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[0], NULL);
#endif
            if((rc = dmClassesUnpack((pSyncPktRcvd->data), 
                                       pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError( "SYN", "DMU", CL_LOG_MESSAGE_2_UNPACK, "dmClass", rc);
            }
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[1], NULL);
            gCorOpDelay.dmDataUnpack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
        }

        if(pSyncPktRcvd != NULL)
        {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
        }	
    } 
    else
    {
        clLogError("SYN", "DMU", CL_LOG_MESSAGE_2_SYNC, "sendDmDataReq", rc);
    } 
	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);
	return (rc);
}

ClRcT sendCorListReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;

	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
	    clLogError("SYN", "CLT", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/*First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_COR_LIST_REQUEST);

	/* Now send the packet to the destination */
	SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);
    clHeapFree(pSyncPktSend);

    if(rc == CL_OK)
    {
        corSyncPkt_t *pSyncPktRcvd = NULL;
        /*We would have got the packed corList data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "CLT", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "CLT", "Failed to Unmarshall DM DATA");
        }
        else 
        {
            if((rc = corListUnpack((pSyncPktRcvd->data), pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError("SYN", "CLT", CL_LOG_MESSAGE_2_UNPACK, "corList", rc);
            }
        }
        if(pSyncPktRcvd != NULL)
        {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
        }	
    }
    else
    {
        clLogError("SYN", "CLT", CL_LOG_MESSAGE_2_SYNC, "sendCorListReq", rc);
    } 

	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);
	return (rc);
}


ClRcT sendMoTreeReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
    corSyncPkt_t* pSyncPktRcvd = NULL;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif

	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
        clLogError("SYN", "MTU", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/*First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_MOTREE_REQUEST);

	/* Now send the packet to the destination */
	SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);
     clHeapFree(pSyncPktSend);

    if(rc == CL_OK)
    {
        /*We would have got the packed MOClassTree data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "MTU", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "MTU", "Failed to Unmarshall DM DATA");
        }
        /* Unpack the MOClassTree */
        else if(pSyncPktRcvd->dlen == 0)
        { 
            clLogError("SYN", "MTU", "The size of the MO Tree buffer obtained from the active COR is zero. ");
        }
        else 
        {
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[0], NULL);
#endif
            if((rc = corMOTreeUnpack(NULL,(pSyncPktRcvd->data),
                              (ClUint32T *)&pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError("SYN", "MTU", CL_LOG_MESSAGE_2_UNPACK, "corMOTree", rc);
            }
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[1], NULL);
            gCorOpDelay.moClassTreeUnpack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
        } 
        if(pSyncPktRcvd != NULL)
        {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
        }	
    }
    else
    {
        clLogError("SYN", "MTU", CL_LOG_MESSAGE_2_SYNC, "sendMoTreeReq", rc);
    } 

	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);
	return (rc);
}

ClRcT sendObjectTreeReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
    corSyncPkt_t* pSyncPktRcvd = NULL;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif


	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/* First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_OBJECT_TREE_REQUEST);

	/* Now send the packet to the destination */
	SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);
    clHeapFree(pSyncPktSend);

    if(rc == CL_OK)
    {
        /* We would have got the packed ObjTree data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "OBU", "Failed while allocating memory for the response obtained from active COR. ");
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "OBU", "Failed while unmarshalling the COR packet from COR server active. rc[0x%x]", rc);
        }
        /* Unpack the ObjTree */
        else if(pSyncPktRcvd->dlen == 0)
        {
            clLogError("SYN", "OBU", "The object packed data obtained from active COR server is of zero length.");
        }
        else 
        {
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[0], NULL);
#endif
            if((rc = corObjTreeUnpack(NULL, (pSyncPktRcvd->data),
                              (ClUint32T *)&pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError("SYN", "OBU", CL_LOG_MESSAGE_2_UNPACK, "corObjTree", rc);
            }
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[1], NULL);
            gCorOpDelay.objTreeUnpack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
        }
        if(pSyncPktRcvd != NULL)
        {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
        }	
    }
    else
    {
        clLogError("SYN", "OBU", CL_LOG_MESSAGE_2_SYNC, "sendObjectTreeReq", rc);
    } 
	
	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);

	return (rc);
}

ClRcT sendRmDataReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
	corSyncPkt_t* pSyncPktRcvd = NULL;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif

	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/* First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_RM_DATA_REQUEST);

	/* Now send the packet to the destination */
	SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);

    clHeapFree(pSyncPktSend);      

    if(rc == CL_OK)
    {
        /* We would have got the packed RM data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "RMU", "Failed while allocating the buffer to unpack the data sent by COR active.");
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "RMU", "Failed to Unmarshalling the buffer sent by COR active. rc[0x%x]", rc);
        }
      /* Unpack the ObjTree */
       else if(pSyncPktRcvd->dlen == 0)
       {
           clLogError("SYN", "RMU", "The data sent by active COR for the route list packing request has zero length");
       }
       else 
       {
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[0], NULL);
#endif
            if((rc = rmRouteUnpack((void *)(pSyncPktRcvd->data),
                                            pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError("SYN", "RMU", CL_LOG_MESSAGE_2_UNPACK, "rmRoute", rc); 
            }
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[1], NULL);
            gCorOpDelay.routeListUnpack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
       }

       if(pSyncPktRcvd != NULL)
       {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
       }	
   }	
   else
   {
        clLogError("SYN", "RMU", CL_LOG_MESSAGE_2_SYNC, "sendRmDataReq", rc);
   } 
	clBufferDelete(&inMessageHandle);
	clBufferDelete(&outMessageHandle);
	return rc;
}

ClRcT sendNiTableReq()
{
    ClRcT rc = CL_OK;
	corSyncPkt_t* pSyncPktSend = NULL;
	ClBufferHandleT inMessageHandle = 0;
	ClBufferHandleT outMessageHandle = 0;
	ClUint32T pktSize = 0;
    corSyncPkt_t* pSyncPktRcvd = NULL;
#ifdef CL_COR_MEASURE_DELAYS
    struct timeval delay[2] = {{0}};
#endif

	pktSize = sizeof(corSyncPkt_t);
	
	pSyncPktSend = clHeapAllocate(pktSize);
	if(pSyncPktSend == NULL)
	{
		clLogError("SYN", "NIU", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
		return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	}

	/* First Construct the packet. i.e. Fill in the common attribute in the packet */
	CONSTRUCT_COR_COMM_PKT(pSyncPktSend, COR_SYNC_NI_TABLE_REQUEST);

    /* Now send the packet to the destination */
    SEND_PKT_TO_MASTER(inMessageHandle, clXdrMarshallcorSyncPkt_t, outMessageHandle, pSyncPktSend, pktSize, rc);

    clHeapFree(pSyncPktSend);      

    if(rc == CL_OK)
    {

        /* We would have got the packed RM data in the outMsg */
        pSyncPktRcvd = clHeapAllocate(sizeof(corSyncPkt_t)); 
        if(NULL == pSyncPktRcvd)
        {
            clLogError("SYN", "NIU", "Failed while allocating memory for sync packet for NI Table. ");
        }
        else if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(outMessageHandle, (void *)pSyncPktRcvd)))
        {
            clLogError("SYN", "NIU", "Failed while unmarshalling the \
                    Name Interface table marshalled data obtained from active COR. ");
        }
        /* Unpack the ObjTree */
        else if(pSyncPktRcvd->dlen == 0)
        {
            clLogError("SYN", "NIU", "The CORs Name Interface table data obtained has zero length. ");
        }
        else 
        {
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[0], NULL);
#endif
            if((rc = corNiTableUnPack((void *)(pSyncPktRcvd->data),
                                            pSyncPktRcvd->dlen)) != CL_OK)
            {
                clLogError("SYN", "NIU",  CL_LOG_MESSAGE_2_UNPACK, "corNiTable", rc);
            }
#ifdef CL_COR_MEASURE_DELAYS
            gettimeofday(&delay[1], NULL);
            gCorOpDelay.niTableUnpack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
        }
        
        if(pSyncPktRcvd != NULL)
        {
            clHeapFree(pSyncPktRcvd->data);
            clHeapFree(pSyncPktRcvd);
        }	
    }	
    else
    {
        clLogError("SYN", "NIU", CL_LOG_MESSAGE_2_SYNC, "sendNiTableReq", rc);
    } 

    clBufferDelete(&inMessageHandle);
    clBufferDelete(&outMessageHandle);
	return rc;
}

ClRcT sendSlotToMoIdMapTableReq()
{
	return CL_OK;
}



/* RMD function to process the sync requests for different data types.*/
ClRcT  VDECL(_corSyncRequestProcess) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{

   ClRcT rc = CL_OK;
   corSyncPkt_t *pPkt = NULL ;
   corSyncPkt_t *pSyncPkt = NULL;
   ClUint32T    size = 0;
#ifdef CL_COR_MEASURE_DELAYS
   struct timeval delay[2] = {{0}};
#endif

   if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
   {
       clLogError("SYN", "REQ", "The COR server Initialization is in progress....");
       return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
   }

   CL_FUNC_ENTER();

	/* Need to put the lock here */
	clOsalMutexLock(gCorMutexes.gCorServerMutex);

    pPkt = clHeapAllocate(sizeof(corSyncPkt_t)); 
    if (pPkt == NULL)
    {
			 clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("malloc failed"));
             return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
   
	if((rc = VDECL_VER(clXdrUnmarshallcorSyncPkt_t, 4, 0, 0)(inMsgHandle, (void *)pPkt)))
	{
	    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
		clHeapFree(pPkt);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corSyncPkt_t "));
		return rc;	
	}

	/* Server to Server Version verify */
  	clCorServerToServerVersionValidate(pPkt->version, rc); 
    if(rc != CL_OK)
	{
		if((rc = clXdrMarshallClVersionT((void *)&pPkt->version, outMsgHandle, 0)) != CL_OK) 
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Sending Version field back to slave :  Marshall corSyncPkt_t failed\n"));
	    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
		clHeapFree(pPkt);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pPkt->opcode)
    {
        case   COR_SYNC_HELLO_REQUEST: 
            {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","HRQ", "Got the request for the cor-list addition ");
                /**
                 *  Fix for Bug#6055 and #5714
                 *  This snippet of code set the state of the synchronisation. Also it checks whether
                 *  are there any transaction requests in progress through a global transaction request
                 *  counter. If the transaction request counter is greater than zero, then it returns an
                 *  error CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN).
                 */
		        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
			    pCorSyncState = CL_COR_SYNC_STATE_INPROGRESS;

                if(gCorTxnRequestCount > 0)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The Txn job counter is non-"
                                " zero [%d]. Retry Again......", gCorTxnRequestCount));
                    pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
                    return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
                }

                clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

                ClIocPhysicalAddressT corAdd;
                corAdd.nodeAddress = pPkt->src;
                corAdd.portId      = CL_IOC_COR_PORT;
                 
                /* Add the COR instance (which sent HELLO Message) to corList*/
                rc = corListAdd((corAdd.nodeAddress&0xFFFF), corAdd, 0);
                if(rc != CL_OK)
                {
                    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                    pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, CL_LOG_MESSAGE_1_COR_LIST_ADD, rc);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Add COR instance to corList \n"));
                }
            }
        break;
      case   COR_SYNC_CONFIG_DATA_REQUEST:
      case   COR_SYNC_DM_DATA_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","DMC", "Got the request for the Dm class sync-up ");

                    ClBufferHandleT dmBufHandle = {0};
                 
                   /*@todo:
                    *  Need to lock whole DM, before packing.
                    */
             
                    pSyncPkt = clHeapAllocate(sizeof(corSyncPkt_t));
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                    memset(pSyncPkt, 0, sizeof(corSyncPkt_t));
                  
                    /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_DM_DATA_REQUEST_REPLY);
                
                    rc = clBufferCreate(&dmBufHandle);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the buffer message. rc[0x%x]", rc));
                        goto handleError;
                    }
                    
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[0], NULL);
#endif
                    /* Pack the DM class info now.*/
                    rc = dmClassesPack(&dmBufHandle);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&dmBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the dmClass information. rc[0x%x]", rc));
                        goto handleError;
                    }
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[1], NULL);
                    gCorOpDelay.dmDataUnpack = 0;
                    gCorOpDelay.dmDataPack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

                    rc = clBufferLengthGet(dmBufHandle, &pSyncPkt->dlen);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&dmBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the length of the dm class buffer. rc[0x%x]", rc));
                        goto handleError;
                    }

                    clLog(CL_LOG_SEV_INFO, "SYN","DMC","The dm class buffer is of size [%d]", pSyncPkt->dlen);
                            
                    rc = clBufferFlatten(dmBufHandle, &pSyncPkt->data);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&dmBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flatten of the buffer. rc[0x%x]", rc));
                        goto handleError;
                    }
                    
                    /* Write to Buffer */
				    if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)) != CL_OK) 
				    { 
					    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
					    pCorSyncState = CL_COR_SYNC_STATE_INVALID;
					    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
					    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t \n"));
                    }
                    
                    clHeapFree(pSyncPkt->data);
                    clHeapFree(pSyncPkt);
                    clBufferDelete(&dmBufHandle);
                    clLog(CL_LOG_SEV_DEBUG, "SYN","DMC", "Compeleted the dm class packing");
               } 
               break;
      case   COR_SYNC_COR_LIST_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","CLT", "Got the request for the cor-list sync-up ");

                    ClUint32T corListLen = 0;
                    size = sizeof(corSyncPkt_t) + COR_LIST_BLOCK_SIZE; 
                 
                    pSyncPkt = clHeapAllocate(size);
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                    memset(pSyncPkt, 0, size);
                  
                   /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_COR_LIST_REQUEST_REPLY);
                
                   /* Pack the corList now.*/
                    pSyncPkt->data = (ClUint8T *)(&pSyncPkt->data) + sizeof(pSyncPkt->data);
                    corListLen = corListPack((pSyncPkt->data));

                   /* Update dm data size */
                    pSyncPkt->dlen = corListLen;
                   
                   /* Write to Buffer */
					if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)))
					{
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                        pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t \n"));
					}
                    clHeapFree(pSyncPkt);
               }
               break;
      case   COR_SYNC_MOTREE_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","MOT", "Got the request for the class-tree sync-up ");

                    ClUint32T moClassBufferSize = 0;
                    ClBufferHandleT    moTreeBuffer = 0;
                 
                   /*@todo:
                    * 1) Need to lock whole MOClassTree, before packing.
                    */
                    pSyncPkt = clHeapAllocate(sizeof(corSyncPkt_t));
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					            CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                    memset(pSyncPkt, 0, sizeof(corSyncPkt_t));
                  
                   /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_MOTREE_REQUEST_REPLY);
                
                    rc = clBufferCreate(&moTreeBuffer);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the moTree buffer. rc[0x%x]", rc));
                        goto handleError;
                    }

#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[0], NULL);
#endif
                   /* Pack the MOClass tree now*/
                    rc = corMOTreePack(NULL, &moTreeBuffer);
                    if(rc != CL_OK)
                     {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&moTreeBuffer);
		       	        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, CL_LOG_MESSAGE_2_TREE_PACK, "MOClass", rc);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not pack the MOClassTree with error 0x%x",rc) );
                        goto handleError;
                     }
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[1], NULL);
                    gCorOpDelay.moClassTreeUnpack = 0;
                    gCorOpDelay.moClassTreePack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif

                    rc = clBufferLengthGet(moTreeBuffer, &moClassBufferSize);
                    if(CL_OK != rc)
                    {
                       clHeapFree(pSyncPkt);
                       clBufferDelete(&moTreeBuffer);
                       CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the moclass buffer. rc"));
                       goto handleError;
                    }

                    rc = clBufferFlatten(moTreeBuffer, &pSyncPkt->data);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&moTreeBuffer);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattenning of the moclass-tree data. rc[0x%x]", rc));
                        goto handleError;
                    }

                    clLog(CL_LOG_SEV_INFO, "SYN","MOT", "Packing the MO-Class Tree: BufferSize [%d]", moClassBufferSize);
                    
                   /* Update data size */
                    pSyncPkt->dlen = moClassBufferSize;
                
                   /* Write to Buffer */
					if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)))
					{
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                            pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t \n"));
					}

                    clHeapFree(pSyncPkt->data);
                    clBufferDelete(&moTreeBuffer);
                    clHeapFree(pSyncPkt);
                    
                    clLog(CL_LOG_SEV_DEBUG, "SYN","MOT", "Completed the processing of class-tree packing ");
               }
               break;
     case   COR_SYNC_OBJECT_TREE_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","OBT", "Got the request for the object-tree sync-up ");

                    ClUint32T objBufferSize = 0;
                    ClBufferHandleT objTreeBuffer = 0;
 
                   /*@todo:
                    * 1) Need to lock whole ObjectTree (and dm), before packing.
                   */

                    pSyncPkt = clHeapAllocate(sizeof(corSyncPkt_t));
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					        CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                    memset(pSyncPkt, 0, sizeof(corSyncPkt_t));
                  
                   /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_OBJECT_TREE_REQUEST_REPLY);
                
                   /* Pack the Object Tree  now*/
                
                    rc = clBufferCreate(&objTreeBuffer);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the object pack buffer . rc[0x%x]", rc));
                        goto handleError;
                    }

#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[0], NULL);
#endif
                    rc = corObjTreePack(NULL, CL_COR_OBJ_FLAGS_ALL, &objTreeBuffer);
                    if(rc != CL_OK)
                     {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&objTreeBuffer);
		       	        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, CL_LOG_MESSAGE_2_TREE_PACK, "Object", rc);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not pack the Object Tree with error. rc [0x%x]",rc) );
                        goto handleError;
                     }
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[1], NULL);
                    gCorOpDelay.objTreeUnpack = 0;
                    gCorOpDelay.objTreePack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
                    rc = clBufferLengthGet(objTreeBuffer, &objBufferSize);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&objTreeBuffer);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the \
                                        size of the Obj Tree buffer. rc [0x%x]", rc));
                        goto handleError;
                    }

                    rc = clBufferFlatten(objTreeBuffer, &pSyncPkt->data); 
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&objTreeBuffer);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the flatten.. rc [0x%x]", rc));
                        goto handleError;
                    }

                    clLog(CL_LOG_SEV_INFO, "SYN","OBT", "Packing the Object Tree: BufferSize [%d]", objBufferSize);

                   /* Update data size */
                    pSyncPkt->dlen = objBufferSize;
                
                   /* Write to Buffer */
					if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)) != CL_OK)
					{
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                        pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t rc[0x%x]\n", rc));
					}

                    clHeapFree(pSyncPkt->data);
                    clBufferDelete(&objTreeBuffer);
                    clHeapFree(pSyncPkt);
                    
                    clLog(CL_LOG_SEV_DEBUG, "SYN","OBT", "Completed the processing for the object-tree packing ");
               }
               break;
      case   COR_SYNC_RM_DATA_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","RLT", "Got the request for the route list sync-up ");

                    ClUint32T rmBufferSize = 0;
                    ClBufferHandleT  routeBufHandle = 0;
 
                   /*@todo:
                    * 1) Need to lock whole RM, before packing.
                   */

                    pSyncPkt = clHeapAllocate(sizeof(corSyncPkt_t));
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc = (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                    memset(pSyncPkt, 0, sizeof(corSyncPkt_t));
                  
                    rc = clBufferCreate(&routeBufHandle);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the rm-buffer handle. rc[0x%x]", rc));
                        goto handleError;
                    }
                   /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_RM_DATA_REQUEST_REPLY);
                
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[0], NULL);
#endif
                    rc = rmRoutePack( &routeBufHandle);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&routeBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while Packing the rout list.. rc [0x%x]", rc));
                        goto handleError;
                    }
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[1], NULL);
                    gCorOpDelay.routeListUnpack = 0;
                    gCorOpDelay.routeListPack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
#endif
                    rc = clBufferLengthGet(routeBufHandle, &rmBufferSize);
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&routeBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the route list buffer. rc [0x%x]", rc));
                        goto handleError;
                    }
                    rc = clBufferFlatten(routeBufHandle, &pSyncPkt->data); 
                    if(CL_OK != rc)
                    {
                        clHeapFree(pSyncPkt);
                        clBufferDelete(&routeBufHandle);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattening the rm buffer. rc [0x%x]", rc));
                        goto handleError;
                    }
                    
                    clLog(CL_LOG_SEV_INFO, "SYN","RLT", "Packing the Route List : BufferSize [%d]", rmBufferSize);

                   /* Update data size */
                    pSyncPkt->dlen = rmBufferSize;
                
                   /* Write to Buffer */
					if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)) != CL_OK)
					{
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                          pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t. rc[0x%x] \n", rc));
					}

                    clHeapFree(pSyncPkt->data);
                    clBufferDelete(&routeBufHandle);
                    clHeapFree(pSyncPkt);
                    
                    clLog(CL_LOG_SEV_DEBUG, "SYN","RLT", "Completed the processing the route list packing ");
               }
               break;
      case   COR_SYNC_NI_TABLE_REQUEST:
               {
                    clLog(CL_LOG_SEV_DEBUG, "SYN","NIT", "Got the request for the ni-table sync-up ");

                    ClUint32T niTableSize = 0;
                    void      *packedNiTable = NULL;
 
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[0], NULL);
#endif
                    if((rc = corNiTablePack(&packedNiTable, &niTableSize))!= CL_OK)
                    {
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                            pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                        /* If could not pack due to some reason, make the packed size = 0 */
                        niTableSize = 0;
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Failure in packing NI Table .. Error 0x%x \n",rc) );
                    }
#ifdef CL_COR_MEASURE_DELAYS
                    gettimeofday(&delay[1], NULL);
                    gCorOpDelay.niTableUnpack = 0;
                    gCorOpDelay.niTablePack = (delay[1].tv_sec * 1000000 + delay[1].tv_usec) - 
                                        + (delay[0].tv_sec * 1000000 + delay[0].tv_usec);
                    gCorOpDelay.corStandbySyncup = 0;
                    gCorOpDelay.objDbRecreate = 0;
                    gCorOpDelay.routeDbRecreate = 0;
                    gCorOpDelay.corDbRecreate = 0;
#endif
                    pSyncPkt = clHeapCalloc(1,sizeof(corSyncPkt_t) + niTableSize);
                    if( NULL == pSyncPkt )
                    {
		                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					        CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clHeapAllocate failed"));
                        rc =  (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                        goto handleError;
                    }
                  
                    /* Fill the packet details.*/
                    CONSTRUCT_COR_COMM_PKT(pSyncPkt, COR_SYNC_NI_TABLE_REQUEST_REPLY);
                
                    /* Put the packed Ni table to the synched packet. */
                    pSyncPkt->data = (ClUint8T *)(&pSyncPkt->data) + sizeof(pSyncPkt->data);
                    if(niTableSize)
                        memcpy(pSyncPkt->data, packedNiTable, niTableSize);

                    /* Update data size */
                    pSyncPkt->dlen = niTableSize;
                
                    /* Write to Buffer */
					if((rc = VDECL_VER(clXdrMarshallcorSyncPkt_t, 4, 0, 0)((void *)pSyncPkt, outMsgHandle, 0)))
					{
                        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                            pCorSyncState = CL_COR_SYNC_STATE_INVALID;
                        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshall corSyncPkt_t \n"));
					}
				
					/* Master - Slave Sync Up done -- set the global variable to start run time sync up */
					if(rc == CL_OK)
						gCorSlaveSyncUpDone = CL_TRUE;

                    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
                        pCorSyncState = CL_COR_SYNC_STATE_COMPLETED;
                    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
                    clHeapFree(packedNiTable);
                    clHeapFree(pSyncPkt);
               }
               break;

      case   COR_SYNC_SLOT_TO_MOID_MAP_TABLE:
                clLog(CL_LOG_SEV_DEBUG, "SYN","SMT", "Got the request for the slot-to-moid-table sync-up ");
                break;

      case   COR_SYNC_DATA_END:
                break;

      default:
           {
                clLog(CL_LOG_SEV_ERROR, "SYN","", "Invalid SYNC operation requested.");
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid SYNC operation requested\n"));
                rc =  (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
                goto handleError;
           }
    }
    
    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
    clHeapFree(pPkt);
    CL_FUNC_EXIT();
    return (rc);

handleError:
    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
        pCorSyncState = CL_COR_SYNC_STATE_INVALID;
    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
    clHeapFree(pPkt);
    CL_FUNC_EXIT();
    return rc;
}
