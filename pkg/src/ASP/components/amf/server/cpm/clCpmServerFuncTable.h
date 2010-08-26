#ifndef _CL_CPM_SERVER_FUNC_TABLE_H_
#define _CL_CPM_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(cpmNativeFuncList)[] = 
{
    /*
     * Fix functions 
     */
    
    VSYM_EMPTY(NULL, CPM_FUNC_ID(0)),                                       /* 0 */
    
    /*
     * CPM-EO interaction functions
     */
    
    VSYM(compMgrEORegister, COMPO_REGIS_EO),      /* 1 */
    VSYM_EMPTY(NULL, COMPO_QUERY_EO) /* compMgrOneEOGet */ ,                /* 2 */
    VSYM_EMPTY(NULL, COMPO_QUERY_EOIDS) /* compMgrEOIDsGet */ ,                /* 3 */
    VSYM(compMgrFuncEOWalk, COMPO_FUNC_WALK_EOS), /* 4 */
    VSYM_EMPTY(NULL, COMPO_MGR_RL_EVT) /* compMgrRLEventHandle */ ,           /* 5 */
    VSYM_EMPTY(NULL, COMPO_MGR_STRT_POLL) /* compMgrPollingStart */ ,            /* 6 */
    VSYM(compMgrEOStateUpdate, COMPO_UPD_EOINFO),                       /* 7 */
    VSYM(compMgrEOStateSet, COMPO_SET_STATE),                          /* 8 */
    VSYM_EMPTY(NULL, COMPO_EOLIST_SHOW)/* compMgrEOListShow */ ,              /* 9 */
    VSYM_EMPTY(NULL, COMPO_EO_PING)/* compMgrEOPing */ ,                  /* 10 */
    VSYM_EMPTY(NULL, COMPO_RMDSTATS_GET)/* compMgrEORmdStats */ ,              /* 11 */
    
    /*
     * SAF functions
     */
    
    VSYM(cpmClientInitialize, CPM_COMPONENT_INITIALIZE),                        /* 12 */
    VSYM(cpmClientFinalize, CPM_COMPONENT_FINALIZE),                          /* 13 */
    VSYM(cpmComponentRegister, CPM_COMPONENT_REGISTER),                       /* 14 */
    VSYM(cpmComponentUnregister, CPM_COMPONENT_UNREGISTER),                     /* 15 */
    VSYM(cpmComponentHealthcheckStart, CPM_COMPONENT_HEALTHCHECK_START),               /* 16 */
    VSYM(cpmComponentHealthcheckStop, CPM_COMPONENT_HEALTHCHECK_STOP),                /* 17 */
    VSYM(cpmComponentHealthcheckConfirm, CPM_COMPONENT_HEALTHCHECK_CONFIRM),             /* 18 */
    VSYM(cpmComponentInstantiate, CPM_COMPONENT_INSTANTIATE),                    /* 19 */
    VSYM(cpmComponentTerminate, CPM_COMPONENT_TERMINATE),                      /* 20 */

    /*
     * CPM extended functions
     */
    
    VSYM(cpmComponentCleanup, CPM_COMPONENT_CLEANUP),                        /* 21 */
    VSYM(cpmComponentRestart, CPM_COMPONENT_RESTART),                        /* 22 */
    VSYM_EMPTY(NULL, CPM_COMPONENT_EXTN_HEALTHCHECK) /* Ext HealthCheck */ ,                /* 23 */
    VSYM(cpmComponentIdGet, CPM_COMPONENT_ID_GET),                          /* 24 */
    VSYM(cpmComponentLAUpdate, CPM_COMPONENT_LA_UPDATE),                       /* 25 */
    VSYM(cpmComponentPIDGet, CPM_COMPONENT_PID_GET),                         /* 26 */
    VSYM(cpmComponentStatusGet, CPM_COMPONENT_STATUS_GET),                      /* 27 */
    VSYM(cpmComponentListDebugAll, CPM_COMPONENT_DEBUG_LIST_GET),                   /* 28 */
    VSYM(cpmComponentAddressGet, CPM_COMPONENT_ADDRESS_GET),                     /* 29 */
    VSYM_EMPTY(NULL, CPM_COMPONENT_MOID_GET),                                       /* 30 */
    VSYM(cpmComponentInfoGet, CPM_COMPONENT_INFO_GET),                       /* 31 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(32)),                                       /* 32 */

    /*
     * SU functions
     */

    VSYM_EMPTY(NULL, CPM_FUNC_ID(33)),                                       /* 33 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(34)),                                       /* 34 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(35)),                                       /* 35 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(36)),                                       /* 36 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(37)),                                       /* 37 */
    VSYM_EMPTY(NULL, CPM_FUNC_ID(38)),                                       /* 38 */

    /*
     * Call which are mainly for AMS submodule interest 
     */
    VSYM(cpmComponentCSISet, CPM_COMPONENT_CSI_ASSIGN),                         /* 39 */
    VSYM(cpmComponentCSIRmv, CPM_COMPONENT_CSI_RMV),                         /* 40 */
    VSYM(cpmCBResponse, CPM_CB_RESPONSE),                              /* 41 */
    VSYM(cpmPGTrack, CPM_COMPONENT_PGTRACK),                                 /* 42 */
    VSYM(cpmPGTrackStop, CPM_COMPONENT_PGTRACK_STOP),                             /* 43 */
    VSYM(cpmComponentHAStateGet, CPM_COMPONENT_HA_STATE_GET),                     /* 44 */
    VSYM(cpmQuiescingComplete, CPM_COMPONENT_QUIESCING_COMPLETE),                       /* 45 */
    VSYM(cpmComponentFailureReport, CPM_COMPONENT_FAILURE_REPORT),                  /* 46 */
    VSYM(cpmComponentFailureClear, CPM_COMPONENT_FAILURE_CLEAR),                   /* 47 */
    VSYM(cpmIsCompRestarted, CPM_COMPONENT_IS_COMP_RESTARTED),                         /* 48 */
    VSYM(cpmAspCompLcmResHandle, CPM_COMPONENT_LCM_RES_HANDLE),                     /* 49 */
    VSYM(cpmResetNodeElseCommitSuicideRmd, CPM_RESET_ELSE_COMMIT_SUICIDE),           /* 50 */

    /*
     * CPM-CPM interaction 
     */
    
    VSYM(cpmCpmLocalRegister, CPM_CPML_REGISTER),                        /* 51 */
    VSYM(cpmCpmLocalDeregister, CPM_CPML_DEREGISTER),                      /* 52 */
    VSYM(cpmCpmConfirm, CPM_CPML_CONFIRM),                              /* 53 */
    
    /*
     * Call which are mainly for BM submodule interest 
     */
    
    VSYM(cpmBootLevelGet, CPM_BM_GET_CURRENT_LEVEL),                            /* 54 */
    VSYM(cpmBootLevelSet, CPM_BM_SET_LEVEL),                            /* 55 */
    VSYM(cpmBootLevelMax, CPM_BM_GET_MAX_LEVEL),                            /* 56 */
    VSYM(cpmNodeCpmLResponse, CPM_NODE_CPML_RESPONSE),                        /* 57 */
    
    /*
     * Bug 3610: Added entry for cpmProcNodeShutDownReq.
     */
    VSYM(cpmProcNodeShutDownReq, CPM_PROC_NODE_SHUTDOWN_REQ),                     /* 58 */
    VSYM(cpmNodeShutDown, CPM_NODE_SHUTDOWN),                            /* 59 */
    VSYM(cpmNodeArrivalDeparture, CPM_CM_NODE_ARRIVAL_DEPARTURE),                    /* 60 */
    VSYM(cpmSlotInfoGet, CPM_SLOT_INFO_GET),                             /* 61 */
    VSYM(cpmIocAddressForNodeGet, CPM_NODE_IOC_ADDR_GET),                    /* 62 */
    VSYM(cpmGoBackToRegisterCallback, CPM_NODE_GO_BACK_TO_REG),                /* 63 */
    /* 
     * Last entry should be empty.
     */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_DECL(cpmMgmtFuncList)[] = 
{
    VSYM(cpmNodeConfigSet, CPM_MGMT_NODE_CONFIG_SET),  /* 0 */
    VSYM(cpmNodeConfigGet, CPM_MGMT_NODE_CONFIG_GET),  /* 1 */
    VSYM(cpmNodeRestart, CPM_MGMT_NODE_RESTART),       /* 2 */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, AMF)[] = 
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, cpmNativeFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_CPM_MGMT_CLIENT_TABLE_ID, cpmMgmtFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
