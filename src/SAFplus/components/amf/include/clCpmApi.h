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

/**
 * \file
 * \brief Header file for the APIs and data types exposed by the CPM.
 * \ingroup cpm_apis
 */

/**
 * \addtogroup cpm_apis
 * \{
 */

#ifndef _CL_CPM_API_H_
#define _CL_CPM_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>
#include <clTimerApi.h>
#include <clCksmApi.h>
#include <clIocApi.h>
#include <clRmdApi.h>
#include <clIocApiExt.h>

#include <clCpmConfigApi.h>
#include <clCpmErrors.h>
#include <clAmsTypes.h>

/**
 * SAF version supported by the CPM.
 */
/**
 * Release code.
 */
#define CL_CPM_RELEASE_CODE                 'B'
/**
 * Major version.
 */
#define CL_CPM_MAJOR_VERSION                0x01
/**
 * Minor version.
 */
#define CL_CPM_MINOR_VERSION                0x01

/**
 * Event Channel information related to Component failure.
 * Component Manager publishes event for the component failures using event
 * filters. At present you may subscribe to it using the IOC address.
 */
#define CL_CPM_EVENT_CHANNEL_NAME           "CPM_EVENT_CHANNEL"

/**
 * The node Arrival and Departure related Event channel information.
 */
#define CL_CPM_NODE_EVENT_CHANNEL_NAME      "CPM_NODE_EVENT_CHANNEL"

/**
 * This define is related to EO state management, in the context of
 * heartbeat success or failure.
 */
#define CL_CPM_EO_ALIVE                     (ClUint32T)0

/**
 * EO state management related defines, in the context of heartbeat success or
 * failure.
 */
#define CL_CPM_EO_DEAD                      (ClUint32T)0xFFFFFFF

/**
 * IOC address related macros.
 */
#define CL_CPM_IOC_SLOT_BITS        16
#define CL_CPM_IOC_SLOT_BITS_HEX    0x0000ffff

/**
 *  Macro for generating the IOC address based on the \e chassidId and \e slotId.
 */
#define CL_CPM_IOC_ADDRESS_GET(myCh, mySl, address) \
    (address) = (myCh) << CL_CPM_IOC_SLOT_BITS;     \
    (address) |= (mySl);

/**
 *  Macro for getting the \e slotId from IOC address.
 */
#define CL_CPM_IOC_ADDRESS_SLOT_GET(address, mySl)  \
    (mySl) = (address) & CL_CPM_IOC_SLOT_BITS_HEX;
    
/**
 *  Macro for getting the \e chsssisId from IOC address.
 */
#define CL_CPM_IOC_ADDRESS_CHASSIS_GET(address, myCh)   \
    (myCh) = (address) >> CL_CPM_IOC_SLOT_BITS;

/**
 * The type of the handle supplied by the CPM to the process which
 * calls the clCpmClientInitialize() API. This indicates the
 * association of the process with the CPM(AMF) client library.
 */
typedef ClHandleT ClCpmHandleT;

/**
 * Feedback sent by the software component in response to
 * heartbeat request sent by CPM.
 */
#define ClCpmSchedFeedBackT  ClEoSchedFeedBackT

/**
 * Pattern for subscribing for component arrival event.
 */
#define CL_CPM_COMP_ARRIVAL_PATTERN   (1 << 0)
/**
 * Pattern for subscribing for component terminate event.
 */
#define CL_CPM_COMP_DEPART_PATTERN    (1 << 1)
/**
 * Pattern for subscribing for component death event
 */
#define CL_CPM_COMP_DEATH_PATTERN     (1 << 2)
/**
 * Pattern for subscribing for node arrival event
 */
#define CL_CPM_NODE_ARRIVAL_PATTERN   (1 << 3)
/**
 * Pattern for subscribing for node departure event
 */
#define CL_CPM_NODE_DEPART_PATTERN    (1 << 4)
/**
 * Pattern for subscribing for node death event
 */
#define CL_CPM_NODE_DEATH_PATTERN     (1 << 5)


/**
 * Flags that can be used by subscriber to distinguish diffrent
 * events related to node.
 */
typedef enum
{
    /**
     * Indicates a node's arrival.
     */
    CL_CPM_NODE_ARRIVAL,

    /**
     * Indicates a node's graceful departure.
     */
    CL_CPM_NODE_DEPARTURE,

    /**
     * Indicates a node's sudden/ungraceful departure.
     */
    CL_CPM_NODE_DEATH
} ClCpmNodeEventT;

    
/**
 * Flags that can be used by subscriber to distinguish diffrent
 * component events.
 */
typedef enum
{
    /**
     * Indicates that component is ready to provide service.
     */
    CL_CPM_COMP_ARRIVAL,
    /**
     * Indicates that component was terminated gracefully.
     */
    CL_CPM_COMP_DEPARTURE,
    /**
     * Indicates that component was terminated ungracefully,
     * i.e. failure event.
     */
    CL_CPM_COMP_DEATH
} ClCpmCompEventT;
        
/**
 * Payload data for the component death event published by the CPM.
 */
