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
 * File        : clCpmIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the interfaces for CPM used internally by other ASP
 * components.  
 *
 *
 *****************************************************************************/

#ifndef _CL_CPM_IPI_H_
#define _CL_CPM_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clEoApi.h>    

/**
 * Version supported for the EO library by CPM.
 */
#define CL_CPM_EO_VERSION_NO                0x0100

/**
 *  The structure ClCpmFuncWalkT is passed as an argument to function compoFuncEOWalk.
 *  It contains the function to be invoked and argument to be passed (if any).
 */
typedef struct
{
    /**
     * Function to be called.
     */
    ClUint32T funcNo;
    /**
     * Length of buffer.
     */
    ClUint32T inLen;
    /**
     * Input buffer.
     */
    ClInt8T inputArg[1];
} ClCpmFuncWalkT;

/**
 * This defines the RMD number for the Component Manager EO management
 * client installed on each EO.
 */
typedef enum
{
    /**
     * EO initialization function.
     */
    CL_EO_INITIALIZE_COMMON_FN_ID = 0,
    /**
     * EO Get State.
     */
    CL_EO_GET_STATE_COMMON_FN_ID =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 0),
    /**
     * EO Set State.
     */
    CL_EO_SET_STATE_COMMON_FN_ID =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 1),
    /**
     * Pings an EO.
     */
    CL_EO_IS_ALIVE_COMMON_FN_ID =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 2),
    /**
     * Returns RMD state of an EO.
     */
    CL_EO_GET_RMD_STATS =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 3),
    /**
     * Sets the thread priority of EO Threads.
     */
    CL_EO_SET_PRIORITY =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 4),
    /**
     * Sets the log level of the EO.
     */
    CL_EO_LOG_LEVEL_SET =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 5),
    /**
     * Retrieves the log level of the EO.
     */
    CL_EO_LOG_LEVEL_GET =
    CL_EO_GET_FULL_FN_NUM(CL_EO_EO_MGR_CLIENT_TABLE_ID, 6),
    /**
     * End of EO Function.
     */
    CL_EO_LAST_COMMON_FN_ID
} ClEoFunctionsIdT;

/**
 * This enumeration defines the COR class IDs for the SAF information
 * model.
 */
typedef enum
{
    CL_CPM_CLASS_UNKNOWN = 0,
    /**
     * Cluster MO Class ID.
     */
    CL_CPM_CLASS_CLUSTER = 0x7d02,
    /**
     * Node MO Class ID.
     */
    CL_CPM_CLASS_NODE = 0x7d03,
    /**
     * Service Unit MO Class ID.
     */
    CL_CPM_CLASS_SU = 0x7d04,
    /**
     * Component MO Class ID.
     */
    CL_CPM_CLASS_COMP = 0x7d05
} ClCpmClassIdT;

/**
 * This enumeration defines the COR attribute IDs for the SAF
 * information model.
 */
typedef enum
{
    /**
     * Component class attribute for component name.
     */
    CL_CPM_COMP_NAME = 1,
    /**
     * Component class attribute for its operational state.
     */
    CL_CPM_COMP_OPERATIONAL_STATE,
    /**
     * Component class attribute for its presence state.
     */
    CL_CPM_COMP_PRESENCE_STATE,
    /**
     * Service Unit class attribute for its name.
     */
    CL_CPM_SU_NAME,
    /**
     * Service Unit class attribute for its operational state.
     */
    CL_CPM_SU_OPERATIONAL_STATE,
    /**
     * Service Unit class attribute for its presence state.
     */
    CL_CPM_SU_PRESENCE_STATE,
    /**
     * Node class attribute for name.
     */
    CL_CPM_NODE_NAME
} ClCpmAttrIdsT;
    
/**
 * This enumeration defines the request types returned by the
 * Component Manager, while invoking registered function callback, on
 * success of LCM command.
 */
typedef enum
{
    CL_CPM_REQUEST_NONE = 0,
    /**
     * SAF Compliant Health Check request.
     */
    CL_CPM_HEALTHCHECK = 1,
    /**
     * SA-Aware and Proxied component terminate request.
     */
    CL_CPM_TERMINATE = 2,
    /**
     * Proxied component instantiation request.
     */
    CL_CPM_PROXIED_INSTANTIATE = 3,
    /**
     * Proxied component cleanup request.
     */
    CL_CPM_PROXIED_CLEANUP = 4,
    /**
     * Component extensive healthcheck request.
     */
    CL_CPM_EXTN_HEALTHCHECK = 5,
    /**
     * SA-Aware OR Non-proxied component instantiation request.
     */
    CL_CPM_INSTANTIATE = 6,
    /**
     * Component cleanup request.
     */
    CL_CPM_CLEANUP = 7,
    /**
     * Component restart request.
     */
    CL_CPM_RESTART = 8
} ClCpmCompRequestTypeT;

