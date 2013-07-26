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
 * File        : clCorRMDWrap.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * The module contains macros (wrappers) around Rmd functions.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_RMD_WRAP_H_
#define _CL_COR_RMD_WRAP_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clIocServices.h>
#include <clBufferApi.h>
#include "clCpmApi.h"
#include <clCorErrors.h>
#include <clLogApi.h>

#define COR_RMD_DFLT_RETRIES   2
#define COR_RMD_DFLT_TIME_OUT   100000 /* Default time out for any RMD call called by COR*/
#define CL_COR_TRY_AGAIN_SLEEP_TIME 100000 /* This is in microseconds */
#define CL_COR_TRY_AGAIN_MAX_RETRIES 5
#define CL_COR_MASTER_ADDRESS_GET_RETRIES 5 /* The number of retries to get the master addess */

#define CL_CPM_MASTER_ADDRESS_GET(pNodeAddress) \
do { \
	ClUint8T 	retries = 0; \
	do { \
		rc = clCpmMasterAddressGet(pNodeAddress) ; \
		if (CL_OK != rc) \
		{ \
			retries++; \
			clLogNotice("COR", "RMD", "Unable to get the master address from AMS. " \
				"rc[0x%x], Retrying [%d].", rc, retries); \
			usleep(CL_COR_TRY_AGAIN_SLEEP_TIME); \
		} \
	} while ((CL_OK != rc) &&  (retries < CL_COR_MASTER_ADDRESS_GET_RETRIES)) ; \
}while(0) 
	