typedef struct
{
    /**
     * Component which failed/lost heartbeat to the CPM.
     */
    ClNameT compName;
    /**
     * Node name containing the component.
     */
    ClNameT nodeName;
    /**
     * Component ID.
     */
    ClUint32T compId;
    /**
     * EO ID.
     */
    ClEoIdT eoId;
    /**
     * IOC address of the node containing the component.
     * component is located.
     */
    ClIocNodeAddressT nodeIocAddress;
    /**
     *  IOC port of the node containing the component.
     */
    ClIocPortT eoIocPort;
    /**
     * Indication of whether component is:
     * - Coming up
     * - Going down gracefully (i.e. termination)
     * - Gone down ungracefully (i.e. abnormal exit, crash etc)
     */
    ClCpmCompEventT operation;
} ClCpmEventPayLoadT;

/**
 * Payload data for the node arrival/departure event published by the
 * CPM.
 */
typedef struct
{
    /**
     * Name of the node for which this event is published.
     */
    ClNameT nodeName;
    /**
     * Node IOC Address, where this component is located.
     */
    ClIocNodeAddressT nodeIocAddress;
    /**
     * Indication for whether the node is starting up, i.e., \c CL_CPM_NODE_ARRIVAL
     * or shutting down gracefully, i.e., \c CL_CPM_NODE_DEPARTURE or shutting down
     * ungracefully, i.e., \c CL_CPM_NODE_DEATH.
     */
    ClCpmNodeEventT operation;
} ClCpmEventNodePayLoadT;

/*
 * Comp CSI data fetched from clCpmCompCSIList API
 */
typedef struct ClCpmCompCSI
{
    ClAmsHAStateT haState;
    ClAmsCSIDescriptorT csiDescriptor;
}ClCpmCompCSIT;


typedef struct ClCpmCompCSIRef
{
    ClUint32T numCSIs;
    ClCpmCompCSIT *pCSIList;
}ClCpmCompCSIRefT;

/**
 * Component specific data which is maintained by CPM.
 */
typedef struct ClCpmCompSpecInfo
{
    /**
     * Arguments of a component.
     */

    ClUint32T numArgs;
    ClCharT **args;

    /**
     * Healthcheck related information
     */
    ClUint32T period;
    ClUint32T maxDuration;
    ClAmsLocalRecoveryT recovery;
    
} ClCpmCompSpecInfoT;

/**
 * SAF compliant APIs and data types of CPM.
 */

/**
 * \brief Component Manager requests the component, identified by \e
 * compName, to execute healthcheck using \e healthcheckkey
 * [Asynchronous Call].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pCompName Pointer to the name of the component that must
 * undergo healthcheck.
 *
 * \param pHealthCheckKey Key to fetch the healthcheck related
 * attributes.
 *
 */
typedef ClRcT (*ClCpmHealthCheckCallbackT)(CL_IN ClInvocationT invocation,
                                           CL_IN const ClNameT *pCompName,
                                           CL_IN ClAmsCompHealthcheckKeyT
                                           *pHealthCheckKey);

/**
 * \brief Component Manager requests the component, identified by \e
 * compName, to gracefully shutdown [Asynchronous Call with reply].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pCompName Pointer to the name of the component that must
 * gracefully terminate.
 *
 */
typedef ClRcT (*ClCpmTerminateCallbackT) (CL_IN ClInvocationT invocation,
                                          CL_IN const ClNameT *pCompName);

/**
 * \brief Component Manager requests the component, identified by \e
 * compName, to assume a particular HA state, specified by \e haState,
 * for one or all the CSIs [Asynchronous Call].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pCompName Pointer to the name of the component to which a
 * CSI needs to be assigned.
 *
 * \param haState HA-state to be assigned for CSI identified by \e
 * csiDescriptor, or for all CSIs, already supported by component [if
 * \c CSI_TARGET_ALL is set in \e csiFlags of \e csiDescriptor].
 *
 * \param csiDescriptor Information about the CSI.
 *
 * \sa ClCpmCSIRmvCallbackT
 */
typedef ClRcT (*ClCpmCSISetCallbackT) (CL_IN ClInvocationT invocation,
                                       CL_IN const ClNameT *pCompName,
                                       CL_IN ClAmsHAStateT haState,
                                       CL_IN ClAmsCSIDescriptorT csiDescriptor);

/**
 * \brief Removes one or all the assigned CSIs.  Component Manager
 * requests the component, identified by \e compName, to remove a
 * particular CSI or all the CSIs [Asynchronous Call].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pCompName Pointer to the name of the component to which a
 * CSI needs to be assigned.
 *
 * \param pCsiName Pointer to the \e csiName that must be removed from
 * component identified by \e compName.
 *
 * \param csiFlags Flags, which specify whether one or more CSI are
 * affected.
 *
 * \sa ClCpmCSISetCallbackT
 */
typedef ClRcT (*ClCpmCSIRmvCallbackT) (CL_IN ClInvocationT invocation,
                                       CL_IN const ClNameT *pCompName,
                                       CL_IN const ClNameT *pCsiName,
                                       CL_IN ClAmsCSIFlagsT csiFlags);

