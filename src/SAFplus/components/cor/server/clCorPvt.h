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
 * File        : clCorPvt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Declarations for internal routines used across the modules. 
 *
 *
 *****************************************************************************/
#ifndef _COR_PVT_H
#define _COR_PVT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorServiceId.h>
#include <clOmCommonClassTypes.h>
#include <clEoApi.h>
#include <clParserApi.h>
#include <clTxnApi.h>

/*Internal Header*/
#include "clCorRmDefs.h"
#include "clCorDmDefs.h"
#include <clRmdIpi.h>


/* Macro definitions and Enums */

#define CL_COR_CLIENT_SESSION_EO_FUNC  CL_EO_GET_FULL_FN_NUM(CL_EO_COR_CLIENT_TABLE_ID, 1)

/* Following function calculates log to the base 2 of number passed */
/* If log2 is a fraction, then 1 is added to the log value */
#define LOG2(num, val)\
do\
{\
    val =0;\
    if(num==1)\
        val = 1;\
    else\
    {\
        int divider=2;\
        for(;num/divider>0;val++,divider*=2);\
        if(num%(divider/2)!=0)\
           val++;\
    }\
}while(0);

/**
 * Macro to process the delay data. 
 */
#define CL_COR_OP_DELAY_PROCESS(actualTime, time, pTime) \
do {\
        if(0 != actualTime) \
        { \
            time = actualTime; \
            goto sendData; \
        } \
        else \
        { \
            pTime = &actualTime; \
            goto getFromStandby;\
        } \
}while(0)


/* Forward Declarations */
/**
 * Enumeration to track the initialization phase.
 */ 
typedef enum ClCorInitStage{
    CL_COR_INIT_INCOMPLETE,
    CL_COR_INIT_DONE
}ClCorInitStageT;

/* forward declaration of CORMOClass */
struct MArrayNode;
typedef struct MArrayNode  CORMOClass_t;
typedef struct MArrayNode* CORMOClass_h;
                                                                                                                            
/* forward declaration of CORMSOClass */
struct CORMSOClass;
typedef struct CORMSOClass CORMSOClass_t;
typedef CORMSOClass_t*     CORMSOClass_h;


/* Enum for determining the type of job. */
typedef enum ClCorJobType  
{
    CL_COR_JOB_TYPE_MODIFY,
    CL_COR_JOB_TYPE_READ 
}ClCorJobTypeT;


/**
 * Enum for determining the state of the transaction.
 */
typedef enum ClCorObjTxnProcState{
    CL_COR_OBJ_TXN_STATE_INVALID,
    CL_COR_OBJ_TXN_STATE_SET_INPROGRESS,
    CL_COR_OBJ_TXN_STATE_DELETE_INPROGRESS,
}ClCorObjTxnProcStateT;

/* Structure for storing the session information */
struct ClCorBundleData {

    ClTxnTransactionHandleT       txnId;                  /* The handle obtained as part of the client's GET job. */
    ClCntHandleT                  jobCont;                /* Handle of the container to store all the jobs */
    ClRmdResponseContextHandleT   clientAddr;             /* Ioc Address of the Client for calling the EO callback function. */
    ClCorJobTypeT                 jobType;                /* Type of job -  Read or modify.   */
    ClBufferHandleT               outMsgHandle;           /* Buffer handle to be returned */
};

typedef struct ClCorBundleData ClCorBundleDataT;
typedef ClCorBundleDataT * ClCorBundleDataPtrT;
	
struct ClCorDbgGetBundle
{
    ClOsalMutexIdT          corGetMutexId;
    ClOsalCondIdT           corGetCondId;
    ClBufferHandleT         msgH;
    ClCntHandleT            cntHandle;
};

typedef struct ClCorDbgGetBundle ClCorDbgGetBundleT;
typedef ClCorDbgGetBundleT*    ClCorDbgGetBundlePtrT;

    
struct ClCorDbgBundleData
{
    ClCorTypeT      attrType;
    ClCorAttrTypeT  arrType;
    ClCorAttrIdT    attrId;
    ClPtrT          data;
    ClUint32T       size;
    ClInt32T        index;
    ClCorJobStatusT jobStatus;
    ClCorMOIdT      moId;
    ClCorAttrPathT  attrPath;
};