/**
 * Payload sent in the response of the Component Manager LCM functions.
 */
typedef struct
{
    /**
     * Name of the Component/Service Unit.
     */
    ClCharT name[CL_MAX_NAME_LENGTH];
    /**
     * Response return code for the LCM request.
     */
    ClRcT returnCode;
    /**
     * LCM request type invoked by you.
     */
    ClCpmCompRequestTypeT requestType;
} ClCpmLcmResponseT;

/**
 * The structure ClCpmLcmReplyT is passed to the Component Manager LCM
 * API if a response such as a success or failure is required for the
 * requested operation.
 */
typedef struct
{
    /**
     * IOC address of the node to which the reply needs to be sent.
     */
    ClIocNodeAddressT srcIocAddress;
    /**
     *  Port number where reply needs to be sent.
     */
    ClIocPortT srcPort;
    /**
     * RMD number that will get invoked in response.
     */
    ClUint32T rmdNumber;
} ClCpmLcmReplyT;

/**
 * Response data sent by Boot Manager after setting the boot level.
 */
typedef struct
{
    /**
     * Name of the node for which boot set level was requested.
     */
    ClNameT nodeName;
    /**
     * BootLevel, which was requested to be set.
     */
    ClUint32T bootLevel;
    /**
     * Response return code for the boot set level.
     */
    ClRcT retCode;
} ClCpmBmSetLevelResponseT;

extern ClRcT clCpmComponentResourceCleanup(void);

/**
 ************************************
 *  \brief Walks through all the EOs.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pWalk  Contains information regarding the walk.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CPM_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_ERR_VERSION_MISMATCH On version mismatch.
 *
 *  \par Description:
 *  This API is used to perform an operation on all the EOs registered with
 *  the Component Manager. It invokes the function with arguments which
 *  are specified as function number and input buffer fields in \e pWalk
 *  on all the EOs registered with the component manager.
 *
 *  \note This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmExecutionObjectStateSet()
 *
 */
extern ClRcT clCpmFuncEOWalk(CL_IN ClCpmFuncWalkT *pWalk);

/**
 ************************************
 *  \brief Sets the state of an EO.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param destAddr Address of the Component Manager.
 *  \param eoId ID of the EO.
 *  \param state New state being assigned to the EO.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to set the state of an EO, as specified by the parameter
 *  \e state. After calling this API the EO designated by \e eoId on the node
 *  having the address \e destAddr will be set to the state \e state.
 *
 *  \note This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmExecutionObjectStateSet(CL_IN ClIocNodeAddressT destAddr,
                                          CL_IN ClEoIdT eoId,
                                          CL_IN ClEoStateT state);

/************************** CPM-BM related API **************************/

/**
 ************************************
 *  \brief Returns the current bootlevel of a node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pNodeName Pointer to the name of the node for which
 *  boot level is to be returned.
 *  \param pBootLevel (out) Boot level of the node \e pNodeName.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to return the current bootlevel of the node.
 *  The bootlevel returned by this API indicates the level upto which
 *  a particular node \e pNodeName has been booted.
 *
 *  \note
 *  This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmBootLevelSet(), clCpmBootLevelMax()
 *
 */
extern ClRcT clCpmBootLevelGet(CL_IN ClNameT *pNodeName,
                               CL_OUT ClUint32T *pBootLevel);

/**
 ************************************
 *  \brief Sets the bootlevel of a node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pNodeName Pointer to the name of the node for which
 *  boot level is to be set.
 *
 *  \param pSrcInfo  Pointer to the source information, where the reply of
 *  the requested operation is expected. If no response is required,
 *  you can pass this as NULL.
 *
 *  \param bootLevel Boot level to be set for the node.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to set the bootlevel of the node. If the \e bootLevel is
 *  more than the current boot level, then the CPM brings up all the service units
 *  mentioned in subsequent boot levels up to the required boot level.
 *  If the \e bootLevel is less than the current boot level, then the CPM
 *  terminates all the service units which are listed above the \e bootLevel.
 *
 *  \note
 *  This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmBootLevelGet(), clCpmBootLevelMax()
 *
 */
extern ClRcT clCpmBootLevelSet(CL_IN ClNameT *pNodeName,
                               CL_IN ClCpmLcmReplyT *pSrcInfo,
                               CL_IN ClUint32T bootLevel);

/**
 ************************************
 *  \brief Returns the maximum bootlevel of a node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pNodeName Pointer to the name of the node for which
 *  boot level is returned.
 *  \param pBootLevel (out) Maximum boot level of the node \e pNodeName.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to return the maximum possible bootlevel of the node.
 *  This indicates the maximum level upto which the node \e pNodeName can be
 *  booted.
 *
 *  \note
 *   This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmBootLevelGet(), clCpmBootLevelSet()
 *
 */
extern ClRcT clCpmBootLevelMax(CL_IN ClNameT *pNodeName,
                               CL_OUT ClUint32T *pBootLevel);