/**
 * \brief Requested information gets delivered to \e
 * notificationBuffer. Type of information returned in this buffer
 * depends on the \e trackFlag parameter of the \e
 * clAMSProtectionGroupTrack function [Asynchronous Call]
 *
 * \param pCsiName Pointer to the \e csiName that must be removed from
 * component identified by \e compName.
 *
 * \param pNotificationBuffer Pointer to a notification buffer, which
 * contains the requested information.
 *
 * \param numberOfMembers Number of components that belongs to
 * protection group associated with the CSI, designated by \e csiName.
 *
 * \param error Indication whether Component Manager was able to
 * perform the operation.
 *
 */
typedef
void (*ClCpmProtectionGroupTrackCallbackT) (CL_IN const ClNameT *pCsiName,
                                            CL_IN ClAmsPGNotificationBufferT
                                            *pNotificationBuffer,
                                            CL_IN ClUint32T numberOfMembers,
                                            CL_IN ClUint32T error);

/**
 * \brief Component Manager requests the proxy component to
 * instantiate the component, identified by \e proxiedCompName
 * [Asynchronous Call].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pProxiedCompName Pointer to the name of the component which
 * needs to be instantiated.
 *
 * \sa ClCpmProxiedComponentCleanupCallbackT
 */
typedef ClRcT (*ClCpmProxiedComponentInstantiateCallbackT) (CL_IN ClInvocationT
                                                            invocation,
                                                            CL_IN const ClNameT
                                                            *pProxiedCompName);

/**
 * \brief Component Manager requests the proxy component to cleanup
 * the component, identified by \e proxiedCompName [Asynchronous
 * call].
 *
 * \param invocation Particular invocation of this callback function.
 *
 * \param pProxiedCompName Pointer to the name of the component to
 * which needs to be cleaned up.
 *
 * \sa ClCpmProxiedComponentInstantiateCallbackT
 */
typedef ClRcT (*ClCpmProxiedComponentCleanupCallbackT) (CL_IN ClInvocationT
                                                        invocation,
                                                        CL_IN const ClNameT
                                                        *pProxiedCompName);

/**
 * \brief This is the type of the callback fuction, which will be
 * called when an node/component arrival/departure event occurs.
 *
 * \param event Type of the event indicating whether it is node arrival/departure
 * or component arrival/departure.
 *
 * \param pArg Argument to the callback fuction.
 * 
 * \param pAddress Address of the node/component where the \e event occurred.
 *
 */
typedef void (*ClCpmNotificationFuncT)(CL_IN ClIocNotificationIdT event, 
                                       CL_IN ClPtrT pArg,
                                       CL_IN ClIocAddressT *pAddress);

/**
 * The structure ClCpmCallbacksT contains the various callback
 * functions that the Component Manager can invoke on a component.
 */
typedef struct
{
    /**
     * Callback for checking the health of the component.
     */
    ClCpmHealthCheckCallbackT appHealthCheck;
    /**
     * Callback for gracefully shutting down the component.
     */
    ClCpmTerminateCallbackT appTerminate;
    /**
     * Callback for assigning the component service instance to the
     * component.
     */
    ClCpmCSISetCallbackT appCSISet;
    /**
     * Callback for removing the component service instance assigned
     * to the component.
     */
    ClCpmCSIRmvCallbackT appCSIRmv;
    /**
     * Callback invoked when the component registers for tracking
     * changes in protection group and protection group configuration
     * changes.
     */
    ClCpmProtectionGroupTrackCallbackT appProtectionGroupTrack;
    /**
     * Callback for instantiating a proxied component.
     */
    ClCpmProxiedComponentInstantiateCallbackT appProxiedComponentInstantiate;
    /**
     * Callback for cleaning up a proxied component.
     */
    ClCpmProxiedComponentCleanupCallbackT appProxiedComponentCleanup;
} ClCpmCallbacksT;

/**
 ************************************
 *  \brief Initializes the client Component Manager library.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCpmHandle Handle of the component.
 *  \param pCallback Callbacks provided by the application to the Component
 *  Manager.
 *  \param pVersion Required version number.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  Before invoking this API, the EO must be created. This API is used to
 *  update the EO client table internally based on \e ClCpmCallbacksT.
 *  It also ensures that the Component Manager client and Server Libraries are
 *  compatible in version.
 *
 *  \note
 *  This API is equivalent to \e saAmfInitialize, which makes a synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientFinalize()
 *
 */
extern ClRcT clCpmClientInitialize(CL_OUT ClCpmHandleT *pCpmHandle,
                                   CL_IN const ClCpmCallbacksT *pCallback,
                                   CL_INOUT ClVersionT *pVersion);

/**
 ************************************
 *  \brief Cleans up the client Component Manager library.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to clean up the client Component Manager library.
 *  It releases all the resources acquired when clCpmClientInitialize()
 *  was called. The \e cpmHandle will become invalid after calling
 *  this API.
 *
 *  \note
 *  This API is equivalent to \e saAmfFinalize, which makes an asynchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize()
 *
 */
extern ClRcT clCpmClientFinalize(CL_IN ClCpmHandleT cpmHandle);

