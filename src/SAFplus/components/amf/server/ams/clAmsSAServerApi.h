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
 * ModuleName  : amf
 * File        : clAmsSAServerApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS Service
 * Availability API.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_SA_SERVER_API_H_
#define _CL_AMS_SA_SERVER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <clAmsTypes.h>
#include <clAmsSACommon.h>
#include <clCpmApi.h>
#include <clEoApi.h>

#include <clCpmAms.h>
#include <clAms.h>

/*
 * _clAmsSACSIHAStateGet 
 * ----------------
 * return HA state of a component for a given CSI
 *
 * @param
 *  compName                   - Name of the component whose HA state is is
                                 requested 
 *  csiName                    - Name of the CSI
 *  haState                    - HA state of the component for the CSI 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSACSIHAStateGet( 
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *csiName,
        CL_OUT  ClAmsHAStateT  *haState );

/*
 * _clAmsSAPGTrackStop 
 * ----------------
 * Remove the protection group client from the PG list 
 *
 * @param
 *   iocAddress                 - ioc address of the client 
 *   cpmHandle                  - cpm initialization handle 
                                  to invoke the client callback
 *   csiName                    - name of the csi which contains the PG client 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAPGTrackStop( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  SaNameT  *csiName);

/*
 * _clAmsSAPGTrackAdd 
 * ----------------
 * Add a client in the protection group list of a CSI
 *
 * @param
 *   iocAddress                 - ioc address of the client 
 *   cpmHandle                  - cpm initialization handle 
                                  to invoke the client callback
 *   csiName                    - name of the csi which contains the PG client 
 *   trackFlags                 - tracking flags of the client
 *   notificationBuffer         - Buffer in which the response is sent back
                                  if the track flags is CL_AMS_PG_TRACK_CURRENT
                                  and this buffer is non null
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAPGTrackAdd( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  SaNameT  *csiName,
        CL_IN  ClUint8T  trackFlags,
        CL_INOUT  ClAmsPGNotificationBufferT  *notificationBuffer);

/*
 * _clAmsSAMarshalPGTrackDispatch
 * ----------------
 * This function marshals the pg track response to be sent to pg clients
 *
 * @param
 *  csi                     - csi whose PG has changed 
 *  comp                    - component name whose HA state has changed
 *  pgChange                - change in the protection group
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAMarshalPGTrackDispatch( 
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClAmsCompT  *comp, 
        CL_IN  ClAmsPGChangeT  pgChange);

/*
 * _clAmsSAPGTrackDispatch
 * ----------------
 * AMS to CPM callout to dispatch the response to pg clients interested in the
 * protection group of the CSI

 @param
 * iocAddress               - ioc address of the pg track client
 * cpmHandle                - cpm library handle
 * csi                      - csi whose PG has changed 
 * buffer                   - protection group notification buffer
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAPGTrackDispatch( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  SaNameT  *csiName,
        CL_IN  ClAmsPGNotificationBufferT  *buffer );

/*
 * _clAmsSACSISet
 * ----------------
 * This function requests CPM to assign HA state to a component for a given CSI
 * on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                assigned the CSI 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 * invocation                 - invocation ID for the CSI set operation
 * haState                    - ha state assigned to the component 
 * csiDescriptor              - csi descriptor for the CSI
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSACSISet(
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsHAStateT  haState,
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor );

/*
 * Used for CSI replays while standby is transitioning to master.
 */
extern ClRcT
_clAmsSACSISetNoCkpt(
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsHAStateT  haState,
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor );

/*
 * _clAmsSACSIRemove
 * ----------------
 * This function requests CPM to remove CSI assignment from the component
 * on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                assigned the CSI 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 * invocation                 - invocation ID for the CSI set operation
 * csiDescriptor              - csi descriptor for the CSI
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSACSIRemove(
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor );


/*
 * _clAmsSACSIQuiescingComplete
 * ----------------
 * CPM callback to inform AMS about CSI quiescing completion
 *
 * @param
 *  invocation                 - invocation for csi quiescing request 
 *  retCode                    - return code from the client to inform about
                                 success / failure of the csi operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSACSIQuiescingComplete( 
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClRcT  retCode );

/*
 * _clAmsSACSIOperationResponse
 * ----------------
 * CPM callback to inform AMS about CSI operation completion
 *
 * @param
 *  invocation                 - invocation for csi operation request 
 *  retCode                    - return code from the client to inform about
                                 success / failure of the csi operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSACSIOperationResponse(
        CL_IN  ClInvocationT  invocationID,
        CL_IN  ClRcT  retCode );

/*
 * _clAmsSAComponentErrorReport
 * ----------------
 * CPM callback to inform AMS about error reported for a component 
 *
 * @param
 *  compName                   - Component name on which error in reported 
 *  errorDetectionTime         - time stamp of error detection
 *  recommendedRecovery        - recovery action recommended by the reportee
 *  alarmHandle                - alram handle returned by alarm raise for this
                                 error. This is used by fault manager / alarm
                                 manager to get the information about the error                                  / fault when AMS informs it about the recovery
                                 action taken.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAComponentErrorReport (
        CL_IN  const SaNameT  *compName,
        CL_IN  ClTimeT  errorDetectionTime,
        CL_IN  ClAmsLocalRecoveryT  recommendedRecovery,
        CL_IN  ClUint32T  alarmHandle,
        CL_IN  ClUint64T  instantiateCookie);

/*
 * _clAmsSAFaultReportCallback
 * ----------------
 * This function marshals the response to fault manager 
 *
 * @param
 *  entity                  - entity on which the fault was reported 
 *  recovery                - recovery action undertaken by AMF
 *  repairNecessary         - indication from AMS whether repair is necessary
 *
 * @returns
 *   CL_OK                  - Operation successful
 *
 */

extern ClRcT
_clAmsSAFaultReportCallback(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsRecoveryT  recovery,
        CL_IN  ClUint32T  repairNecessary );


/*
 * _clAmsSANodeFailFast
 * ----------------
 * This is AMS callout to inform CPM that fail fast / reboot operation should be
 * performed on the faulty node
 * @param
 *  nodeName                  - Name of the node which needs to be rebooted 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT _clAmsSANodeFailFast(
                                  CL_IN  SaNameT  *nodeName,
                                  CL_IN  ClBoolT isASPAware);

/*
 * _clAmsSANodeFailOver
 * ----------------
 * This is AMS callout to inform CPM that node failover has occured 

 * @param
 *  nodeName                  - Name of the node which needs to be rebooted 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT _clAmsSANodeFailOver(
                                  CL_IN   SaNameT *nodeName,
                                  CL_IN   ClBoolT isASPAware);

extern ClRcT _clAmsSANodeFailOverRestart(
                                         CL_IN   SaNameT *nodeName,
                                         CL_IN   ClBoolT isASPAware);

/*
 * _clAmsSAComponentInstantiate
 * ----------------
 * This function requests CPM to instantiate a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                instantiated
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAComponentInstantiate(
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *proxyCompName,
        CL_IN  ClUint64T instantiateCookie );


/*
 * _clAmsSAProxiedComponentInstantiate
 * ----------------
 * This function requests CPM to instantiate a proxied component on behalf of 
 * AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                instantiated
 * invocation                 - invocation ID for the instantiate operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */
extern ClRcT
_clAmsSAProxiedComponentInstantiate(
        CL_IN   SaNameT             *compName,
        CL_IN   ClInvocationT       invocation);