typedef struct ClCorDbgBundleData ClCorDbgBundleDataT;
typedef ClCorDbgBundleDataT * ClCorDbgBundleDataPtrT;

/**
 * Structure which will act as key for the session-job container.
 */

struct ClCorDbgBundleJobKey 
{
//    ClUint16T           ohSize;
//    ClCorObjectHandleT  oh;
    ClCorMOIdT          moId;
    ClCorAttrPathT      attrPath;
    ClCorAttrIdT        attrId;
};

typedef struct ClCorDbgBundleJobKey ClCorDbgBundleJobKeyT;
typedef ClCorDbgBundleJobKeyT * ClCorDbgBundleJobKeyPtrT;


/**
 * Structure to be used in the debug module.
 */
struct _ClCorDbgGetOp 
{
    ClBufferHandleT   *pMsgHandle;
    ClOsalMutexIdT    mutexId;
    ClOsalCondIdT     condId;
    ClCorJobStatusT   jobStatus;
};

typedef struct _ClCorDbgGetOp ClCorDbgGetOpT;

/* used to store the Mo Class and Om class id mapping */
typedef struct ClCorMsoOmType
{
    ClCorClassTypeT moClassId;
    ClCorServiceIdT svcId;
    ClOmClassTypeT omClassId;
}ClCorMsoOmTypeT;

typedef ClCorMsoOmTypeT* ClCorMsoOmTypePtrT;

/* used to store the om class name to class id mapping */
typedef struct ClCorOmClassData
{
    ClCharT* className;
    ClOmClassTypeT classId;
}ClCorOmClassDataT;

/* Structure which will be used in the object show cli command. */
struct _ClCorShowGetOpInfo{
    ClPtrT                  data;
    CORAttr_h               attrH; 
    ClOsalMutexIdT          corAttrShowMutex;
    ClOsalCondIdT           corAttrShowCond;
    ClCorJobStatusT         jobStatus;
    ClBufferHandleT         msgH;
};

typedef struct _ClCorShowGetOpInfo _ClCorShowGetOpInfoT;
typedef ClCorOmClassDataT* ClCorOmClassDataPtrT;


/* Structure to store the mutexes used in COR server */
struct _ClCorServerMutex
{
    ClOsalMutexIdT 		gCorServerMutex;
    ClOsalMutexIdT 		gCorSyncStateMutex;
    ClOsalMutexIdT		gCorDeltaObjDbMutex;
    ClOsalMutexIdT		gCorDeltaRmDbMutex;
    ClOsalMutexIdT      gCorTxnValidation;
};

typedef struct _ClCorServerMutex _ClCorServerMutexT;
typedef _ClCorServerMutexT * _ClCorServerMutexPtrT;


/**
 * Structure to store the time taken to perform certain operations.
 */ 
struct _ClCorOpDelay{
    ClTimeT dmDataUnpack;
    ClTimeT moClassTreeUnpack;
    ClTimeT objTreeUnpack;
    ClTimeT routeListUnpack;
    ClTimeT niTableUnpack;
    ClTimeT corStandbySyncup;
    ClTimeT dmDataPack;
    ClTimeT moClassTreePack;
    ClTimeT objTreePack;
    ClTimeT routeListPack;
    ClTimeT niTablePack;
    ClTimeT objTreeRestoreDb;
    ClTimeT routeListRestoreDb;
    ClTimeT objDbRecreate;
    ClTimeT routeDbRecreate; 
    ClTimeT corDbRecreate;
};

typedef struct _ClCorOpDelay _ClCorOpDelayT;
typedef _ClCorOpDelayT * _ClCorOpDelayPtrT;

typedef struct _corMsoConfigT
{
    ClUint32T count;
    ClOsalMutexT mutex;
    ClRmdResponseContextHandleT syncRespHandle;
    ClBufferHandleT outMsgHandle;
} corMsoConfigT;