/**
 ************************************
 *  \brief Returns an operating system handle for detecting
 *  pending callbacks.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pSelectionObject A pointer to the operating system handle that the
 *  process can use to detect pending callbacks
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API returns the operating system handle \e pSelectionObject,
 *  associated with the handle \e cpmHandle. The process can use this operating
 *  system handle to detect pending callbacks, instead of repeatedly invoking
 *  clCpmDispatch() for this purpose.
 *  \par
 *  The operating system handle returned by this API is a file descriptor
 *  that is used with poll() or select() systems call detect incoming
 *  callbacks.
 *  \par
 *  The \e pSelectionObject returned by this API is valid until
 *  clCpmClientFinalize() is invoked on the same handle \e cpmHandle.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmDispatch()
 *
 */
extern ClRcT clCpmSelectionObjectGet(CL_IN ClCpmHandleT cpmHandle,
                                     CL_OUT ClSelectionObjectT
                                     *pSelectionObject);

/**
 ************************************
 *  \brief Invoke pending callbacks.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param dispatchFlags Flags that specify the callback execution behaviour
 *  of this function
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API invokes, in the context of the calling EO, pending callbacks for
 *  handle \e cpmHandle in a way that is specified by the \e dispatchFlags
 *  parameter.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmSelectionObjectGet()
 *
 */
extern ClRcT clCpmDispatch(CL_IN ClCpmHandleT cpmHandle,
                           CL_OUT ClDispatchFlagsT dispatchFlags);

/**
 ************************************
 *  \brief Registers a component with CPM.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *  \param pProxyCompName Name of the component, proxy for \e pCompName.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing Null Pointer
 *  \retval CL_CPM_ERR_INIT If callbacks are provided during the
 *  initialization is improper.
 *  \retval CL_CPM_ERR_EXIST If the component has already been registered.
 *
 *  \par Description:
 *  This API is used to register a component with CPM. It can also be used
 *  by a proxy component to register a proxied component. By calling this
 *  API, the component is indicating that it is up and running and is ready
 *  to provide service.
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentRegister, which makes a
 *  synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentUnregister()
 *
 */
extern ClRcT clCpmComponentRegister(CL_IN ClCpmHandleT cpmHandle,
                                    CL_IN const ClNameT *pCompName,
                                    CL_IN const ClNameT *pProxyCompName);

/**
 ************************************
 *  \brief Un-registers a component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *  \param pProxyCompName Name of the component proxied by \e pCompName
 *  component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_CPM_ERR_BAD_OPERATION If the component identified by
 *  \e pCompName has not unregistered all the proxied components OR
 *  \e proxyComp is not the proxy for \e pCompName.
 *
 *  \par Description:
 *  This API is used for two purposes:
 *  Either a proxy component can unregister one of its proxied components or
 *  a component can unregister itself. The \e cpmHandle in this
 *  API must be same as that used in the corresponding
 *  clCpmComponentRegister() call.
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentUnregister, which makes an
 *  asynchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentRegister()
 *
 */
extern ClRcT clCpmComponentUnregister(CL_IN ClCpmHandleT cpmHandle,
                                      CL_IN const ClNameT *pCompName,
                                      CL_IN const ClNameT *pProxyCompName);

/**
 ************************************
 *  \brief Returns the component name.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API returns the name of the component the calling process belongs to.
 *  This API can be invoked by the process before its component has been
 *  registered with the CPM. The component name provided by this API should be
 *  used by a process when it registers its local component.
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentNameGet.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentRegister()
 *
 */
extern ClRcT clCpmComponentNameGet(CL_IN ClCpmHandleT cpmHandle,
                                   CL_OUT ClNameT *pCompName);

extern ClRcT clCpmComponentDNNameGet(CL_IN ClCpmHandleT cpmHandle,
                                     CL_OUT ClNameT *pCompName,
                                     CL_OUT ClNameT *pDNName);

/**
 ************************************
 *  \brief Respond to AMF with the result of components execution of a
 *         particular request by AMF.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param invocation associates an invocation of this response function
 *  with a particular invocation of a callback function by the cpm.
 *  \param rc Status of executing a particular callback of AMF.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to respond the success and failure of the callback
 *  invoked by CPM. Through this API the component responds to the CPM
 *  with the result of its execution of a particular request of the
 *  CPM designated by the \e invocation.
 *
 *  \note
 *  This API is equivalent to \e saAmfResponse.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentRegister()
 *
 */

extern ClRcT clCpmResponse(CL_IN ClCpmHandleT cpmHandle,
                           CL_IN ClInvocationT invocation,
                           CL_IN ClRcT rc);

