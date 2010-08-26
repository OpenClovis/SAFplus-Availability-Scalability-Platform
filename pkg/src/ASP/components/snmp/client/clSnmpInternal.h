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
 * Build: 4.0.0
 */
/*******************************************************************************
 * ModuleName  : snmp                                                          
 * File        : clSnmpInternal.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * FIXME: TO DO
 *
 *
 *****************************************************************************/
#ifndef _CLOVIS_SNMP_H_
#define _CLOVIS_SNMP_H_

#ifdef __cplusplus
extern "C" {
#endif


#define CL_SNMP_MAX_STR_LEN 255
#define CL_SNMP_MAX_OID_LEN         128 
#define CL_SNMP_MAX_ATTR_FOR_INDEX  8
#define CL_SNMP_MAX_MOID_STR_LEN  1024
#define CL_SNMP_DISPLAY_STRING_SIZE 255

typedef enum
{
    CL_SCALAR_OBJ = 1,
    CL_IM_TABLE,
    CL_COMPONENT_TABLE,
    CL_ALARM_TABLE,
    CL_SNMP_TABLE_MAX
}clSnmpTables;
    
    
/*****************************************************************************/
/* Enums for SNMP GET/SET operations :                                       */
/*****************************************************************************/
typedef enum
{
/*imTable enums */
   CL_SNMP_GET_FIRST_IM_TBL,
   CL_SNMP_GET_NEXT_IM_TBL,
   CL_SNMP_GET_MO_AVAILABLE_SVCS,
   CL_SNMP_SET_MO_ROW_STATUS,
   
/*componentTable enums */
   CL_SNMP_GET_FIRST_COMPONENT_TBL,
   CL_SNMP_GET_NEXT_COMPONENT_TBL,

   CL_SNMP_GET_COMPONENT_NAME,
   CL_SNMP_GET_COMPONENT_OPERATIONAL_STATUS,
   CL_SNMP_GET_COMPONENT_PRESENCE_STATUS,

   CL_SNMP_SET_COMPONENT_NAME,
   CL_SNMP_SET_COMPONENT_LOG_LEVEL,
/* alarmTable enums */
    CL_SNMP_GET_FIRST_ALARM_TBL,        
    CL_SNMP_GET_NEXT_ALARM_TBL,
    
    CL_SNMP_GET_ALARM_ID,
    CL_SNMP_GET_ALARM_SEVERITY,
    CL_SNMP_GET_ALARM_OP_STATE,
    CL_SNMP_GET_ALARM_ENABLE,           
    CL_SNMP_GET_ALARM_SUSPEND,
    CL_SNMP_GET_ALARM_CLEAR,
    CL_SNMP_GET_ALARM_ROW_STATUS,
    
    CL_SNMP_SET_ALARM_SEVERITY,
    CL_SNMP_SET_ALARM_ENABLE,           
    CL_SNMP_SET_ALARM_SUSPEND,
    CL_SNMP_SET_ALARM_CLEAR,
    CL_SNMP_SET_ALARM_ROW_STATUS       
}clSnmpOpsE;


typedef enum {
    CL_SNMP_OBJ_IM = 0,
    CL_SNMP_IM_MOID,
    CL_SNMP_IM_ASVS,
    CL_SNMP_IM_MO_ROW_STATUS,
    CL_SNMP_OBJ_COMPONENT ,
    CL_SNMP_COMPONENT_ID,
    CL_SNMP_COMPONENT_NAME,
    CL_SNMP_COMPONENT_OPRATIONAL_STATUS,
    CL_SNMP_COMPONENT_PRESENCE_STATUS,
    CL_SNMP_COMPONENT_LOG_LEVEL,
    CL_SNMP_ALARM,
    CL_SNMP_ALARM_ID,
    CL_SNMP_ALARM_SEVERITY,
    CL_SNMP_ALARM_OP_STATE,
    CL_SNMP_ALARM_ENABLE,              
    CL_SNMP_ALARM_SUSPEND,
    CL_SNMP_ALARM_CLEAR,
    CL_SNMP_ALARM_ROW_STATUS,
    CL_SNMP_ATTR_MAX			
} ClSnmpAttrListE;

typedef enum
{
    CL_SNMP_COMPONENT_OPERATIONAL_STATE_NOTIFY = 1,
    CL_SNMP_COMPONENT_PRESENCE_STATE_NOTIFY,
    CL_SNMP_COMPONENT_LOG_LEVEL_CHANGE_NOTIFY,
    CL_SNMP_MO_ROW_STATUS_NOTIFY,
    CL_SNMP_ALARM_ROW_STATUS_NOTIFY,
    CL_SNMP_DEFAULT_NOTIFY
}ClSnmpTrapE;

typedef enum
{
    CL_SNMP_ACTIVE = 1,
    CL_SNMP_NOT_ON_SERVICE,
    CL_SNMP_NOT_READY,
    CL_SNMP_CREATE_AND_GO,
    CL_SNMP_CREATE_AND_WAIT,
    CL_SNMP_DESTROY
}ClSnmpRowStatus;

/*****************************************************************************/
/* Enums to identify bit map of available services                          */
/*****************************************************************************/
typedef enum
{
    CL_SNMP_FAULT_SERVICE = 0,
    CL_SNMP_ALARM_SERVICE,
    CL_SNMP_PROVISION_SERVICE,
    CL_SNMP_PERFORMANCE_SERVICE,
    CL_SNMP_CHASSIS_MGMT_SERVICE,
    CL_SNMP_BOOT_MGMT_SERVICE,
    CL_SNMP_RMS_MGMT_SERVICE,
    CL_SNMP_SECURITY_MGMT_SERVICE,
    CL_SNMP_EO_MGMT_SERVICE,
    CL_SNMP_MAX_SERVICES     
}ClSnmpAvailableSvcsT;

typedef enum
{
    CL_SNMP_TABLE  = 1,
    CL_SNMP_TRAP_FUNCTION_ID
}ClSnmpFunctionE;

typedef enum
{
	CL_SNMP_STRING_ATTR,
	CL_SNMP_NON_STRING_ATTR
}ClSnmpAttrDataTypeT;

typedef struct ClSnmpCmpTableIdx
{
    ClCharT  moIdx[CL_SNMP_DISPLAY_STRING_SIZE] ;
}ClSnmpCmpTableIdxT;

typedef struct ClSnmpImTableIdx
{
    ClCharT  moIdx[CL_SNMP_DISPLAY_STRING_SIZE] ;
}ClSnmpImTableIdxT;

typedef struct ClSnmpAlarmTableIdx
{
   ClCharT             moIdx[CL_SNMP_DISPLAY_STRING_SIZE];
   ClUint32T           alarmIdx;
}ClSnmpAlarmTableIdxT;


typedef struct ClSnmpScalarIdx
{
   ClUint32T           idx;
}ClSnmpScalarIdxT;

typedef struct
{
    ClInt32T tableType;
    ClCharT  oid[CL_SNMP_MAX_OID_LEN];
    ClInt32T oidLen;
    ClInt32T opCode;
    ClInt32T attrLen;
    union
    {
        ClSnmpScalarIdxT       scIdx;
        ClSnmpCmpTableIdxT     cmpIdx;
        ClSnmpImTableIdxT      imIdx;
        ClSnmpAlarmTableIdxT   almIdx;
    }info;
}ClSnmpTableIdxT;
typedef struct
{
    ClCharT moId[CL_SNMP_MAX_STR_LEN];
    ClInt32T alarmId;
    unsigned long oid[CL_SNMP_MAX_STR_LEN];
    ClInt32T oidLen;
}ClSnmpIndexT;

typedef struct ClSnmpAttrInfo
{
    ClCorAttrIdT attrId;
    ClUint32T    attrLen;
}ClSnmpAttrInfoT;

typedef struct ClAttrTable
{
    ClInt32T  tableType;
    ClUint32T  nAttr;
    ClSnmpAttrInfoT attr[CL_SNMP_MAX_ATTR_FOR_INDEX];
}ClSnmpAttrTableT;

typedef struct 
{
	ClUint8T type;
	ClUint8T size;
}ClSnmpAttrTypeT;

typedef struct
{
	ClInt32T      tableType;
	unsigned long tableOid[CL_SNMP_MAX_OID_LEN];
	ClUint8T      opCode;
	ClUint8T      numAttr;
	ClSnmpAttrTypeT attr[CL_SNMP_MAX_ATTR_FOR_INDEX];
}ClSnmpIndexTableT;
/* prototype of function pointers used:                                      */
/*****************************************************************************/
typedef ClUint32T (*snmpFuncArray[]) (   void *argv, 
                                         ClCharT *pVal, 
                                         ClInt32T *pErrCode);

ClUint32T (*fpExecuteSnmp[2]) (void* argv, 
                               ClCharT *pVal,
                               ClInt32T *pErrCode);

ClUint32T (*fpAspSnmpInit) (void);

ClUint32T (*fpSendNotification) (   void* dispMoid, 
                                    ClCharT* description, 
                                    ClInt32T status);


void * snmpSendTrap (ClCharT *dispMoid, ClCharT *description, ClInt32T status, ClUint32T snmpTrapEnum );

ClRcT executeSnmp (void *argv, char* pVal, int *pOpNum, int *pErrCode);

#ifdef __cplusplus
}
#endif
#endif

