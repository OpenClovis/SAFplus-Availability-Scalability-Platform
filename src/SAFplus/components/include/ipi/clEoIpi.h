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
 * ModuleName  : include
 * File        : clEoIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This header file contains the internal macros and data types used by
 *  the EO library.
 *
 ****************************************************************************/

/* This file is only for internal consumption. within EO and ASP */

#ifndef _CL_EO_IPI_H_
#define _CL_EO_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>
#include <clBufferApi.h>
#include <clIocApiExt.h>

/* Default service RMD FN#*/
#define CL_EO_DEFAULT_PRIORITY_SET		\
			CL_EO_GET_FULL_FN_NUM(CL_EO_DEFAULT_SERVICE_TABLE_ID,1)
/**
 * This is the EO Handle for EO IocPort conversion.
 */
#define  CL_EO_HANDLE_TO_PORT(eoHandle) (((ClEoExecutionObjT*)(eoHandle))->eoPort)

/**
 * Callers check for validity (e.g.,NULL) of the handle before calling.
 */
#define  CL_EO_HANDLE_TO_ID(eoHandle)   (((ClEoExecutionObjT*)(eoHandle))->eoID)
/**
 * Indicates starting service/function number.
 */
#define CL_EO_BASE_SERVICE_ID  0      
#define CL_EO_BASE_CLIENT_ID   0



/*  EO Service Object (function table) related defines */

/* FIXME: Why do we have this limit. logClient is using this, 
which he shouldn't */
#define CL_EO_MAX_NO_EOS         		    64 

#define CL_EO_GET_FN_NUM(fullFn)        (((fullFn) & CL_EO_FN_MASK) >> CL_EO_FN_BIT_SHIFT)

#define CL_EO_CLIENT_MASK               (~CL_EO_FN_MASK)
#define CL_EO_GET_CLIENT_NUM(fullFn)    (((fullFn) & CL_EO_CLIENT_MASK) \
										    >> CL_EO_CLIENT_BIT_SHIFT)


/**
 * EO client ID enum.
 * Client ID determines the location/tableIdx where the function related to 
 * the clients are installed.
 *
 * Clovis defined  EO Client IDs fall in the range 0 - 8
 * User defined EO Client IDs start from cl_EO_CLOVIS_RESERVED_CLIENTID_END + 1
 */


/* Eo Type, required for SAF related operations */
typedef enum ClEoType {
    /* Maneged Eo are those by using which we can control the life cycle
     * of the component */
    CL_EO_MANAGED,
    /* All other EO exist in the system are local */
    CL_EO_UNMANAGED,
}ClEoTypeT;

typedef struct ClEoSerialize {
    ClBufferHandleT msg;
    ClIocRecvParamT *pMsgParam;
}ClEoSerializeT;

/*****************************************************************************/
/******************************** EO APIs ************************************/
/*****************************************************************************/
/*                                                                           */
/* pageeo201 : clEoLibInitialize                                             */
/* pageeo202 : clEoLibFinalize                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  Functions
 *****************************************************************************/


/**
 ************************************
 *  \page pageeo201 clEoLibInitialize
 *
 *  \par Synopsis:
 *  Initializes the EO library.
 *
 *  \par Header File:
 *  clEoConfigApi.h
 *
 *  \par Library Files:
 *  libClEo
 *
 *  \par Syntax:
 *  \code     ClRcT clEoLibInitialize();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \par Description:
 *  This API is used to initialize the EO library. This API
 *  basically creates a list which contains the mapping of
 *  EO port to EO objects.
 *
 *  \par Related APIs:
 *
 *  clEoLibFinalize()
 *
 */
extern ClRcT clEoLibInitialize();

/**
 ************************************
 *  \page pageeo202 clEoLibFinalize
 *
 *  \par Synopsis:
 *  Cleans up the EO component.
 *
 *  \par Header File:
 *  clEoConfigApi.h
 *
 *  \par Library Files:
 *  libClEo
 *
 *  \par Syntax:
 *  \code     ClRcT clEoLibFinalize();
 *  \endcode
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \par Description:
 *  This API is used for EO component cleanup function.
 *
 *  \par Related APIs:
 *
 *  clEoLibInitialize()
 *
 */