/**
 ************************************
 *  \brief Returns the HA state of the component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param compName A pointer to the name of the component for which the
 *  information is requested
 *  \param csiName A pointer to the name of the component service instance
 *  for which the information is requested
 *  \param haState Pointer to the HA state that the AMS is assigning to the
 *  component, identified by \e compName, on behalf of the component service
 *  instance, identified by \e csiName.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER on passing the unallocated node Name
 *  \retval CL_ERR_DOESNT_EXIST CPM library is not able to retrieve the
 *  nodeName.
 *
 *  \par Description:
 *  This API returns the HA state of a component, identified by \e compName,
 *  on behalf of the component service instance, identified by \e csiName.
 *  The HA state of the component indicates whether it is currently responsible
 *  to provide service characterized by the component service instance assigned
 *  to it, whether it is a standby or whether it is in the quiesced state.
 *
 *  \par Library Files:
 *  libClAmfClient
 *
 *  \sa clCpmClientInitialize()
 *
 */
extern ClRcT clCpmHAStateGet(CL_IN ClCpmHandleT cpmHandle,
                             CL_IN ClNameT *compName,
                             CL_IN ClNameT *csiName,
                             CL_OUT ClAmsHAStateT *haState);

/**
 ************************************
 *  \brief Respond to AMF whether it was able to successfully service
 *         all pending requests for a particular component service
 *         instance, following the earlier request by AMF to the component
 *         to enter the \e CL_AMS_HA_STATE_QUIESCING HA state via the
 *         components ::ClCpmCSISetCallbackT callback.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param invocation Associates an invocation of this response function
 *                    with a particular invocation of a callback function
 *                    by the CPM.
 *  \param retCode Indicates the status of the quiescing operation.
 *  
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to respond the success and failure of the
 *  callback invoked by CPM for the quiescing the HA state of a given
 *  CSI. Using this call a component can notify the CPM that it has
 *  successfully stopped its activity related to a particular
 *  component service instance or to all component service instances
 *  assigned to it, following a previous request by the AMS to enter
 *  the \c CL_AMS_HA_STATE_QUIESCING HA state for that particular
 *  component service instance or to all component service instances.
 *
 *  \note
 *  This API is equivalent to \e saAmfCSIQuiescingComplete.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentRegister(), clCpmResponse()
 *
 */
extern ClRcT clCpmCSIQuiescingComplete(CL_IN ClCpmHandleT cpmHandle,
                                       CL_IN ClInvocationT invocation,
                                       CL_IN ClRcT retCode);

/**
 ************************************
 *  \brief Notifies about the failed component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Pointer to the component name for which the failure is
 *  notified.
 *  \param errorDetectionTime Time when the error is detected.
 *  \param recommendedRecovery Recommonded recovery to be performed using AMF.
 *  \param alarmHandle This is the key returned by the \e clAlarmRaise function.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to notify the Component Manager about the failure
 *  of a component. It reports an error and provides a recovery
 *  recommendation to the AMF. The AMF will react on the error report
 *  and carry out a recovery operation to retain the availability of
 *  component service instances supported by the erroneous
 *  component. The AMF will not carry out weaker recovery action than
 *  that recommended by the above API, but it may decide to escalate
 *  to higher recovery level.
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentFailureReport, which makes a
 *  synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentFailureClear()
 *
 */
extern ClRcT
clCpmComponentFailureReport(CL_IN ClCpmHandleT cpmHandle,
                            CL_IN const ClNameT *pCompName,
                            CL_IN ClTimeT errorDetectionTime,
                            CL_IN ClAmsLocalRecoveryT recommendedRecovery,
                            CL_IN ClUint32T alarmHandle);

/**
 ************************************
 *  \brief Notifies about the restoration of the failed component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Pointer to the component name for which the failure is
 *  notified.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to notify the Component Manager that the failed component
 *  is restored. It cancels the failure notification made by
 *  clCpmComponentFailureReport().
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentFailureClear, which makes a
 *  synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentFailureReport()
 *
 */
extern ClRcT clCpmComponentFailureClear(CL_IN ClCpmHandleT cpmHandle,
                                        CL_IN ClNameT *pCompName);

/**
 ************************************
 *  \brief Starts the component healthcheck.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *  \param pCompHealthCheck Key used to fetch healthcheck parameters
 *  for this component. Not used.
 *  \param invocationType Indicates whether the AMF should initiate
 *  the healthcheck or the component itself performs the healthcheck.
 *  \param recommondedRecovery Recommended recovery to be performed by
 *  the AMF if the component fails a healthcheck.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER One of the parameters are invalid.
 *  \retval CL_ERR_INVALID_HANDLE The passed CPM handle is invalid.
 *
 *  \par Description:
 *  This API starts health check of the component \e pCompName. The \e
 *  invocationType specifies whether this is an AMF invoked
 *  healthcheck or a component invoked healthcheck. If the \e
 *  invocationType is CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED, the
 *  appHealthCheck() callback function must be supplied when calling
 *  clCpmClientInitialize().
 *
 *  \note
 *  This API is equivalent to \e saAmfHealthCheckStart.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmHealthcheckConfirm(),
 *      clCpmHealthcheckStop()
 *
 */
extern ClRcT
clCpmHealthcheckStart(CL_IN ClCpmHandleT cpmHandle,
                      CL_IN const ClNameT *pCompName,
                      CL_IN const ClAmsCompHealthcheckKeyT *pCompHealthCheck,
                      CL_IN ClAmsCompHealthcheckInvocationT invocationType,
                      CL_IN ClAmsRecoveryT recommondedRecovery);