typedef struct ClCorAttrInitInfo {
    ClCorAttrIdT    attrId;
    ClUint32T       size;
    ClUint32T       offset;
    ClPtrT          pValue;
    ClCorAttrTypeT  type;
    ClCorTypeT      dataType;
} ClCorAttrInitInfoT;

typedef struct ClCorAttrInitInfoList
{
    ClUint32T numAttrs;
    ClCorAttrInitInfoT* pAttrInitInfo;
} ClCorAttrInitInfoListT;

/* --- MO Path API's --- */
ClRcT _corMOClassCreate(ClCorMOClassPathPtrT moPath, ClInt32T maxInstances, CORMOClass_h *moClassHandle);
ClRcT _corMOClassDelete(ClCorMOClassPathPtrT moPath);
ClRcT _corMSOClassCreate(CORMOClass_h moClassHandle, ClCorServiceIdT svcId, ClCorClassTypeT class, CORMSOClass_h *msoClassHandle);
ClRcT clCorMSOClassExist(ClCorMOClassPathPtrT moPath, ClCorServiceIdT svcId); 
ClRcT _corMSOClassDelete(CORMOClass_h moClassHandle, ClCorServiceIdT svcId);
ClRcT clCorMOClassExist(ClCorMOClassPathPtrT moPath); 
ClRcT corMOClassHandleGet(ClCorMOClassPathPtrT corPath,  CORMOClass_h* moClassHandle);
ClRcT corMSOClassHandleGet(CORMOClass_h moClassHandle, ClCorServiceIdT svcId, CORMSOClass_h* msoClassHandle);
ClRcT _corMOPathToClassIdGet(ClCorMOClassPathPtrT pPath, ClCorServiceIdT svcId, ClCorClassTypeT* pClassId);
ClRcT _clCorMoClassPathNextSiblingGet(ClCorMOClassPathPtrT this);
ClRcT _clCorMoClassPathFirstChildGet(ClCorMOClassPathPtrT this);



 /* --- Object related APIs --- */
ClRcT _clCorSubTreeDelete(ClCorMOIdT);

ClRcT _clCorMoIdFirstInstanceGet(ClCorMOIdPtrT this);
ClRcT _clCorMoIdNextSiblingGet(ClCorMOIdPtrT this);


/* Object handle manipulation APIs*/
ClRcT _clCorObjectFlagsGet(ClCorMOIdPtrT moh, ClCorObjFlagsT* pFlags);
ClRcT _clCorObjectFlagsSet(ClCorMOIdPtrT moh, ClCorObjFlagsT pFlags);


ClRcT _clCorObjectHandleToTypeGet(ClCorObjectHandleT objHandle, ClCorObjTypesT* type);
ClRcT _clCorObjectHandleToMoIdGet(ClCorObjectHandleT objHandle, ClCorMOIdPtrT moId, ClCorServiceIdT *srvcId);
ClRcT _clCorObjectHandleGet(ClCorMOIdPtrT this, ClCorObjectHandleT *objHandle);
    
ClRcT _clCorObjectWalk(ClCorMOIdPtrT cAddr, ClCorMOIdPtrT cAddrWC, 
        void (*fp) (void *, ClBufferHandleT), ClCorObjWalkFlagsT flags, ClBufferHandleT bufferHandle);
ClRcT _corObjectCountGet(ClUint32T *);


ClRcT _corClassAttrValueSet(CORClass_h classHandle, ClCorAttrIdT attrId, 
                                   ClInt64T  init, ClInt64T  min, ClInt64T  max);

ClRcT _corClassAttrUserFlagsSet(CORClass_h classHandle, ClCorAttrIdT attrId, ClUint32T flags);

ClRcT clCorOHMaskCalculate(ClUint8T* ohMask);


/* Persistency Operations.*/
ClRcT  _clCorDataSave();
ClRcT  _clCorDataRestore();
ClRcT  _clCorDataFrequentSave(char  *fileName, ClTimerTimeOutT  frequency);

/*** MoId/Logical Address mapping ***/
ClRcT _clCorMoIdToLogicalSlotGet(ClCorMOIdPtrT pMoId, ClUint32T* logicalSlot);

/*** cor station disable call to its peer */
ClRcT clCorStationDisable();

