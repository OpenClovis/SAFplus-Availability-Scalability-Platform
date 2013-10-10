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
 * This header file contains CPM client side related definitions.
 */

#ifndef _CL_CPM_CLIENT_H_
#define _CL_CPM_CLIENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clIocApi.h>
#include <clCpmApi.h>
#include <clEoApi.h>

/*
 * CPM internal header files
 */
#include <clCpmIpi.h>    
#include <clCpmCommon.h>
#include <clVersion.h>

#define CL_LOG_SP(...) __VA_ARGS__
#define CPM_CLIENT_CHECK(X, Z, retCode)         \
    if (retCode != CL_OK)                       \
    {                                           \
        char __tempstr[256];                    \
        snprintf(__tempstr,256,CL_LOG_SP Z);    \
        clLog(X,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,__tempstr);                \
        rc = retCode;                           \
        goto failure;                           \
    }

#define CL_CPM_CLIENT_DFLT_TIMEOUT      CL_RMD_DEFAULT_TIMEOUT
#define CL_CPM_CLIENT_DFLT_RETRIES      CL_RMD_DEFAULT_RETRIES
#define CL_CPM_CLIENT_DFLT_PRIORITY     CL_IOC_DEFAULT_PRIORITY

/**
 * The function ids for the RMD functions of CPM.
 */

/**
 * The CPM-EO functionality. Most of it is obsolete and has been
 * removed. Should go away.
 */
#define COMPO_REGIS_EO      			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)
#define COMPO_QUERY_EO      			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)
#define COMPO_QUERY_EOIDS   			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3)
#define COMPO_FUNC_WALK_EOS 			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4)
#define COMPO_MGR_RL_EVT    			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 5)
#define COMPO_MGR_STRT_POLL 			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 6)
#define COMPO_UPD_EOINFO    			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 7)
#define COMPO_SET_STATE     			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 8)
#define COMPO_EOLIST_SHOW   			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 9)
#define COMPO_EO_PING       			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 10)
#define COMPO_RMDSTATS_GET  			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 11)

/**
 * Component life cycle management/SAF related functionality.
 */
#define CPM_COMPONENT_INITIALIZE			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 12)
#define CPM_COMPONENT_FINALIZE				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 13)
#define CPM_COMPONENT_REGISTER				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 14)
#define CPM_COMPONENT_UNREGISTER			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 15)
#define CPM_COMPONENT_HEALTHCHECK_START		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 16)
#define CPM_COMPONENT_HEALTHCHECK_STOP		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 17)
#define CPM_COMPONENT_HEALTHCHECK_CONFIRM	CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 18)
#define CPM_COMPONENT_INSTANTIATE			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 19)
#define CPM_COMPONENT_TERMINATE				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 20)
#define CPM_COMPONENT_CLEANUP				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 21)
#define CPM_COMPONENT_RESTART				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 22)
#define CPM_COMPONENT_EXTN_HEALTHCHECK		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 23)
#define CPM_COMPONENT_ID_GET				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 24)
#define CPM_COMPONENT_LA_UPDATE				CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 25)
#define CPM_COMPONENT_PID_GET			    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 26)
#define CPM_COMPONENT_STATUS_GET			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 27)
#define CPM_COMPONENT_DEBUG_LIST_GET		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 28)
#define CPM_COMPONENT_ADDRESS_GET			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 29)
#define CPM_COMPONENT_MOID_GET			    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 30)
#define CPM_COMPONENT_INFO_GET                  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 31)    

/**
 * SAF related functionality.
 */
#define CPM_COMPONENT_CSI_ASSIGN	        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 39)
#define CPM_COMPONENT_CSI_RMV		        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 40)
#define CPM_CB_RESPONSE                     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 41)
#define CPM_COMPONENT_PGTRACK		        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 42)
#define CPM_COMPONENT_PGTRACK_STOP	        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 43)
#define CPM_COMPONENT_HA_STATE_GET			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 44)
#define CPM_COMPONENT_QUIESCING_COMPLETE	CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 45)
#define CPM_COMPONENT_FAILURE_REPORT		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 46)
#define CPM_COMPONENT_FAILURE_CLEAR			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 47)

/**
 * Misc.
 */
#define CPM_COMPONENT_IS_COMP_RESTARTED		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 48)
#define CPM_COMPONENT_LCM_RES_HANDLE		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 49)
#define CPM_RESET_ELSE_COMMIT_SUICIDE		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 50)

/**
 * CPM-CPM related functionality.
 */