#define COR_CALL_RMD_SYNC(nodeAddr, userRmdFlags, funcId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOutBuf, pOutBufLen, rc) \
do\
{\
    ClRcT (*unmarshall) (ClBufferHandleT msg , void* pGenVar) = clCorXdrUnmarshallFP;\
    ClUint8T   *pInData = (ClUint8T *)pInBuf;\
    ClUint32T   pInDataLength = pInBufLen;\
    ClUint8T   *pOutData = (ClUint8T *)pOutBuf;\
    ClUint32T  *pOutDataLength = (ClUint32T *)pOutBufLen;\
    ClRmdOptionsT      rmdOptions = CL_RMD_DEFAULT_OPTIONS;\
    ClBufferHandleT inMsgHdl = 0;\
    ClBufferHandleT outMsgHdl = 0;\
    ClIocAddressT  destAddr;\
    ClBoolT bRetry = CL_FALSE; \
    ClUint32T noOfRetries = 0; \
    ClUint32T rmdFlags = pOutData ? CL_RMD_CALL_NEED_REPLY :0;\
    \
    rmdFlags |= userRmdFlags; \
    \
    rmdOptions.timeout = COR_RMD_DFLT_TIME_OUT;\
    rmdOptions.retries = COR_RMD_DFLT_RETRIES;\
    rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;\
    \
    if(pInData && pInDataLength )\
    {\
        if((rc = clBufferCreate(&inMsgHdl))!= CL_OK)\
        {\
	        return (rc);\
        }\
	    if((rc = clCorXdrMarshallFP((void*) pInData, inMsgHdl, 0))) \
	    {\
	    	return rc;\
	    }\
    }\
    if(pOutData && pOutDataLength)\
    {\
        if((rc = clBufferCreate(&outMsgHdl))!= CL_OK)\
        {\
           return (rc);\
        }\
    }\
    \
    destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;\
    \
    do { \
        if (nodeAddr != 0) \
            destAddr.iocPhyAddress.nodeAddress = nodeAddr; \
        else \
            CL_CPM_MASTER_ADDRESS_GET(&destAddr.iocPhyAddress.nodeAddress);\
        \
        rc = clRmdWithMsg(destAddr,funcId, inMsgHdl,\
                        outMsgHdl, rmdFlags&(~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);\
        \
        bRetry = clCorHandleRetryErrors(rc); \
        if (bRetry) \
        { \
            clLogNotice("COR", CL_LOG_CONTEXT_UNSPECIFIED, "Cor Server has returned try again error. So retrying.."); \
        } \
    }while (bRetry && (noOfRetries++ < CL_COR_TRY_AGAIN_MAX_RETRIES)); \
    \
    if((rc==CL_OK)&&(pOutData && pOutDataLength)&&unmarshall)\
	    (*unmarshall)(outMsgHdl, (void *) pOutData);\
    if(pInData)\
        clBufferDelete(&inMsgHdl);\
    if(pOutData)\
        clBufferDelete(&outMsgHdl);\
}\
while(0)

#define COR_CALL_RMD_SYNC_WITH_MSG(fcnId, inMsgHdl, outMsgHdl, rc)\
do\
{\
    ClRmdOptionsT      rmdOptions = CL_RMD_DEFAULT_OPTIONS;\
    ClUint32T          rmdFlags = CL_RMD_CALL_ATMOST_ONCE; \
    rmdOptions.timeout = COR_RMD_DFLT_TIME_OUT;\
    rmdOptions.retries = COR_RMD_DFLT_RETRIES;\
    rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;\
    ClIocAddressT  destAddr;\
    ClBoolT bRetry = CL_FALSE; \
    ClUint32T noOfRetries = 0; \
    \
    if (outMsgHdl) rmdFlags |= CL_RMD_CALL_NEED_REPLY; \
    \
    destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;\
    \
    do { \
        CL_CPM_MASTER_ADDRESS_GET(&destAddr.iocPhyAddress.nodeAddress);\
        rc = clRmdWithMsg(destAddr,fcnId, inMsgHdl, outMsgHdl, rmdFlags&(~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);\
        bRetry = clCorHandleRetryErrors(rc); \
        if (bRetry) \
        { \
            clLogNotice("COR", CL_LOG_CONTEXT_UNSPECIFIED, "COR server has returned try again error. So retrying.."); \
        } \
    }while (bRetry && (noOfRetries++ < CL_COR_TRY_AGAIN_MAX_RETRIES)); \
}\
while(0)

#define COR_CALL_RMD_ASYNC_WITH_MSG(fcnId, inMsgHdl, outMsg, callbackFunc, rc)\
do\
{\
    ClUint32T          rmdFlags = CL_RMD_CALL_ASYNC|CL_RMD_CALL_NEED_REPLY|CL_RMD_CALL_NON_PERSISTENT ; \
    ClIocAddressT  destAddr = {{0}};\
    ClRmdOptionsT  rmdOptions = { CL_RMD_TIMEOUT_FOREVER, \
                                    0, CL_RMD_DEFAULT_PRIORITY, \
                                    CL_RMD_DEFAULT_TRANSPORT_HANDLE}; \
    ClRmdAsyncOptionsT rmdAsyncOpt = { NULL, callbackFunc }; \
    ClBoolT bRetry = CL_FALSE; \
    ClUint32T noOfRetries = 0; \
    destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;\
    \
    do {\
        CL_CPM_MASTER_ADDRESS_GET(&destAddr.iocPhyAddress.nodeAddress );\
        rc = clRmdWithMsg(destAddr,fcnId, inMsgHdl, outMsg, rmdFlags, &rmdOptions, &rmdAsyncOpt);\
        bRetry = clCorHandleRetryErrors(rc); \
        if (bRetry) \
        { \
            clLogNotice("COR", CL_LOG_CONTEXT_UNSPECIFIED, "Cor Server has returned try again error. So retrying.."); \
        } \
    } while (bRetry && (noOfRetries++ < CL_COR_TRY_AGAIN_MAX_RETRIES)); \
} \
while(0)

#define COR_CALL_RMD(fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)\
COR_CALL_RMD_SYNC(0, CL_RMD_CALL_ATMOST_ONCE, fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)

#define COR_CALL_RMD_WITHOUT_ATMOST_ONCE(fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)\
COR_CALL_RMD_SYNC(0, 0, fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)

#define COR_CALL_RMD_WITH_DEST(sdAddr,fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)\
COR_CALL_RMD_SYNC(sdAddr, CL_RMD_CALL_ATMOST_ONCE, fcnId, clCorXdrMarshallFP, pInBuf, pInBufLen, clCorXdrUnmarshallFP, pOpBuf, pOpBufLen, rc)

#ifdef __cplusplus
}
#endif


#endif /* _CL_COR_RMD_WRAP_H_ */