extern ClRcT clEoLibFinalize();


extern ClRcT clEoUnblock(ClEoExecutionObjT *pThis);

/* 
 *  NAME: clEoServiceIndexGet
 *
 *  This function translates the input into appropriate table no and
 *  function no
 *
 *  @param    func[IN]     : Function Number
 *            pClientID[OUT]: table in which to search for
 *            pFuncNo[OUT]  : id of function to be called
 *
 *  @returns 
 */
ClRcT clEoServiceIndexGet(CL_IN ClUint32T func, 
			CL_OUT ClUint32T *pClientID, 
			CL_OUT ClUint32T *pFuncNo);


/* 
 *  NAME: clEoMaximumNumberOfClientsGet
 *
 *  This function is for getting the user configured EO_MAX_NO_CLIENTS
 *
 *  @param    none
 *
 *  @returns max no. of clients that can be associated with an EO
 */
ClUint32T clEoMaximumNumberOfClientsGet(); 

#if 0
/**
 ************************************
 *  \page pageEO122 clEOCookieIdGet
 *
 *  \par Synopsis:
 *  Returns Server Cookie ID.
 *
 *  \par Description:
 *  This API is used to return the Server Cookie Id.
 *
 *  \par Syntax:
 *  \code     extern ClUint32T clEOCookieIdGet(
 *                 CL_IN ClUint32T clientId);
 *  \endcode
 *
 *  \param clientId: ClientId of the service, for which cookie Id is required.
 *
 *  \par Return values:
 *  Service cookieId.
 *
 */


extern __inline__ ClUint32T clEOCookieIdGet(CL_IN ClUint32T clientId);
#endif



/***********************************************************************/
/*                      CLI/DEBUG commands                             */
/***********************************************************************/
#if 0
/**
 ************************************
 *  \page pageEO123 clEoRmdExecute
 *
 *  \par Synopsis:
 *  Executes an RMD command.
 *
 *  \par Description:
 *  This API is used to execute an RMD command on a particular EO.
 *
 *  \par Syntax:
 *  \code     extern ClRcT clEoRmdExecute(
 *                 CL_IN ClIocNodeAddressT omAddress,
 *                 CL_IN ClIocPortT eoPort,
 *                 CL_IN ClUint32T rmdNo,
 *                 CL_IN ClUint32T inputArg1, 
 *                 CL_IN ClUint32T inputArg2);
 *  \endcode
 *
 *  \param omAddress: OMAddress of the EO.
 *  \param eoPort: Port of the EO where the RMD will be executed.
 *  \param rmdNo: RMD to be invoked.
 *  \param inputArg1: First input argument to the RMD call.
 *  \param inputArg2: Second input argument to the RMD call.
 *
 *  \retval CL_OK: The API executed successfully.
 *
 */


extern ClRcT clEoRmdExecute(CL_IN ClIocNodeAddressT omAddress, 
        CL_IN ClIocPortT    eoPort, 
        CL_IN ClUint32T     rmdNo,
        CL_IN ClUint32T     inputArg1, 
        CL_IN ClUint32T     inputArg2); /*clEoRmdExecute */

#endif


/**
 ************************************
 *  \page pageEO105 clEoStateSet
 *
 *  \par Synopsis:
 *  Sets the state of EO.
 * 
 *  \par Header File:
 *  clEoApi.h
 * 
 *  \par Library Files:
 *  libClEo
 *
 *  \par Syntax:
 *  \code     extern ClRcT clEoStateSet(
 *                 CL_IN ClEoExecutionObjT  *pThis,
 *                 CL_IN ClEoStateT         flags);
 *  \endcode
 *
 *  \param pThis: Handle of the EO.
 *  \param state: State to which the EO is switched.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_EO_IMPROPER_STATE: If state change requested for an invalid state.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to set the state of EO as directed by the Component Manager 
 *  and state argument. The state of the EO indicates whether the management functions
 *  are allowed on this EO, whether services provided by the EO can be used, whether
 *  the application of this EO should suspend its state and operation and whether the
 *  EO or communication object or both will be deleted or not as a result of setting
 *  the EO to a particular state.
 * 
 *  \par Related APIs:
 *
 * 
 */