/*
 * _clAmsSAComponentTerminate
 * ----------------
 * This function requests CPM to terminate a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                terminated 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAComponentTerminate(
        CL_IN  SaNameT  *compName,
        CL_IN  SaNameT  *proxyCompName );

/*
 * _clAmsSAComponentCleanup
 * ----------------
 * This function requests CPM to cleanup a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                cleaned 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAComponentCleanup(
        CL_IN  SaNameT  *compName, 
        CL_IN  SaNameT  *proxyCompName );

/*
 * _clAmsSAProxiedComponentCleanup
 * ----------------
 * This function requests CPM to cleanup a proxied component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                cleanup 
 * invocation                 - invocation ID for the cleanup operation
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAProxiedComponentCleanup(
       CL_IN    SaNameT             *compName,
       CL_IN    ClInvocationT       invocation );

/*
 * _clAmsSAComponentRestart
 * ----------------
 * This function requests CPM to restart a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                restarted 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
_clAmsSAComponentRestart(
        CL_IN   SaNameT             *compName );

/*
 * _clAmsSAComponentOperationResponse
 * ----------------
 * CPM callback to inform AMS about Component operation completion
 *
 * @param
 *  compName                  - Name of the component on which the operation
                                was performed
 *  requestType               - type of operation performed on the component
 *  retCode                    - return code from the client to inform about
                                 success / failure of the component operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT 
_clAmsSAComponentOperationResponse(
        CL_IN  SaNameT  compName,
        CL_IN  ClCpmCompRequestTypeT  requestType,
        CL_IN  ClRcT  retCode );

/*
 * _clAmsSAEntityAdd
 * ----------------
 * This function requests CPM to add a component/node to its table.
 *
 * @param
 *  entityRef                   - Component/node which needs to be added
 *                                to CPM table.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT _clAmsSAEntityAdd(CL_IN ClAmsEntityRefT *entityRef);

/*
 * _clAmsSAEntityRmv
 * ----------------
 * This function requests CPM to remove a component/node from its table.
 *
 * @param
 *  entityRef                   - Component/node which needs to be removed
 *                                from CPM table.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT _clAmsSAEntityRmv(CL_IN ClAmsEntityRefT *entityRef);
    
/*
 * _clAmsSAEntitySetConfig
 * ----------------
 * This function requests CPM to set a component/node configuration
 * in its table.
 *
 * @param
 *  entityRef                     - Component/node for which configuration
 *                                needs to be set.
 *  bitMask                       - Bit mask specifying which attrs to set.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT _clAmsSAEntitySetConfig(CL_IN ClAmsEntityConfigT *entityConfig,
                                     CL_IN ClUint64T bitMask);

/*
 * _clAmsSANodeJoin
 * ----------------
 * This is CPM callout to inform AMS about the node arrival event 
 *
 * @param
 *  nodeName                  - Name of the node which has joined the cluster 

 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT 
_clAmsSANodeJoin(
        CL_IN  SaNameT  *nodeName );

/*
 * _clAmsSANodeLeave
 * ----------------
 * This is CPM callout to inform AMS about the node departure event 
 *
 * @param
 *  nodeName                  - Name of the node which has left/leaving the 
                                cluster 
 *  request                   - indication whether the node is leaving or
                                already left the cluster

 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT 
_clAmsSANodeLeave(
        CL_IN  SaNameT  *nodeName,
        CL_IN  ClCpmNodeLeaveT  request,
        CL_IN  ClBoolT scFailover);

/*
 * _clAmsSANodeLeaveCompleted
 * ----------------
 * This is AMS callout to inform CPM that switchover for the node is complete
 * and the node can be shutdown 
 *
 * @param
 *  nodeName                  - Name of the node which had requested shutdown
                                operation 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT 
_clAmsSANodeLeaveCompleted(
                           CL_IN  SaNameT  *nodeName,
                           ClCpmNodeLeaveT nodeLeave);

/*
 * _clAmsSACkptServerReady
 * ----------------
 * CPM to AMS callout to inform AMS that ckpt server is up and running. It
 * also informs the AMS about the checkpoint library initialization handle 
 * for AMS use.

 @param
 * ckptInitHandle           - ckpt library initialization handle 
 * mode                     - AMS Instantiate mode ( Active / Standby )
 *
 * @returns
 *   CL_OK                  - Operation successful
 *
 */

extern ClRcT
_clAmsSACkptServerReady(
        CL_IN  ClCkptHdlT  ckptInitHandle,
        CL_IN  ClUint32T  mode );

/*
 * _clAmsSAEventServerReady
 * ----------------
 * CPM to AMS callout to inform AMS about the availability of event server.
 * This callback will be called when-ever event server changes state from
 * available to non-available and vice vera.
 *
 * @param
 * eventServerReady         - event server is available / not available
 *
 * @returns
 *  CL_OK                   - Operation successful
 * 
 */