/**
 ************************************
 *  \brief Inform AMF the status of component invoked healthcheck.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *  \param pCompHealthCheck Key used to fetch healthcheck parameters
 *  for this component. Not used.
 *  \param healthCheckResult Result of the component invoked healthcheck.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE The passed CPM handle is invalid.
 *
 *  \par Description:
 *  This API is used by the component to inform AMF that, it has
 *  performed a healthcheck on component \e pCompName and whether the
 *  healthcheck was successful or not using \e healthCheckResult,
 *  which is the exit status of the healthcheck performed on the
 *  component.
 *
 *  \note
 *  This API is equivalent to \e saAmfHealthCheckConfirm.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmHealthcheckStart(), 
 *      clCpmHealthcheckStop()
 *
 */
extern ClRcT
clCpmHealthcheckConfirm(CL_IN ClCpmHandleT cpmHandle,
                        CL_IN const ClNameT *pCompName,
                        CL_IN const ClAmsCompHealthcheckKeyT *pCompHealthCheck,
                        CL_IN ClRcT healthCheckResult);

/**
 ************************************
 *  \brief Stops the component healthcheck.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component for which health-check needs to be
 *  stopped.
 *  \param pCompHealthCheck Key used to fetch healthcheck parameters
 *  for this component. Not used.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE The passed CPM handle is invalid.
 *
 *  \par Description:
 *  This API is used to stop the healthcheck of the component \e compName.
 *
 *  \note
 *  This API is equivalent to \e saAmfComponentHealthCheckStop.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmComponentHealthCheckStart()
 *
 */
extern ClRcT
clCpmHealthcheckStop(CL_IN ClCpmHandleT cpmHandle,
                     CL_IN const ClNameT *pCompName,
                     CL_IN const ClAmsCompHealthcheckKeyT *pCompHealthCheck);

/**
 ************************************
 *  \brief Track the protection group for the given CSI.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCsiName Pointer to the CSI for which protection group needs to
 *  be tracked.
 *  \param trackFlags The kind of tracking that is requested, which is the
 *  bitwise OR of one or more of the flags \c CL_AMS_PG_TRACK_CURRENT,
 *  \c CL_AMS_PG_TRACK_CHANGES or \c CL_AMS_PG_TRACK_CHANGES_ONLY.
 *  \param pNotificationBuffer pointer to a buffer of type
 *  ClAmsPGNotificationT. This parameter is ignored if \c
 *  CL_AMS_PG_TRACK_CURRENT is not set in \e trackFlags; otherwise, if
 *  \e notificationBuffer is not NULL, the buffer will contain
 *  information about all components in the protection group when
 *  clCpmProtectionGroupTrack() returns.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to request the framework to start tracking changes in the
 *  protection group associated with the component service instance, identified
 *  by \e csiName, or changes of attributes of any component in the protection
 *  group. These changes are notified via the invocation of the
 *  \e ClCpmProtectionGroupTrackCallbackT() callback function, which must have
 *  been supplied when the process invoked the \e clCpmClientInitialize call.
 *
 *  \note
 *  This API is equivalent to \e saAmfProtectionGroupTrack, which makes a
 *  synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmProtectionGroupTrackStop()
 *
 */
extern ClRcT clCpmProtectionGroupTrack(CL_IN ClCpmHandleT cpmHandle,
                                       CL_IN ClNameT *pCsiName,
                                       CL_IN ClUint8T trackFlags,
                                       CL_INOUT ClAmsPGNotificationBufferT
                                       *pNotificationBuffer);

/**
 ************************************
 *  \brief Stop tracking the protection group for the given CSI.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCsiName Pointer to the CSI for which protection group needs to
 *  be tracked.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  The invoking process requests the Availability Management Framework to
 *  stop tracking protection group changes for the component service instance
 *  designated by \e pCsiName.
 *
 *  \note
 *  This API is equivalent to \e saAmfProtectionGroupTrackStop, which makes a
 *  synchronous call.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmClientInitialize(), clCpmProtectionGroupTrack()
 *
 */
extern ClRcT clCpmProtectionGroupTrackStop(CL_IN ClCpmHandleT cpmHandle,
                                           CL_IN ClNameT *pCsiName);

/**
 * Other utility APIs exposed by CPM.
 */
    
/**
 ************************************
 *  \brief Returns the component ID of a component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param cpmHandle Handle returned by \e clCpmClientInitialize API.
 *  \param pCompName Name of the component.
 *  \param pCompId (out) Unique Id of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to return the component ID for a component identified
 *  by \e pCompName. This unique component ID is node internal and is not known
 *  to outside entities. Each time a component is instantiated, a new component
 *  ID is assigned to it.
 *
 *  \note This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmComponentIdGet(CL_IN ClCpmHandleT cpmHandle,
                                 CL_IN ClNameT *pCompName,
                                 CL_OUT ClUint32T *pCompId);

/**
 ************************************
 *  \brief Returns the IOC address of a component.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param nodeAddress Node IOC Address.
 *  \param pCompName Name of the component.
 *  \param pCompAddress IOC address of the component including port
 *  information.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to return the IOC address of a component identified by
 *  \e pCompName. This represents the physical address of the node where the
 *  CPM is running.
 *
 *  \note This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentStatusGet()
 *
 */