#define CPM_CPML_REGISTER		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 51)
#define CPM_CPML_DEREGISTER	    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 52)
#define CPM_CPML_CONFIRM		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 53)

/**
 * Boot manager functionality. Should go away.
 */
#define CPM_BM_GET_CURRENT_LEVEL	CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 54)
#define CPM_BM_SET_LEVEL			CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 55)
#define CPM_BM_GET_MAX_LEVEL		CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 56)

#define CPM_MGMT_NODE_CONFIG_SET    CL_EO_GET_FULL_FN_NUM(CL_CPM_MGMT_CLIENT_TABLE_ID, 0)
#define CPM_MGMT_NODE_CONFIG_GET    CL_EO_GET_FULL_FN_NUM(CL_CPM_MGMT_CLIENT_TABLE_ID, 1)
#define CPM_MGMT_NODE_RESTART       CL_EO_GET_FULL_FN_NUM(CL_CPM_MGMT_CLIENT_TABLE_ID, 2)
#define CPM_MGMT_MIDDLEWARE_RESTART CL_EO_GET_FULL_FN_NUM(CL_CPM_MGMT_CLIENT_TABLE_ID, 3)
#define CPM_MGMT_COMP_CONFIG_SET    CL_EO_GET_FULL_FN_NUM(CL_CPM_MGMT_CLIENT_TABLE_ID, 4)

/**
 * Misc.
 */
#define CPM_NODE_CPML_RESPONSE		    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 57)
#define CPM_PROC_NODE_SHUTDOWN_REQ	    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 58)
#define CPM_NODE_SHUTDOWN			    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 59)
#define CPM_CM_NODE_ARRIVAL_DEPARTURE	CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 60)
#define CPM_SLOT_INFO_GET               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 61)
#define CPM_NODE_IOC_ADDR_GET           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 62)
#define CPM_NODE_GO_BACK_TO_REG         CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 63)

#define CPM_FUNC_ID(n) CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, n)    


typedef struct compMgrClientInfo
{
    ClUint32T version;
    ClEoExecutionObjT eoObj;
    ClUint32T flag;
    ClIocNodeAddressT pingAddr;
    ClEoIdT eoId;
    ClIocPortT eoPort;
    ClEoStateT state;
    ClCpmFuncWalkT walkInfo;
    SaNameT compName;
}ClCpmClientInfoT;

/**
 * Actual RMD service functions representing above functions ids.
 */

/**
 * The CPM-EO functionality. Most of it is obsolete and has been
 * removed. Should go away.
 */
ClRcT VDECL(compMgrEORegister)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrFuncEOWalk)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrEOStateUpdate)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrEOStateSet)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrEOPing)(ClEoDataT data,
                           ClBufferHandleT inMsgHandle,
                           ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrEOListShow)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(compMgrEORmdStats)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

/**
 * Component life cycle management/SAF related functionality.
 */
ClRcT VDECL(cpmClientInitialize)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmClientFinalize)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentRegister)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentUnregister)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentHealthcheckStart)(ClEoDataT data,
                                          ClBufferHandleT inMsgHandle,
                                          ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentHealthcheckStop)(ClEoDataT data,
                                         ClBufferHandleT inMsgHandle,
                                         ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentHealthcheckConfirm)(ClEoDataT data,
                                            ClBufferHandleT inMsgHandle,
                                            ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentInstantiate)(ClEoDataT data,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentTerminate)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentCleanup)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentRestart)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentIdGet)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentLAUpdate)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentPIDGet)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentStatusGet)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentListDebugAll)(ClEoDataT data,
                                      ClBufferHandleT inMsgHandle,
                                      ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentAddressGet)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentMoPathGet)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentInfoGet)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

/**
 * SAF related functionality.
 */
ClRcT VDECL(cpmComponentCSISet)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle);


ClRcT VDECL(cpmComponentCSIRmv)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmCBResponse)(ClEoDataT data,
                           ClBufferHandleT inMsgHandle,
                           ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmPGTrack)(ClEoDataT data,
                        ClBufferHandleT inMsgHandle,
                        ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmPGTrackStop)(ClEoDataT data,
                            ClBufferHandleT inMsgHandle,
                            ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentHAStateGet)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmQuiescingComplete)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentFailureReport)(ClEoDataT data,
                                       ClBufferHandleT inMsgHandle,
                                       ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmComponentFailureClear)(ClEoDataT data,
                                      ClBufferHandleT inMsgHandle,
                                      ClBufferHandleT outMsgHandle);

/**
 * Misc.
 */