extern ClRcT clEoStateSet(
        CL_IN ClEoExecutionObjT     *pThis,
        CL_IN ClEoStateT             state);

/**
 ************************************
 *  \page pageEO101 clEoCreate
 *
 *  \par Synopsis:
 *  Creates an EO.
 * 
 *  \par Header File:
 *  clEoApi.h
 * 
 *  \par Library Files:
 *  libClEo
 *
 *  \par Syntax:
 *  \code     extern ClRcT clEoCreate (
 *                 CL_IN ClEoConfigT        *pConfig,
 *                 CL_OUT ClEoExecutionObjT **pThis);
 *  \endcode
 *
 *  \param pConfig: EO configuration parameters.
 *  \param pThis: (out) Handle of the EO.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing wrong configuration.
 *
 *  \par Description:
 *  This API is used to create an EO with the prescribed configuration. The
 *  EO abstracts the properties of thread or process. This API creates and 
 *  initializes the required data structures, creates the communication port 
 *  and RMD objects associated with the EO and spawns the background thread 
 *  which receives the IOC messages.
 * 
 *  \par Related APIs:
 *
 *  clEoStateSet()
 *
 */


extern ClRcT clEoCreate (
        CL_IN ClEoConfigT           *pConfig, 
        CL_OUT ClEoExecutionObjT    **pThis);


extern ClEoMemConfigT gClEoMemConfig;
extern ClBufferPoolConfigT gClEoBuffConfig;
extern ClHeapConfigT gClEoHeapConfig;
extern ClIocQueueInfoT gClIocRecvQInfo;

extern ClRcT clEoWaterMarkHit(ClCompIdT compId, ClWaterMarkIdT wmId, ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT flag, ClEoActionArgListT argList);

#if 0
extern ClRcT clEoIocCommPortWMNotification(ClCompIdT compId,
                                           ClIocPortT port,
                                           ClIocNodeAddressT node,
                                           ClIocNotificationT *pIocNotification);

extern ClRcT clEoIocPortNotification(
                           ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr
                           );
#endif

extern ClRcT clEoInitialize(ClInt32T argc, ClCharT *argv[]);

extern ClRcT clASPInitialize(void);

extern ClRcT clASPFinalize(void);

extern ClRcT clEoMain(ClInt32T argc, ClCharT *argv[]);

extern ClRcT clEoLibLog(ClUint32T libId,ClUint32T severity, const ClCharT *msg, ...);

extern void clEoNodeRepresentativeDeclare(const ClCharT *pNodeName);

extern ClRcT  clEoThreadSafeInstall(ClRcT (*pClEoSerialize)(ClPtrT),
                                    ClRcT (*pClEoDeSerialize)(ClPtrT));

extern ClRcT clEoThreadSafeDisable(ClUint8T protoid);

extern void clEoSendErrorNotify(ClBufferHandleT message, ClIocRecvParamT *pRecvParam);


extern ClRcT clEoNotificationCallbackInstall(ClIocPhysicalAddressT compAddr, ClPtrT pFunc, ClPtrT pArg, ClHandleT *pHandle);


extern ClRcT clEoNotificationCallbackUninstall(ClHandleT *pHandle);

extern ClRcT clAspClientLibFinalize(void);

extern ClRcT clEoClientGetFuncVersion(ClIocPortT port, ClUint32T funcId, ClVersionT *version);

extern ClRcT clEoEnqueueJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam);

extern ClRcT clEoEnqueueReassembleJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam);

extern ClRcT clEoEnqueueReplyJob(ClEoExecutionObjT *pThis, ClCallbackT job, ClPtrT invocation);

extern ClBoolT clEoQueueAmfResponseFind(ClUint32T pri);

extern ClRcT clEoDebugRegister(void);

extern ClRcT clEoDebugDeregister(void);

extern ClRcT clTimerDebugRegister(void);    

extern ClRcT clTimerDebugDeregister(void);

extern ClRcT clRmdDebugRegister(void);

extern ClRcT clRmdDebugDeregister(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_EO_IPI_H_ */