extern ClRcT
_clAmsSAEventServerReady(
        CL_IN  ClBoolT  eventServerReady );


extern ClRcT
_clAmsSACorServerReady(void);

extern ClRcT
_clAmsSANodeAdd(const ClCharT *node);

extern ClRcT
_clAmsSANodeRestart(const SaNameT *nodeName, ClBoolT graceful);

extern ClRcT
_clAmsSANodeHalt(SaNameT *nodeName, ClBoolT aspAware, ClBoolT ckpt);

extern ClRcT
_clAmsSAClusterReset(ClAmsT *ams);

/*
 * _clAmsSAAmsStateChange
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * its state from Active to Standby or Standby to Active 

 @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAAmsStateChange( ClUint32T mode );

/*
 * _clAmsSAStateChangeActive2Standby
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * state from Active to Standby. AMS running on this controller card should 
 * no longer service the requests and stop all the current operations. 

 @param
 *
 *  mode                      - mode to inform type of transition
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAStateChangeActive2Standby(
        ClUint32T  mode);


/*
 * _clAmsSAStateChangeStandby2Active
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * state from standby to active. AMS running on this controller card should 
 * start servicing the requests. This callout is valid in a situation when 
 * Active system controller was made standby and is now again made active 

 @param
 *
 *  mode                      - mode to inform type of transition
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAStateChangeStandby2Active(
        ClUint32T  mode);


/*
 * _clAmsSAStateChangeReset
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card configuration
 * should be reset 

 @param
 *
 *  mode                      - mode to inform type of transition
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAStateChangeReset(
        ClUint32T  mode );

/*
 * Utility functions for testing / debugging 
 */

/*
 * _clAmsSAPrintDB
 * ----------------
 * This is a debug utility function to print the ams database 
 *
 * @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAPrintDB (void);

/*
 * _clAmsSAPrintDBXML
 * ------------------
 * This is a debug utility function to print the ams database
 * in XML format
 *
 * @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
_clAmsSAPrintDBXML (void);

/*
 * _clAmsSAEntityPrint
 * ----------------
 * This is a debug utility function to print contents of an AMS entity 
 * @param

 * entity                 -  AMS entity name whose contents needs to be printed
 *
 * @returns
 *   CL_OK                - Operation successful

 */

extern ClRcT
    _clAmsSAEntityPrint (
            CL_IN  ClAmsEntityT *entity );

/*
 * _clAmsSAXMLizeInvocation
 * ----------------
 * Utility function to dump the AMS invocation into a XML file which is checkpointed

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

extern ClRcT
_clAmsSAXMLizeInvocation(void);

/*
 * _clAmsSADeXMLizeInvocation
 * ----------------
 * Utility function to read the dumped ams invocation XML file and contruct ams 
 * invocation list from it.

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

extern ClRcT
_clAmsSADeXMLizeInvocation(void);

/*
 * _clAmsSADeXMLizeDB
 * ----------------
 * Utility function to read the dumped ams db file and contruct ams 
 * db from it.

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

extern ClRcT
_clAmsSADeXMLizeDB(void);

/*
 * _clAmsSAXMLizeDB
 * ----------------
 * Utility function to dump the AMS db into a XML file which is checkpointed

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

extern ClRcT
_clAmsSAXMLizeDB(void);


/*
 * _clAmsSAMarshalRmdParams
 * ----------------
 * This function marshals the paramter required for AMS to make call to CPM 
 * like node name and ioc address
 *
 * @param
 *  compName                  - Name of the component on which CPM will perform
                                operation
 *  nodeName                  - node Name for the component
 *  physAddr                  - physical address for the component
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */


extern ClRcT
_clAmsSAMarshalRmdParams(
        CL_IN   SaNameT                 *compName,
        CL_OUT  ClCharT                 **nodeName,
        CL_OUT  ClIocPhysicalAddressT   *physAddr );

/*
 * _clAmsSAEventServerTest
 * ----------------
 *
 * Utility function to test event communication

 @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */


extern ClRcT
_clAmsSAEventServerTest();

#ifdef __cplusplus
}
#endif

#endif // #ifndef _CL_AMS_SA_SERVER_API_H_

