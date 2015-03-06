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
 * ModuleName  : static
 * File        : clSnmpOp.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _SNMP_OP_H_
#define _SNMP_OP_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <clCntApi.h>
#include <clCorMetaData.h>
#include <clMedApi.h>
#include <clSnmpDataDefinition.h>
#include <clSnmpDefs.h>
#include <clAppSubAgentConfig.h>

#include <clMedIpi.h>

/* Structure containing the get request specific information.*/
typedef struct _ClSnmpGetReqInfo{
    netsnmp_request_info    *pRequest;
    ClUint32T               columnType;
}_ClSnmpGetReqInfoT ;


typedef struct ClSnpOper
{
    ClOsalMutexIdT  mtx;
    ClUint32T       refCount;
    ClUint8T        cmtd;
    ClMedOpT        opInfo;
    ClMedErrorListT medErrList;
    ClPtrT          pOpAddData;
}ClSnmpOperT;
#define clSnmpMutexLock( mutex ) {                                    \
    ClRcT _rc = CL_OK;                                               \
    do{                                                              \
        _rc = clOsalMutexLock(mutex);                                \
            if(_rc != CL_OK ){                                       \
                clLogError("MTX", "LCK",                               \
                        "Locking mutex failed [0x%x]",              \
                         _rc);                                      \
                CL_ASSERT(_rc == CL_OK );                            \
            }                                                        \
    }while(0);                                                       \
}

#define clSnmpMutexUnlock( mutex ) {                                  \
    ClRcT _rc = CL_OK;                                               \
    do{                                                              \
        _rc = clOsalMutexUnlock(mutex);                              \
            if(_rc != CL_OK ){                                       \
                clLogError("MTX", "UNL",                               \
                        "Unlocking mutex failed [0x%x]",			 \
                         _rc);                                      \
                CL_ASSERT(_rc == CL_OK );                            \
            }                                                        \
    }while(0);                                                       \
}
extern ClMedHdlPtrT gMedSnmpHdl;

extern ClRcT initMM(void);

extern ClRcT 
clSnmpSendSyncRequestToServer (	void **my_loop_context, 
								void **my_data_context, 
								netsnmp_variable_list *put_index_data, 
								ClUint32T tableType,
                                ClSnmpOpCodeT opCode);

extern ClRcT clExecuteSnmp (ClSNMPRequestInfoT* reqInfo,
                            void* data,
                            ClInt32T *pReqInfo,
                            ClInt32T *pErrCode);
					 
extern ClRcT clRequestAdd (ClSNMPRequestInfoT* reqInfo,
                            void* data,
                            ClInt32T *pReqInfo,
                            ClInt32T *pErrCode, 
                            ClPtrT pReqAddInfo);
extern ClUint32T clGetTable (ClSNMPRequestInfoT* reqInfo,
							 void *data, 
							 ClInt32T *pErrCode);
					
extern ClUint32T clGetTableAttr (ClSNMPRequestInfoT* reqInfo,
						void* data,
						ClInt32T *pErrCode);
						
extern ClUint32T clSnmpJobTableAttrAdd (ClSNMPRequestInfoT* reqInfo,
						void* data, 
						ClInt32T *pOpNum,
						ClInt32T *pErrCode,
                        ClPtrT  pOpAddInfo);
/* Commit function to be added */

extern ClRcT snmpNotifyHandler (struct  ClMedAgentId *, /**< Varbind of Client */
                          ClUint32T ,         /**< SessionId  */ 
                          ClCorMOIdPtrT,              /**< Handle to ClCorMOId : 
                                                    Only for the time being */
                          ClUint32T ,         /**< AttrId  */ 
                          ClCharT *,              /**< Optional data */
                          ClUint32T);         /**< Data length*/ 

extern ClRcT clSnmpAlarmIdToOidGet(ClCorMOIdPtrT pMoId, ClUint32T probableCause, 
                    ClAlarmSpecificProblemT specificProblem, ClCharT* pTrapOid, ClCharT* pTrapDes);
extern ClInt32T clSnmpOidToAlarmIdGet(ClCharT* pTrapOid, ClUint32T *pProbableCause, 
                    ClAlarmSpecificProblemT* pSpecificProblem, ClCorMOIdPtrT pMoId);
extern void clSnmpOidCpy(ClSNMPRequestInfoT * pReqInfo, oid* pOid);

extern ClRcT clSnmpTableIndexCorAttrIdInit(ClUint32T numOfIndices, ClUint8T * pIndexOidList[],
                                           ClSnmpTableIndexCorAttrIdInfoT * pTableIndexCorAttrIdList);
extern ClRcT clSnmpCommit(
        CL_OUT ClMedErrorListT *pMedErrList);

extern void clSnmpUndo(void);
/* Newly added */
extern ClRcT clSnmpMutexInit(void);
extern ClRcT clSnmpMutexFini(void);

extern int
clSnmpModeEndCallback(
        netsnmp_mib_handler               *handler,
        netsnmp_handler_registration      *reginfo,
        netsnmp_agent_request_info        *reqinfo,
        netsnmp_request_info              *requests);


#ifdef __cplusplus
}
#endif
#endif
