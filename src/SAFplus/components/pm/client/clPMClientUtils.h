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
 * ModuleName  : mso 
 * File        : clPMClientUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contins PM configuration internal definitions 
 *          and function prototypes.
 *
 *****************************************************************************/

#ifndef _CL_PM_CLIENT_UTILS_H_
#define _CL_PM_CLIENT_UTILS_H_ 

#include <clOsalApi.h>
#include <clCorTxnApi.h>
#include <clTxnAgentApi.h>
#include <clClistApi.h>
#include <clPMApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clAlarmApi.h>
#include <clHash.h>

#ifdef __cplusplus
extern "C"
    {
#endif

#define CL_PM_ALMCLASSTABLE_BITS         (6)
#define CL_PM_ALMCLASSTABLE_BUCKETS  (1 << CL_PM_ALMCLASSTABLE_BITS)
#define CL_PM_ALMCLASSTABLE_MASK         (CL_PM_ALMCLASSTABLE_BUCKETS - 1)

#define CL_PM_ALMATTRTABLE_BITS         (9)
#define CL_PM_ALMATTRTABLE_BUCKETS  (1 << CL_PM_ALMATTRTABLE_BITS)
#define CL_PM_ALMATTRTABLE_MASK         (CL_PM_ALMATTRTABLE_BUCKETS - 1)

#define CL_PM_CONFIG_FILE "clPMConfig.xml"

typedef struct ClPMAlmTableClassDef
{
    struct hashStruct hash;
    ClCorClassTypeT classId;
    ClCorClassTypeT pmResetAttrId;
    struct hashStruct *pPMAlarmAttrTable[CL_PM_ALMATTRTABLE_BUCKETS];
}ClPMAlmTableClassDefT;

typedef struct ClPMAlmTableClassDefT* ClPMAlmTableClassDefPtrT;

typedef struct ClPMAlmAttrDef
{
    ClAlarmProbableCauseT probableCause;
    ClAlarmSpecificProblemT specificProblem;
    ClAlarmSeverityTypeT severity;
    ClInt64T lowerBound; 
    ClInt64T upperBound; 
}ClPMAlmAttrDefT;

typedef ClPMAlmAttrDefT* ClPMAlmAttrDefPtrT;

typedef struct ClPMAlmTableAttrDef
{
    struct hashStruct hash;
    ClCorAttrIdT attrId;
    ClUint32T alarmCount;
    ClPMAlmAttrDefT* pAlarmAttrDef;
} ClPMAlmTableAttrDefT;

typedef ClPMAlmTableAttrDefT* ClPMAlmTableAttrDefPtrT;

typedef enum
{
    CL_PM_READ = 0,
    CL_PM_RESET
} ClPMOpT;

extern ClOsalMutexT gClPMMutex;

ClRcT clPMLibInitialize();
ClRcT clPMLibFinalize();

ClRcT
clPMRead ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId);
ClRcT
clPMTxnPrepare ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId);
ClRcT
clPMTxnRollback ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId);
ClRcT
clPMTxnUpdate ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId);

void clPMMoIdListDeleteCB(ClClistDataT userData);
ClRcT clPMObjectDataFree(ClPMObjectDataPtrT pmObjData);
void _clPMMoIdListWalkCB(ClClistDataT userData, void* userArg);
ClRcT clPMMoIdListNodeDel(ClClistT moIdList, ClCorMOIdPtrT pMoId);
ClRcT clPMTxnJobUpdate(ClCorTxnIdT txnId, ClCorTxnJobIdT jobId, void* arg);
ClUint32T _clPMAttrSizeGet(ClCorTypeT attrDataType);
ClRcT _clPMTimerCallback(ClPtrT arg);
ClRcT clPMTxnJobWalk(ClCorTxnIdT txnId, ClCorTxnJobIdT jobId, void* arg);

/* PM Alarm Table related functions */
ClRcT clPMAlarmTableCreate();
void clPMAlarmInfoFinalize();
ClRcT clPMAlarmTableAttrInfoGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClPMAlmTableAttrDefT** ppAttrInfo);
ClInt64T pmAtoI(ClCharT* pTemp);
ClAlarmProbableCauseT pmAlarmProbCauseGet(ClCharT* probCause);
ClAlarmSeverityTypeT pmAlarmSeverityGet(ClCharT* severity);
ClRcT pmCorTypeGet(ClCharT* dataTypeName, ClCorTypeT* dataTypeId);
ClRcT _clPMAttrRangeCheck(void* pValue, ClCorAttrTypeT attrType, ClCorTypeT attrDataType, ClInt32T index, 
        ClUint32T size, ClInt64T minVal, ClInt64T max);
ClRcT clPMAttrAlarmProcess(ClCorMOIdPtrT pMoId, ClCorClassTypeT classId, ClPMAttrDataPtrT pmAttr);

ClRcT clPMAlarmClassInfoGet(ClCorClassTypeT classId, ClPMAlmTableClassDefT** ppClassInfo);
ClRcT clPMAlarmAttrInfoGet(struct hashStruct **pPMAlarmAttrTable, ClCorAttrIdT attrId, ClPMAlmTableAttrDefT** ppAttrInfo);
ClRcT clPMAlarmAttrInfoAdd(ClCorClassTypeT classId, ClPMAlmTableAttrDefT* pAttrInfo);
ClRcT clPMAlarmInfoGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClPMAlmTableAttrDefT** ppAttrInfo);
ClRcT clPMAlarmClassInfoAdd(ClCorClassTypeT classId, ClCorAttrIdT pmResetAttrId);

#ifdef __cplusplus
    }
#endif

#endif
