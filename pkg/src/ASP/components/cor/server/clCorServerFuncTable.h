#ifndef _CL_COR_SERVER_FUNC_TABLE_H_
#define _CL_COR_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(corEOFuncList)[] =
{
    /* Fixed functions */
    VSYM_EMPTY(NULL, COR_FUNC_ID(0)),                      /*  0 */

    VSYM(_corSyncRequestProcess, COR_EO_SYNCH_RQST_OP),     /*  1 */
    VSYM(_clCorMsoConfigOp, COR_EO_MSO_CONFIG),                      /*  2 */
    VSYM_EMPTY(NULL, COR_FUNC_ID(3)),                      /*  3 */
    VSYM_EMPTY(NULL, COR_FUNC_ID(4)),                      /*  4 */
    VSYM_EMPTY(NULL, COR_EO_RL_EVT),                      /*  5 */
    VSYM_EMPTY(NULL, COR_EO_CORSYNC_STATUS),                      /*  6 */
    VSYM_EMPTY(NULL, COR_EO_OOB_DATA_HDLER),                      /*  7 */
    VSYM_EMPTY(NULL, COR_EO_DM_ATTR_HDLER),                      /*  8 */
    VSYM_EMPTY(NULL, COR_EO_UPD_MOID_STRT),                      /*  9 */
    VSYM_EMPTY(NULL, COR_EO_WHERETOHANG_GET),                      /*  10 */
    VSYM(_corNIOp, COR_EO_NI_OP),                  /*  11 */

    /* class related functions */
    VSYM(_corClassOp, COR_EO_CLASS_OP),               /*  12 */
    VSYM(_corAttrOp, COR_EO_CLASS_ATTR_OP),                /*  13 */
    VSYM(_corMOTreeClassOpRmd, COR_EO_MOTREE_OP),      /*  14 */
    VSYM_EMPTY(NULL, COR_EO_ROUTE_GET),                     /*  15 */
    VSYM(_corPersisOp, COR_EO_PERSISTENCY_OP),              /*  16 */

    /* object related functions */
    VSYM(_corObjectOp, COR_EO_OBJECT_OP),              /*  17 */
    VSYM(_corObjectHandleConvertOp, COR_EO_OBJECTHDL_CONV), /*  18 */
    VSYM(_corObjectWalkOp, COR_EO_OBJECT_WALK),          /*  19 */

    /* route related functions */
    VSYM(_corRouteApi, COR_EO_ROUTE_OP),              /*  20 */
    VSYM(_corObjectFlagsOp, COR_EO_OBJECT_FLAG_OP),         /*  21 */


    VSYM(_clCorBundleOp, COR_EO_BUNDLE_OP),           /*  22 */
    VSYM(_corStationDisable, COR_EO_STATION_DIABLE),        /*  23 */
    VSYM(clCorTransactionConvertOp, COR_TRANSACTION_OPS), /*  24 */
    VSYM(_CORNotifyGetAttrBits, COR_NOTIFY_GET_RBE),	 /*  25 */
    VSYM(_CORUtilOps, COR_UTIL_OP), /*  26 */
    VSYM(_corMOIdOp, COR_MOID_OP),                /*  27 */
    VSYM(_clCorDelaysRequestProcess, CL_COR_PROCESSING_DELAY_OP), /*  28 */
    VSYM(_clCorClientDbgCliExport, COR_CLIENT_DBG_CLI_OP),  /*  29 */
    VSYM(_corObjExtObjectPack, COR_OBJ_EXT_OBJ_PACK),      /*  30 */
    VSYM(_corMoIdToNodeNameTableOp, COR_MOID_TO_NODE_NAME_TABLE_OP),   /*  31 */

#ifdef GETNEXT
    VSYM(_corObjectGetNextOp, COR_OBJ_GET_NEXT_OPS),       /*  25 */
#endif
    VSYM(_CORUtilExtendedOps, COR_UTIL_EXTENDED_OP), /*  26 */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, COR) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, corEOFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}   
#endif

#endif