/*** get the slave Id from the cor list */
ClRcT clCorSlaveIdGet(ClIocNodeAddressT *nodeAddr);

/* Internal methods to parse the cor meta data from the xml file */
ClRcT clCorInformationModelBuild(const ClCharT *pConfigFile);
ClRcT clCorMsoSvcIdGet(ClCharT* msoType, ClCorServiceIdT* svcId);
ClRcT clCorParseContAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassContAttrDef, ClListHeadT *pAttrList);
ClRcT clCorParseAssocAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassAssocAttrDef, ClListHeadT *pAttrList);
ClRcT clCorParseArrayAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassArrAttrDef, ClListHeadT *pAttrList);
ClRcT clCorParseSimpleAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassSimpleAttrDef, ClListHeadT *pAttrList);
ClRcT clCorAttrDataTypeIdGet(ClCharT* dataTypeName, ClInt32T* dataTypeId);
ClRcT clCorMoClassPathXlate(ClCharT* moClassPathName, ClCorMOClassPathPtrT moClassPath);
ClRcT clCorClassAttrFormUserFlag(ClInt32T *corClassAttrUserFlag, ClCharT* corClassAttrType, ClCharT* corClassAttrPersistency, ClCharT* corClassAttrCaching, ClCharT* corClassAttrInitialized, ClCharT* corClassAttrWritable);
ClRcT clCorOmClassIdFromNameGet(ClCorOmClassDataT* pOmClassData, ClUint32T numOmClasses, ClCharT* omClassName, ClOmClassTypeT* omClassId);
ClRcT clCorOmClassNameFromIdGet(ClOmClassTypeT classId, ClCharT *pClassName, ClInt32T maxClassSize);
/* methods to parse the cor configuration data from xml file */
ClRcT clCorConfigGet(ClUint32T* pCorSaveType);
ClRcT clCorOHMaskValueGet(ClCharT* corOHMaskString, ClUint8T** corOHMaskValue);
ClRcT clCorSaveTypeValueGet(ClCharT* corSaveType, ClUint32T* pCorSaveType);

/* method to validate the OH Mask value against object creation */
ClRcT _clCorMoIdValidateAgainstOHMask(ClCorMOIdPtrT pMoId);

/* Function to read the OM class Information from the XML file.*/
ClRcT clCorOmInfoBuild();

/* Function to get the class id from the moId passed */
ClRcT _corMoIdToClassGet(ClCorMOIdPtrT pMoId, ClCorClassTypeT* pClassId);

/* Function to get the MoId string from MoId value. */
ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh,  ClNameT *moIdName);

/* Wrapper IPI to print the MoId in string for debugging purposes */
ClCharT* _clCorMoIdStrGet(ClCorMOIdPtrT moIdh, ClCharT* tmpStr);

/* Function to get the attribute information */
CORAttr_h dmClassAttrInfoGet(CORClass_h classH, ClCorAttrPathT* pAttrPath, ClCorAttrIdT attrId);

/* 
   Function to create the container to store the runtime 
   attribute information during attr walk 
 */  
ClRcT _clCorAttrWalkRTContCreate();

/* Function to delete the container to store the runtime 
   attribute information during attr walk*/  
ClRcT _clCorAttrWalkRTContDelete();

/* Function to create the MO and all the MSOs */
ClRcT _clCorUtilMoAndMSOCreate(ClCorMOIdPtrT pMoId);

ClRcT clCorBuiltinModuleRun(void);

void clCorClassConfigInitialize(void);

void clCorClassConfigOmClassIdUpdate(ClOmClassTypeT classId);

void clCorClassConfigCorClassIdUpdate(ClCorClassTypeT classId);

ClRcT clCorInformationModelLoad(const ClCharT *pConfigFile, const ClCharT *pRouteFile);

ClRcT clCorCreateResources(const ClCharT *pDirName, const ClCharT *pSuffixName, ClInt32T suffixLen);
ClRcT clCorSyncDataWithSlave(ClInt32T funcId, ClBufferHandleT inMsgHandle);

#ifdef __cplusplus
}
#endif
#endif  /*  _INC_COR_NOTIFY_H_ */