ClRcT VDECL(cpmIsCompRestarted)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmAspCompLcmResHandle)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmResetNodeElseCommitSuicideRmd)(ClEoDataT data,
                                              ClBufferHandleT inMsgHandle,
                                              ClBufferHandleT outMsgHandle);

/**
 * CPM-CPM related functionality.
 */
ClRcT VDECL(cpmCpmLocalRegister)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmCpmLocalDeregister)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmCpmConfirm)(ClEoDataT data,
                           ClBufferHandleT inMsgHandle,
                           ClBufferHandleT outMsgHandle);

/**
 * Boot manager functionality. Should go away.
 */
ClRcT VDECL(cpmBootLevelGet)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmBootLevelSet)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmBootLevelMax)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle);

/**
 * Misc.
 */
ClRcT VDECL(cpmNodeCpmLResponse)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmProcNodeShutDownReq)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmNodeShutDown)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmNodeArrivalDeparture)(ClEoDataT data,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmSlotInfoGet)(ClEoDataT data,
                            ClBufferHandleT inMsgHandle,
                            ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmIocAddressForNodeGet)( ClEoDataT data, 
                                      ClBufferHandleT inMsgHandle,
                                      ClBufferHandleT outMsgHandle);

ClRcT VDECL(cpmGoBackToRegisterCallback)(ClEoDataT data,
                                         ClBufferHandleT inMsgHandle,
                                         ClBufferHandleT outMsgHandle);

/**
 * Client side RMD wrappers for CPM.
 */

ClRcT clCpmClientRMDSync(ClIocNodeAddressT destAddr,
                         ClUint32T fcnId,
                         ClUint8T *pInBuf,
                         ClUint32T inBufLen,
                         ClUint8T *pOutBuf,
                         ClUint32T *pOutBufLen,
                         ClUint32T flags,
                         ClUint32T timeOut,
                         ClUint32T maxRetries,
                         ClUint8T priority);

ClRcT clCpmClientRMDSyncNew(ClIocNodeAddressT destAddr,
                            ClUint32T fcnId,
                            ClUint8T *pInBuf,
                            ClUint32T inBufLen,
                            ClUint8T *pOutBuf,
                            ClUint32T *pOutBufLen,
                            ClUint32T flags,
                            ClUint32T timeOut,
                            ClUint32T maxRetries,
                            ClUint8T priority,
                            ClCpmMarshallT marshallFunction,
                            ClCpmUnmarshallT unmarshallFunction);

ClRcT clCpmClientRMDAsync(ClIocNodeAddressT destAddr,
                          ClUint32T fcnId,
                          ClUint8T *pInBuf,
                          ClUint32T inBufLen,
                          ClUint8T *pOutBuf,
                          ClUint32T *pOutBufLen,
                          ClUint32T flags,
                          ClUint32T timeOut,
                          ClUint32T maxRetries,
                          ClUint32T priority);

ClRcT clCpmClientRMDAsyncNew(ClIocNodeAddressT destAddr,
                             ClUint32T fcnId,
                             ClUint8T *pInBuf,
                             ClUint32T inBufLen,
                             ClUint8T *pOutBuf,
                             ClUint32T *pOutBufLen,
                             ClUint32T flags,
                             ClUint32T timeOut,
                             ClUint32T maxRetries,
                             ClUint32T priority,
                             ClCpmMarshallT marshallFunction);

extern void clCpmCompTerminateSet(ClBoolT doTerminate);    

extern ClRcT VDECL(cpmNodeConfigSet)(ClEoDataT data,
                                     ClBufferHandleT inMsgHdl,
                                     ClBufferHandleT outMsgHdl);

extern ClRcT VDECL(cpmNodeConfigGet)(ClEoDataT data,
                                     ClBufferHandleT inMsgHdl,
                                     ClBufferHandleT outMsgHdl);

extern ClRcT VDECL(cpmNodeRestart)(ClEoDataT data,
                                   ClBufferHandleT inMsgHdl,
                                   ClBufferHandleT outMsgHdl);

extern ClRcT VDECL(cpmMiddlewareRestart)(ClEoDataT data,
                                         ClBufferHandleT inMsgHdl,
                                         ClBufferHandleT outMsgHdl);

extern ClRcT VDECL_VER(cpmCompConfigSet, 5, 1, 0)(ClEoDataT data,
                                                  ClBufferHandleT inMsgHdl,
                                                  ClBufferHandleT outMsgHdl);

extern ClRcT clCpmClientTableRegister(ClEoExecutionObjT *eo);

#ifdef __cplusplus
}
#endif

#endif	/* _CL_CPM_CLIENT_H_ */