extern ClRcT clCpmComponentAddressGet(CL_IN ClIocNodeAddressT nodeAddress,
                                      CL_IN ClNameT *pCompName,
                                      CL_OUT ClIocAddressT *pCompAddress);

/**
 ************************************
 *  \brief Returns the component presence and operational state.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component.
 *  \param pNodeName IOC address of the component.
 *  \param pPresenceState Presence state of the component.
 *  \param pOperationalState Operational state of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to return the presence and operational state of a component
 *  identified by \e pCompName. The presence state of the component reflects the
 *  component life cycle and the operational state of the component is used by
 *  the AMF to determine whether a component is capable of taking component
 *  service instance assignments.
 *
 *  \note This is a synchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmComponentAddressGet()
 *
 */
extern ClRcT clCpmComponentStatusGet(CL_IN ClNameT *pCompName,
                                     CL_IN ClNameT *pNodeName,
                                     CL_OUT ClAmsPresenceStateT *pPresenceState,
                                     CL_OUT ClAmsOperStateT *pOperationalState);

/**
 ************************************
 *  \brief Returns the IOC address of the master.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pIocAddress (out) IOC address of the master.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to retrieve the IOC address of the master. The
 *  address returned by this API represents the physical address of
 *  the node where the CPM/G is running.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmIsMaster()
 *
 */
extern ClRcT clCpmMasterAddressGet(CL_OUT ClIocNodeAddressT *pIocAddress);
extern ClRcT clCpmMasterAddressGetExtended(CL_OUT ClIocNodeAddressT *pIocAddress,
                                           ClInt32T numRetries, 
                                           ClTimerTimeOutT *pDelay);

/**
 ************************************
 *  \brief Informs if the node is master of the cluster.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \par Parameters:
 *  None
 *
 *  \retval CL_TRUE The node is the master.
 *  \retval CL_FALSE The node is not the master.
 *
 *  \par Description:
 *  This API indicates if the node is master of the cluster. This API
 *  can be used by the component to determine whether it is running on
 *  the CPM master (CPM/G (global) or CPM/L (local)).
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 *  \sa clCpmMasterAddressGet()
 *
 */
extern ClUint32T clCpmIsMaster(void);

/**
 ************************************
 *  \brief Shuts down the node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param iocNodeAddress IOC address of the node.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to shut down the node, which corresponds to the given IOC
 *  Address. Shutting down the node terminates all the services  running
 *  on that node.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNodeShutDown(CL_IN ClIocNodeAddressT iocNodeAddress);

/**
 ************************************
 *  \brief Restarts the node
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param iocNodeAddress IOC address of the node.
 *  \param graceful Specifies whether to restart the node gracefully
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is used to restart the node, which corresponds to the
 *  given IOC Address. If graceful is non zero, then all the
 *  application components are terminated gracefully running on the
 *  node. If graceful is zero, applications are abruptly terminated,
 *  either by killing the applications on the node or rebooting the
 *  node. The node may or may not be restarted depending on the
 *  environment variables ASP_NODE_RESTART, ASP_NODE_REBOOT_DISABLE
 *  and ASP_RESTART_ASP.
 *
 *  This APIs behavior depends on the environment variables
 *  ASP_NODE_RESTART, ASP_NODE_REBOOT_DISABLE and ASP_RESTART_ASP,
 *  documented in the command line and environment variable reference
 *  section of the SDK guide.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNodeRestart(CL_IN ClIocNodeAddressT iocNodeAddress,
                              CL_IN ClBoolT graceful);

/**
 ************************************
 *  \brief Switch over the node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param iocNodeAddress IOC address of the node.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *
 *  This API is used to switch over the node, which corresponds to the
 *  given IOC Address. All the application components running on the
 *  node are switched over and terminated gracefully and the node is
 *  shutdown. The node may or may not be restarted depending on the
 *  environment variables ASP_NODE_RESTART, ASP_NODE_REBOOT_DISABLE
 *  and ASP_RESTART_ASP.
 *
 *  This APIs behavior depends on the environment variables
 *  ASP_NODE_RESTART, ASP_NODE_REBOOT_DISABLE and ASP_RESTART_ASP,
 *  documented in the command line and environment variable reference
 *  section of the SDK guide.
 *
 *  \note This is an asynchronous API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNodeSwitchover(CL_IN ClIocNodeAddressT iocNodeAddress);

/*
 * This API is the same as clCpmNodeRestart. However with a few differences.
 * The middleware restart reboots the node or restarts asp if nodeReset flag is CL_FALSE.
 * It works irrespective of the environment variables that one can use to override the node reset behavior.
 * If you want to get environment variable behavior for node resets, then clCpmNodeRestart is the API to use.
 */

extern ClRcT clCpmMiddlewareRestart(ClIocNodeAddressT iocNodeAddress, ClBoolT graceful, ClBoolT nodeReset);

