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
 * File        : clCorClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains inteface definitions between cor server and client.
 *
 *
 *****************************************************************************/

#ifndef _COR_CLIENT_H
#define _COR_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clEoApi.h>
#include <clCorMetaData.h>
#include <clCorNotifyApi.h>
#include <clDebugApi.h>
#include <clVersionApi.h>
#include <clLogApi.h>
#include <clCorLog.h>
#include <clList.h>

/* Internal includes  */
/* #include "clCorIntMetaData.h" */
/*  #include "clCorXdr.h" */

#define COR_MAX_FILENAME_LEN   30

/**
 *  Bundle timeout for the synchronous version of the bundle apply.
 */

#define CL_COR_BUNDLE_TIME_OUT     { 0, COR_RMD_DFLT_RETRIES * COR_RMD_DFLT_TIME_OUT }


                                                                                                                            
#define COR_EO_SYNCH_RQST_OP        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1)
#define COR_EO_MSO_CONFIG           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2)
#define COR_EO_RL_EVT               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,5)
#define COR_EO_CORSYNC_STATUS       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,6)
#define COR_EO_OOB_DATA_HDLER       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,7)
#define COR_EO_DM_ATTR_HDLER        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,8)
#define COR_EO_UPD_MOID_STRT        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,9)
#define COR_EO_WHERETOHANG_GET      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,10)

#define COR_EO_NI_OP                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,11)

/* COR Class related  */
#define COR_EO_CLASS_OP             CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,12)
#define COR_EO_CLASS_ATTR_OP        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,13)
#define COR_EO_MOTREE_OP            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,14)
#define COR_EO_ROUTE_GET            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,15)
#define COR_EO_PERSISTENCY_OP       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,16)

/* Object related  */
#define COR_EO_OBJECT_OP            CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,17)
#define COR_EO_OBJECTHDL_CONV       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,18)
#define COR_EO_OBJECT_WALK          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,19)

#define COR_EO_ROUTE_OP          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,20)
#define COR_EO_OBJECT_FLAG_OP    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,21)
#define COR_EO_BUNDLE_OP         CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,22)
#define COR_EO_STATION_DIABLE    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,23)
#define COR_TRANSACTION_OPS      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,24)
#define COR_NOTIFY_GET_RBE       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,25)
#define COR_UTIL_OP              CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,26)
#define COR_MOID_OP              CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,27)
#define CL_COR_PROCESSING_DELAY_OP CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,28)
#define COR_CLIENT_DBG_CLI_OP    CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,29)
#define COR_OBJ_EXT_OBJ_PACK     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,30)
#define COR_MOID_TO_NODE_NAME_TABLE_OP CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,31)

#ifdef GETNEXT
#define COR_OBJ_GET_NEXT_OPS        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,25)
#endif

#define COR_UTIL_EXTENDED_OP        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,32)

#define COR_FUNC_ID(n)          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, n)

typedef enum 
{
    COR_CLASS_OP_CREATE = 1,
    COR_CLASS_OP_DELETE,
    COR_CLASS_OP_EXISTANCE,
    COR_CLASS_OP_CLASSID_GET,
    COR_CLASS_OP_FIRSTCHILD_GET,
    COR_CLASS_OP_NEXTSIBLING_GET,
    COR_CLASS_OP_MSOCLASSTYPE_GET
} corClassOp_e ;

typedef enum 
{
    COR_TXN_OP_COMMIT   =   1,
} corTxnOp_e ;


typedef enum
{
   MO_CLASS= 1,
   MSO_CLASS,
}corClassType_e;

typedef enum 
{
    COR_ATTR_OP_CREATE = 1,
    COR_ATTR_OP_DELETE,
    COR_ATTR_OP_VALS_SET,
    COR_ATTR_OP_VALS_GET,
    COR_ATTR_OP_FLAGS_SET,
    COR_ATTR_OP_FLAGS_GET,
    COR_ATTR_OP_TYPE_GET,
    COR_ATTR_OP_ID_GET,
    COR_ATTR_OP_ID_GET_NEXT,
    COR_ATTR_OP_ID_LIST_GET
} corAttrOp_e ;

typedef enum 
{
    COR_NI_OP_CLASS_NAME_SET = 1,
    COR_NI_OP_CLASS_NAME_GET,
    COR_NI_OP_CLASS_ID_GET,
    COR_NI_OP_CLASS_NAME_DELETE,
    COR_NI_OP_ATTR_NAME_SET,
    COR_NI_OP_ATTR_NAME_GET,
    COR_NI_OP_ATTR_ID_GET,
    COR_NI_OP_ATTR_NAME_DELETE,
} corNiOp_e ;

