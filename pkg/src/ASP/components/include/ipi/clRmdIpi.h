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
 * ModuleName  : include                                                       
 * File        : clRmdIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This header defines the RMD Library Interface.
 *
 *
 *****************************************************************************/

#ifndef _CL_RMD_IPI_H_
# define _CL_RMD_IPI_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <clEoApi.h>
#include <clIocApiExt.h>

/**
 * 0x0040 If you want the RMD requests to be delivered in order.
 */
# define CL_RMD_CALL_ORDERED          (1<<6)

/**
 * This structure holds the various statistics for calls.
 */
    typedef struct ClRmdStats
    {

/**
 * Number of calls made.
 */
        ClUint32T nRmdCalls;
/**
 * Number of failed calls made.
 */
        ClUint32T nFailedCalls;
/**
 * Number of times requests had to be resent.
 */
        ClUint32T nResendRequests;
        ClUint32T nFailedResendRequests;

/**
 * Number of replies.
 */
        ClUint32T nRmdReplies;
/**
 * Number of erroneous replies.
 */
        ClUint32T nBadReplies;
/**
 * Number of requests.
 */
        ClUint32T nRmdRequests;
/**
 * Number of bad requests.
 */
        ClUint32T nBadRequests;
/**
 * Number of timeouts.
 */
        ClUint32T nCallTimeouts;
/**
 * Number of duplicate requests.
 */
        ClUint32T nDupRequests;
/**
 * Number of calls made with atmost once semantics.
 */
        ClUint32T nAtmostOnceCalls;
        ClUint32T nFailedAtmostOnceCalls;
/**
 * Number of replies sent.
 */
        ClUint32T nReplySend;
        ClUint32T nFailedReplySend;
/**
 * Number of replies resent.
 */
        ClUint32T nResendReplies;
        ClUint32T nFailedResendReplies;
/**
 * Number of calls optimized.
 */
        ClUint32T nRmdCallOptimized;

/**
 * Number of atmost once acks sent.
 */
        ClUint32T nAtmostOnceAcksSent;
        ClUint32T nFailedAtmostOnceAcksSent;

/**
 * Number of atmost once acks received.
 */
        ClUint32T nAtmostOnceAcksRecvd;
        ClUint32T nFailedAtmostOnceAcksRecvd;
        
    } ClRmdStatsT;


/****************** Extern Declarations ******************/

typedef ClHandleT ClRmdResponseContextHandleT;

extern ClRcT clRmdVersionVerify(ClVersionT* pVersion);

extern ClRcT clRmdResponseDefer(ClRmdResponseContextHandleT *pResponseCntxtHdl);

extern ClRcT clRmdSyncResponseSend(ClRmdResponseContextHandleT syncSendCtnxtHdl,
                                  ClBufferHandleT replyMsg,
                                  ClRcT rc);

extern ClRcT clRmdSourceAddressGet(ClIocPhysicalAddressT *pSrcAddr);

extern ClRcT clRmdReceiveReply(ClEoExecutionObjT* pThis, 
            ClBufferHandleT eoRecvMsg,
            ClUint8T priority,
            ClUint8T protoType, 
            ClUint32T length, 
            ClIocPhysicalAddressT srcAddr);
extern ClRcT clRmdReceiveRequest(ClEoExecutionObjT* pThis, 
            ClBufferHandleT eoRecvMsg,
            ClUint8T priority,
            ClUint8T protoType, 
            ClUint32T length, 
            ClIocPhysicalAddressT srcAddr);
extern ClRcT clRmdReceiveAsyncRequest(ClEoExecutionObjT* pThis, 
            ClBufferHandleT eoRecvMsg,
            ClUint8T priority,
            ClUint8T protoType, 
            ClUint32T length, 
            ClIocPhysicalAddressT srcAddr);
extern ClRcT clRmdReceiveAsyncReply(ClEoExecutionObjT* pThis, 
            ClBufferHandleT rmdRecvMsg,
            ClUint8T priority,
            ClUint8T protoType, 
            ClUint32T length, 
            ClIocPhysicalAddressT srcAddr);
extern ClRcT clRmdReceiveOrderedRequest(ClEoExecutionObjT* pThis, 
            ClBufferHandleT eoRecvMsg,
            ClUint8T priority,
            ClUint8T protoType, 
            ClUint32T length, 
            ClIocPhysicalAddressT srcAddr);
extern ClRcT clRmdObjInit(ClRmdObjHandleT *p);

extern ClRcT clRmdObjClose(ClRmdObjHandleT p);


/************************ RMD APIs ***************************/

/**
 ************************************
 *  \page pageRMD101 clRmdLibInitialize
 *
 *  \par Synopsis:
 *  Initializes the RMD Library.
 *
 *  \Header File:
 *  clRmdIpi.h
 *
 *  \Library File:
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT  clRmdLibInitialize(void);
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_NO_MEMORY): On memory allocation failure.
 *  \retval CL_RMD_RC(CL_ERR_INITIALIZED): If the RMD library is already initialized.
 *
 *  \par Description:
 *  This API is used to initialize the RMD library with default values 
 *  if initialization has not happened through configuration. Before calling 
 *  any of the RMD APIs, this API must be called to start the initialization. 
 *
 *  \par RelatedAPIs:
 *
 *
 */

    ClRcT clRmdLibInitialize(ClPtrT config);


/*****************************************************************************/