/**
 ************************************
 *  \brief Returns the name of the local node.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param nodeName Local node name.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing the unallocated node Name.
 *  \retval CL_ERR_DOESNT_EXIST CPM library is not able to retrieve the
 *  \e nodeName.
 *
 *  \par Description:
 *  This API provides the local node Name, in the buffer provided by the user.
 *  This API can be used by a component to determine the name of the node
 *  on which it is running.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmLocalNodeNameGet(CL_IN ClNameT *nodeName);




/**
 ************************************
 *  \brief Returns the status of any component in a system.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param compAddr The physical address of a component, whose status is to be know.
 *  \param pStatus  The pointer to a variable to hold the status of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER If an invalid address is passed.
 *  \e nodeName.
 *
 *  \par Description:
 *  This API should be used to get the status of a component present on any
 *  ASP-node in a system. The *pStatus will contain CL_STATUS_UP or
 *  CL_STATUS_DOWN if the component is up or down respectively.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmCompStatusGet(ClIocAddressT compAddr, ClStatusT *pStatus);


/**
 ************************************
 *  \brief Returns the status of any ASP node in a system.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param compAddr The ASP node address, whose status is to be know.
 *  \param pStatus  The pointer to a variable to hold the status of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API should be used to get the status of an ASP node
 *  in a system. The *pStatus will contain CL_STATUS_UP if that particular ASP
 *  node is up and CL_STATUS_DOWN if the node is down.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNodeStatusGet(ClIocNodeAddressT nodeAddr, ClStatusT *pStatus);


/**
 ************************************
 *  \brief This API installs the callback function when the node/component
 *  arrival/departure events are to be intemated asynchronously.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param compAddr The ASP component address, whose arrival/departure is of
 *  importance. If the callback is for a node's arrival/departure then the 
 *  \e compAddr.portId should be 0.
 *  \param pFunc The callback function pointer, which will get called on
 *  arrival/departure event.
 *  \param pArg The argument which will be passed back in the callback.
 *  \param pHandle The pointer to a varible in which a handle will be
 *  returned, which should used to unistall a function installed.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY If there is no memory to register the callback.
 *
 *  \par Description:
 *  This API should be used, when a node's or a component's arrival/departure event
 *  is of importance and some action needs to be taken depending on the event
 *  occured. This API will register the callback function and the callback
 *  will be invoked when node/component arrival/departure event occurs. 
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNotificationCallbackInstall(ClIocPhysicalAddressT compAddr, ClCpmNotificationFuncT pFunc, ClPtrT pArg, ClHandleT *pHandle);



/**
 ************************************
 *  \brief The API uninstalls the callback, which would have been installed
 *  through the \e clCpmNotificationCallbackInstall API.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pHandle The handle for unistalling the callback, returned by
 *  \e clCpmNotificationCallbackInstall API.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API should be used, when the callback installed through
 *  \e clCpmNotificationCallbackInstall API, is to be uninstalled. The handle
 *  passed to this API should be the correct one as this API doent do any
 *  kind of verification on the handle.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmNotificationCallbackUninstall(ClHandleT *pHandle);


/**
 ************************************
 *  \brief The API gets the comp CSI List from the component CSI cache
 *  maintained per-process.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \param pCompName Name of the component to query to obtain the CSI list
 *  \param pCSIRef   List of CSI references cached for the component 
 *  is returned through this variable
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER The API was invoked with invalid arguments
 *
 *  \par Description:
 *  This API should be used to obtain the cached CSI list for a component
 *  The csi list is returned through the pCSIRef argument. The API returns
 *  the cached haState and the csiDescriptor for the component for each CSI. 
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmCompCSIList(const ClNameT *pCompName, ClCpmCompCSIRefT *pCSIRef);

/**
 ************************************
 *  \brief The API returns whether this node is system controller or not.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \retval CL_YES If the current node is system controller.
 *  \retval CL_NO If the current node is not system controller
 *  (i.e. it is a payload node) or the application calling this API is
 *  running out side of AMF for e.g. using safplus_run script.
 *
 *  \par Description:
 *  This API should be used by the application to check if the node on
 *  which it is running is system controller.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClBoolT clCpmIsSC(void);

/**
 ************************************
 *  \brief The API returns component specific information.
 *
 *  \par Header File:
 *  clCpmApi.h
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER The API was invoked with invalid arguments.
 *  \retval CL_ERR_NO_MEMORY Not enough memory to complete the request.
 *
 *  \par Description:
 *  This API should be used by the application to fetch any component specific
 *  information maintained by CPM.
 *
 *  \note This API allocates a array of pointers as well as memory for
 *  each of the components argument for \arg args. It is the
 *  responsibility of the user to free the memory allocated by this
 *  API.
 *
 *  \par Library Files:
 *  ClAmfClient
 *
 */
extern ClRcT clCpmCompInfoGet(const ClNameT *compName,
                              const ClIocNodeAddressT nodeAddress,
                              CL_OUT ClCpmCompSpecInfoT *compInfo);

/** This component's handle */
extern ClCpmHandleT clCpmHandle;


#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CPM_API_H_ */

/** \} */