typedef enum 
{
    COR_NOTIFY_OP_GET_RBE = 1,
} corNotifyOp_e ;


typedef enum
{
    CL_COR_MOID_TO_CLASS_GET = 1,
    CL_COR_NAME_TO_MOID_GET,
    CL_COR_MOID_TO_NAME_GET
}ClCorMoIdOpT;

typedef enum
{
    COR_MOID_TO_NODE_NAME_GET,
    COR_NODE_NAME_TO_MOID_GET
}corMoIdToNodeNameTableOp_e;

/*
 * These enums are as used as directives in
 * packed Object for client.
 */
typedef enum{
 CL_COR_OBJSTREAM_CONT_INVALID_OP = 0,
 CL_COR_OBJSTREAM_CONT_TRUNCATE,
 CL_COR_OBJSTREAM_CONT_APPEND,
 CL_COR_OBJSTREAM_CONT_SET
/* CL_COR_OBJSTREAM_CONT_FORCED_TO_32BITS = CL_FORCED_TO_32BITS */
}ClCorObjStreamContOpT;

typedef enum ClCorUtilExtendedOp
{
    COR_MO_TO_OM_CLASS_UTIL_OP,
    COR_CONFIG_LOAD_UTIL_OP,
    COR_UTIL_MAX_OP,
} ClCorUtilExtendedOpT;

/* Enum definition for primary OI*/
typedef enum ClCorPrimaryOIStatus
{
    CL_COR_PRIMARY_OI_DISABLED = 0,
    CL_COR_PRIMARY_OI_ENABLED
} ClCorPrimaryOIStatusT;

#if 0
struct ClCorObjectAttrInfo {
          ClCorAttrIdT    attrId;
          ClCorAttrTypeT  attrType;
          union{
                 ClUint32T              len;
                 ClCorObjStreamContOpT  op;  /* op for containment path.*/
               }u;
};
#endif

/*
 * The type definition depicting the synchronization stautes of COR.
 */
typedef enum {
   CL_COR_SYNC_STATE_INVALID,
   CL_COR_SYNC_STATE_INPROGRESS,
   CL_COR_SYNC_STATE_COMPLETED
}ClCorSyncStateT;

struct ClCorObjectAttrInfo {
    ClCorAttrIdT    attrId;
    ClCorAttrTypeT  attrType;
    ClUint32T       lenOrContOp;
    ClUint32T       attrFlag;
};

typedef struct ClCorObjectAttrInfo ClCorObjectAttrInfoT;

typedef struct corClientMoIdToNodeName
{
    ClVersionT         version;
    ClCorMOIdT 		moId;
    SaNameT       nodeName;
    corMoIdToNodeNameTableOp_e op;
}corClientMoIdToNodeNameT;

typedef struct corClsIntf
{
    ClVersionT         version;
    corClassOp_e       op;
    ClCorClassTypeT     classId;
    ClCorClassTypeT     superClassId;
}corClsIntf_t;


typedef struct corClassDetails
{
    ClVersionT     version;
    corClassOp_e   operation;
    corClassType_e classType;
    ClCorMOClassPathT       corPath;
    ClInt32T      maxInstances;
    ClCorServiceIdT    svcId;
    ClCorClassTypeT objClass;
}corClassDetails_t;
    
typedef struct corAttrIntf
{
    ClVersionT        version;
    corAttrOp_e       op;
    ClCorClassTypeT    classId;
    ClCorAttrIdT       attrId;
    ClCorTypeT         attrType;
    ClCorTypeT         subAttrType;
    ClCorClassTypeT    subClassId;
    ClInt64T         min;
    ClInt64T         max;
    ClInt64T         init;
    ClUint32T        flags;
    ClListHeadT      list;
}corAttrIntf_t;


/* Structure to be used to RMD communication for Attribute names */
typedef struct corNiOpInf
{
        ClVersionT        version;
        corNiOp_e        op;
        ClCorClassTypeT  classId;               /* Class Id*/
        ClCorAttrIdT     attrId;                /* Attribute Id*/
        char            name[CL_COR_MAX_NAME_SZ]; /* Class Name */
}corNiOpInf_t;

typedef struct corRouteInfo
{
    ClVersionT    version;
    ClCorCommInfoT stations[COR_TXN_MAX_STATIONS];
    ClUint32T    stnCnt;
    ClCorMOIdT        moId;
}corRouteInfo_t;