/**
 * Component and Service Unit life cycle related functions. These
 * component or Service Unit might exist anywhere in the cluster, if
 * these calls are directed to Component Manager/Global. Component and
 * SU name must be RDN. On a given node, all the component names and
 * SuName must be unique.
 *
 * Though there is no API specified by SAF for these functions, but
 * they do talk about the command, which ultimately needs to be
 * implemented. Following function might be invoked by AMS, FM,
 * BootManager or north Bound Interface as CLI
 */
/**
 ****************************************************************
 * Component Life Cycle related Operations
 *****************************************************************/

/**
 ************************************
 *  \brief Instantiates a component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component being instantiated.
 *  \param pNodeName Pointer to the name of the node where \e pCompName exist.
 *  \param pSrcInfo Pointer to the source information, where the reply of
 *  the requested operation is expected. If no response is required,
 *  you can pass this as NULL.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to instantiate a given component, identified by
 *  \e pCompName on the node \e pNodeName. If \e pSrcInfo is NULL then no reply
 *  regarding the success or failure in instantiating the component will be sent. Otherwise
 *  a reply is sent to the requested port through an RMD call, as specified by the
 *  fields of the structure \e pSrcInfo.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentTerminate(), clCpmComponentCleanup(),
 *      clCpmComponentRestart()
 *
 */
extern ClRcT clCpmComponentInstantiate(CL_IN ClNameT *pCompName,
                                       CL_IN ClNameT *pNodeName,
                                       CL_IN ClCpmLcmReplyT *pSrcInfo);

/**
 ************************************
 *  \brief Terminates the component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component being terminated.
 *  \param pNodeName Pointer to the name of the node where \e pCompName exist.
 *  \param pSrcInfo  Pointer to the source information, where the reply of
 *  the requested operation is expected. If no response is required,
 *  you can pass this as NULL.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to terminate (graceful shut-down) a given component,
 *  identified by \e pCompName on the node \e pNodeName. If \e pSrcInfo
 *  is NULL then no reply regarding the success or failure in terminating the component
 *  is sent. Otherwise a reply is sent to the requested port through an
 *  RMD call, as specified by the fields of the structure \e pSrcInfo.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentInstantiate(), clCpmComponentCleanup(),
 *      clCpmComponentRestart()
 *
 */
extern ClRcT clCpmComponentTerminate(CL_IN ClNameT *pCompName,
                                     CL_IN ClNameT *pNodeName,
                                     CL_IN ClCpmLcmReplyT *pSrcInfo);

/**
 ************************************
 *  \brief Cleans up the component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component being cleaned up.
 *  \param pNodeName Pointer to the name of the node where \e pCompName exist.
 *  \param pSrcInfo  Pointer to the source information, where the reply of
 *  the requested operation is expected. If no response is required,
 *  you can pass this as NULL.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to clean up (abrupt shut-down) a given component,
 *  identified identified by \e pCompName on the node \e pNodeName.
 *  If \e pSrcInfo is NULL then no reply regarding the success or failure in cleaning up
 *  the component will be sent. Otherwise a reply is sent to the requested port
 *  through an RMD call, as specified by the fields of the structure \e pSrcInfo.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentInstantiate(), clCpmComponentTerminate(), 
 *      clCpmComponentRestart()
 *
 */
extern ClRcT clCpmComponentCleanup(CL_IN ClNameT *pCompName,
                                   CL_IN ClNameT *pNodeName,
                                   CL_IN ClCpmLcmReplyT *pSrcInfo);

/**
 ************************************
 *  \brief Restarts the component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component being restarted.
 *  \param pNodeName Pointer to the name of the node where \e pCompName exist.
 *  \param pSrcInfo  Pointer to the source information, where the reply of
 *  the requested operation is expected. If no response is required,
 *  you can pass this as NULL.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to restart a given component, identified by \e pCompName
 *  on node \e pNodeName. If \e pSrcInfo is NULL then no reply as to success or
 *  failure in restarting the component is sent. Otherwise a reply is
 *  sent to the requested port through an RMD call, as specified by the fields of
 *  the structure \e pSrcInfo. The complete restart process includes,
 *  termination of the component, its clean-up and finally instantiating
 *  the component again, back to the earlier state.
 *
 *  \note This is an asynchronous API.
 *
 *  \warning
 *  The cleanup process will get executed only when (graceful) termination is
 *  failed. In this case, termination and cleanup will occur without deleting
 *  the communication port.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentInstantiate(), clCpmComponentTerminate(),
 *      clCpmComponentCleanup()
 *
 */
extern ClRcT clCpmComponentRestart(ClNameT *pCompName,
                                   CL_IN ClNameT *pNodeName,
                                   CL_IN ClCpmLcmReplyT *pSrcInfo);

extern ClRcT clCpmTargetInfoInitialize(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_IPI_H_ */