/**
 ************************************
 *  \page pageRMD102 clRmdLibFinalize
 *
 *  \par Synopsis:
 *  Cleans up the RMD Library.
 *
 *  \Header File:
 *  clRmdIpi.h
 *
 *  \Library File:
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clRmdLibFinalize(void);
 *  \endcode
 *
 *  \par Parameters: 
 *  None
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_NOT_INITIALIZED): If RMD is not initialized.
 *
 *  \par Description:
 *  This API is used to clean up the RMD library. It must be invoked typically 
 *  during the system shutdown process or if the RMD library is no longer used.
 *
 *  \par RelatedAPIs:
 *
 */

    ClRcT clRmdLibFinalize(void);

/*****************************************************************************/
/**
 ************************************
 *  \page pageRMD104 clRmdStatsReset
 *
 *  \par Synopsis:
 *  Resets the statistics of RMD Library.
 *
 *  \Header File:
 *  clRmdApi.h
 *
 *  \Library File:
 *
 *
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clRmdStatsReset(void);
 *  \endcode
 *
 *  \par Parameters: 
 *  None
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_NULL_POINTER): On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to reset the statistics counters of RMD library.
 *
 *  \par RelatedAPIs:
 *
 *  clRmdStatsGet()
 *
 */
    ClRcT clRmdStatsReset(void);

/**
 ************************************
 *  \page pageRMD105 clRmdStatsGet
 *
 *  \par Synopsis:
 *  Returns the current statistics of RMD Library.
 *
 *  \Header File:
 *  clRmdApi.h
 *
 *  \Library File:
 *
 *  
 *  \par Syntax:
 *  \code 	ClRcT clRmdStatsGet(
 * 				ClRmdStatsT *pStats);
 *  \endcode
 *
 *  \param pStats: (out) Stats filled on return from function.
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_INVALID_PARAMETER): On passing an invalid argument.
 *
 *  \par Description:
 *  This API is used to retrieve the current statistics of the RMD library.
 *
 *  \par RelatedAPIs:
 *
 *  clRmdStatsReset()
 *
 */

    ClRcT clRmdStatsGet(CL_OUT ClRmdStatsT *pStats);


/**
 ************************************
 *  \page pageRMD106 clRmdMaxPayloadSizeGet
 *
 *  \par Synopsis:
 *  Returns the maximum size of RMD payload.
 *
 *  \Header File:
 *  clRmdApi.h
 *
 *  \Library File:
 *  
 *
 *  \par Syntax:
 *  \code 	ClRcT clRmdMaxPayloadSizeGet(
 * 				ClUint32T  *pSize);
 *  \endcode
 *
 *  \param pSize: (out) Pointer to the maximum payload size.
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_NULL_POINTER): On passing a NULL pointer.
 *
 *
 *  \par Description:
 *  This API is used to retrieve the maximum size of the RMD payload that
 *  can be transferred.
 *
 *  \par RelatedAPIs:
 *  None
 * 
 *
 */

    ClRcT clRmdMaxPayloadSizeGet(CL_OUT ClUint32T *pSize);


/**
 ************************************
 *  \page pageRMD107 clRmdMaxNumbOfRetriesGet
 *
 *  \par Synopsis:
 *  Returns Maximum number of retries.
 *
 *  \Header File:
 *  clRmdApi.h
 *
 *  \Library File:
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clRmdMaxNumbOfRetriesGet(
 * 				ClUint32T  *pNumbOfRetries);
 *  \endcode
 *
 *  \param pNumbOfRetries: (out) Maximum number of retries.
 *
 *  \retval CL_OK: The API executed successfully
 *  \retval CL_RMD_RC(CL_ERR_NULL_POINTER): On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to retrieve maximum number of retries that 
 *  has been set at the time of configuration.
 *
 *  \par RelatedAPIs:
 *  None
 *
 *
 */

    ClRcT clRmdMaxNumbOfRetriesGet(CL_OUT ClUint32T *pNumbOfRetries);

/**
 ************************************
 *  \page pageRMD108 clRmdDumpPacketStatus
 *
 *  \par Synopsis:
 *  Displays packet information.
 *
 *  \Header File:
 *  clRmdApi.h
 *
 *  \Library File:
 *
 *
 *  \par Syntax:
 *  \code 	ClRcT clRmdDumpPacketStatus(
 * 				ClUint32T isEnable);
 *  \endcode
 *
 *  \param isEnable: It can have the following values:
 *  \arg \e non-zero: If set to a non-zero value, dumping of packets by RMD is \e enabled.
 *  \arg \e 0: If set to zero, dumping of packets by RMD is \e disabled.
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \par Description:
 *  This API is used to enable the display of information about the 
 *  packets RMD receives or sends.
 *
 *  \par RelatedAPIs:
 *  None
 *
 *
 */
    ClRcT clRmdDumpPacketStatus(CL_IN ClUint32T isEnable);

ClRcT clRmdAckReply(ClEoExecutionObjT *pThis,
                        ClBufferHandleT rmdRecvMsg, ClUint8T priority,
                        ClUint8T protoType, ClUint32T length,
                        ClIocPhysicalAddressT srcAddr);



ClRcT clRmdDatabaseCleanup(ClRmdObjHandleT rmdObj, ClIocNotificationT *notification);

ClRcT rmdMetricUpdate(ClRcT status, ClUint32T clientId, ClUint32T funcId, ClBufferHandleT message, ClBoolT response);

ClRcT rmdMetricInitialize(void);

ClRcT rmdMetricFinalize(void);

/*************************************************************/

# ifdef __cplusplus
}
# endif


#endif                          /* _CL_RMD_IPI_H_ */