typedef struct corTxnInfo
{
    ClVersionT     version;
    corTxnOp_e     operation;
    ClUint32T           jobCount;
}corTxnInfo_t;


typedef enum
{
   COR_DATA_SAVE = 1,
   COR_DATA_RESTORE,
   COR_DATA_FREQ_SAVE,
   COR_DATA_FREQ_SAVE_STOP,
}corPersisOp_e;

typedef struct corPersisInfo
{
    ClVersionT    version;
    char          fileName[COR_MAX_FILENAME_LEN];
    corPersisOp_e operation;
    ClTimerTimeOutT	  frequency;
}corPersisInfo_t;


typedef enum
{
   COR_OBJ_OP_CREATE = 1,
   COR_OBJ_OP_DELETE,
   COR_OBJ_OP_ATTR_SET,
   COR_OBJ_OP_OBJHDL_TO_MOID_GET,
   COR_OBJ_OP_OBJHDL_TO_TYPE_GET,
   COR_OBJ_OP_OBJHDL_GET,
   COR_OBJ_OP_FIRSTINST_GET,
   COR_OBJ_OP_NEXTSIBLING_GET,
   COR_OBJ_OP_ATTR_TYPE_SIZE_GET,
#ifdef GETNEXT
    COR_OBJ_OP_NEXTMO_GET, 
#endif
   COR_OBJ_OP_MOID_TO_OBJHDL_GET
}corObjectOp_e;


/**
 * Typedef to store the bundle status.
 */

typedef enum
{
/**
 * Bundle has been initialized.
 */
    CL_COR_BUNDLE_INITIALIZED,
/**
 * COR client didnot receive the response for this bundle.
 */
    CL_COR_BUNDLE_IN_EXECUTION,
/**
 * Cor client's synchronous bundle apply had timed out.
 */
    CL_COR_BUNDLE_TIMED_OUT
}ClCorBundleStatusT;



typedef struct corObjectInfo
{  
    ClVersionT           version;
    corObjectOp_e       operation;
    ClCorMOIdT          moId;
    ClUint16T           objHSize;
    ClCorObjectHandleT  pObjHandle;
    ClCorAttrPathT      attrPath;
    ClCorAttrIdT        attrId;
    ClInt32T            index;
    ClUint32T           size;
}corObjectInfo_t;


typedef struct corObjHdlConversion
{
    ClVersionT          version;
    corObjectOp_e       operation;
    ClUint16T           ohSize;
    ClCorObjectHandleT  pObjHandle;
    ClCorMOIdT          moId;
    ClCorObjTypesT      type;
    ClCorServiceIdT     svcId;
}corObjHdlConvert_t;


typedef enum
{
    COR_OBJ_WALK_OBJCNT_GET,
    COR_OBJ_WALK_DATA_GET,
    COR_OBJ_FLAG_GET,
    COR_OBJ_FLAG_SET,
    COR_OBJ_SUBTREE_DELETE,
}corObjWalkOp_e;


typedef struct corObjFlagNWalkInfo
{
    ClVersionT         version;
    corObjWalkOp_e     operation;
    ClCorMOIdT             moId;
    ClCorMOIdT	       moIdWithWC;
    ClCorObjWalkFlagsT  flags;    
}corObjFlagNWalkInfoT;

/*  typedef corObjWalkInfo_t corObjFlagInfo_t; */

/* Typedef for the Route */
typedef enum
{
   COR_SVC_RULE_STATION_ENABLE = 1,
   COR_SVC_RULE_STATION_ADDR_GET,
   COR_SVC_RULE_STATION_STATUS_GET,
   COR_SVC_RULE_STATION_DISABLE,
   COR_SVC_RULE_STATION_DELETE,
   COR_SVC_RULE_STATION_DELETE_ALL,
   COR_PRIMARY_OI_SET,
   COR_PRIMARY_OI_UNSET,
   COR_PRIMARY_OI_GET
}corRouteOp_e;

typedef enum ClCorStationStatus
{
    CL_COR_STATION_INVALID = -1,
    CL_COR_STATION_ENABLE = 1,
    CL_COR_STATION_DISABLE,
    CL_COR_STATION_DELETE
}ClCorStationStatus_e;

typedef struct corRouteApiInfo
{
	ClVersionT        version;
    corRouteOp_e      reqType;
    ClCorMOIdT        moId;
    ClCorAddrT        addr;
    ClUint8T          status;
    ClUint8T          primaryOI;
    ClCorServiceIdT   srvcId;
}corRouteApiInfo_t;


typedef enum
{
   COR_EVENT_SUBSCRIBE= 1,
   COR_EVENT_UNSUBSCRIBE,
}corEventOp_e;

typedef struct corEventInfo
{
    ClVersionT        version;
    corEventOp_e      operation;
    ClEoIdT           eoId;
    ClCorMOIdT            moId;
    ClCorServiceIdT       svcId;
    ClUint32T        rmdId;
    ClCorOpsT          ops;
    ClUint32T        cookie;
    ClCorNotifyHandleT pSubHandle;
    ClCorAttrListT     attrList;
}corEventInfo_t;


#ifdef GETNEXT
typedef struct corObjGetNextInfo
{
    ClVersionT     version;
    corObjectOp_e  operation;
    ClCorObjectHandleT objHdl;
    ClCorClassTypeT classId;
    ClCorServiceIdT    svcId;
}corObjGetNextInfo_t;
#endif



/**
 * Structure to store bundle information. 
 */

struct ClCorBundleInfo {
    ClCntHandleT                    jobStoreCont;  /* Container for storing the jobs */
    ClCorBundleCallbackPtrT         funcPtr;       /* Function pointer to be called after operation is done */
    ClPtrT                          userArg;       /* User Argument. */
    ClCorBundleStatusT              bundleStatus;   /*  Current status of the bundle*/
    ClCorBundleOperationTypeT       bundleType;    /* BundleType - transactional or non-transactional */
};
typedef struct ClCorBundleInfo ClCorBundleInfoT;
typedef ClCorBundleInfoT* ClCorBundleInfoPtrT;


/*
 * Internal Version related definitions
 */
extern ClVersionDatabaseT gCorClientToServerVersionDb;

#define clCorClientToServerVersionValidate(version, rc) \
		do\
		{ \
			if((rc = clVersionVerify(&gCorClientToServerVersionDb, (&version)) ) != CL_OK) \
			{ \
			    clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to validate the client version."); \
			} \
		}while(0) 


#define CL_COR_VERSION				{'B', 0x1, 0x1}
#define CL_COR_VERSION_SET(version)     \
		do { 							\
			version.releaseCode = 'B' ; \
			version.majorVersion = 0x1; \
			version.minorVersion = 0x1; \
		}while(0)

/* MACRO to help print errors    */
/* Prints error and returns RC */
#define CL_COR_RETURN_ERROR(ERROR,STRING, RC)\
    do \
        {\
        if(ERROR <= CL_DEBUG_ERROR)\
            {\
            clOsalPrintf("\nFile : %s, Line : %d, Error : %s, RC = 0x%x\n", __FILE__,__LINE__, STRING, RC);\
            }\
      CL_FUNC_EXIT();\
        return RC;\
        }\
        while(0)
        
/* Macro to log the function entry */
#define CL_COR_FUNC_ENTER(CL_COR_AREA, CL_COR_CONTEXT) \
do {\
    clLog(CL_LOG_SEV_TRACE, CL_COR_AREA, CL_COR_CONTEXT, "Entering [%s]", __FUNCTION__); \
}while(0)

/* Macro to log the function exit */
#define CL_COR_FUNC_EXIT(CL_COR_AREA, CL_COR_CONTEXT) \
do {\
    clLog(CL_LOG_SEV_TRACE, CL_COR_AREA, CL_COR_CONTEXT, "Leaving [%s]", __FUNCTION__); \
}while(0)




/*  FUNCTIONS */
extern ClRcT
clTxnCommitRmd(ClUint32T cData, ClBufferHandleT  inMsgHandle,ClBufferHandleT  outMsgHandle);

/**
 ************************************
 *
 *  Get MOId in a message.
 *
 *  
 *
 *
 *  \param pMoId:  Handle of the COR attribute path.
 *  \param pMsg: 
 *
 *  \par Return values:
 *  None
 *
 */

extern void clCorMoIdGetInMsg(ClCorMOIdPtrT pMoId, ClBufferHandleT* pMsg );

/**
 *  Function to swap the attribute value.
 */
extern ClRcT clCorObjAttrValSwap(CL_INOUT  ClUint8T        *pData,
                                 CL_IN      ClUint32T       dataSize,
                                 CL_IN      ClCorAttrTypeT  attrType,
                                 CL_IN      ClCorTypeT      arrayDataType);  /*data qualifier,
                                                                if attrType is of type CL_COR_ARRAY_ATTR */

extern ClBoolT clCorHandleRetryErrors(ClRcT retCode);

#ifdef __cplusplus
}
#endif
#endif 
