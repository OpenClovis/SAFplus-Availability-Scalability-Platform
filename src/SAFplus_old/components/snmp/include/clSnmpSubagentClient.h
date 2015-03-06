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
 * ModuleName  : snmp                                                          
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

#ifndef _CL_SNMP_SUB_AGENT_CLIENT_H_
#define _CL_SNMP_SUB_AGENT_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
    
#include <clList.h>

#include <clSnmpDataDefinition.h>
#include <clSnmpDefs.h>

typedef struct scalarInfo
{
    ClListHeadT list;
    ClCharT name[CL_MAX_NAME_LENGTH];
    ClCharT oid[CL_MAX_NAME_LENGTH];
    ClBoolT settable;
    ClUint8T type;
    ClUint32T attr;
    ClUint32T parentmo;
    ClUint32T tableType;
} ClSnmpScalarInfoT;

typedef struct columnInfo
{
    ClListHeadT list;
    ClUint32T column;
    ClSnmpScalarInfoT *s;
} ClSnmpColumnInfoT;

typedef struct tableInfo
{
    ClListHeadT list;
    ClCharT name[CL_MAX_NAME_LENGTH];
    ClCharT oid[CL_MAX_NAME_LENGTH];
    ClListHeadT indexes;
    ClListHeadT nonindexes;
    ClBoolT settable;
    ClUint32T minColumn;
    ClUint32T maxColumn;
    ClUint32T mo;
    ClUint32T nIndexes;
    ClSnmpTableIndexCorAttrIdInfoT *attrIdList;
    ClUint32T tableType;
    ClListHeadT columns;
} ClSnmpTableInfoT;

typedef struct trapInfo
{
    ClListHeadT list;
    ClCharT name[CL_MAX_NAME_LENGTH];
    ClCharT oid[CL_MAX_NAME_LENGTH];
} ClSnmpTrapInfoT;
    
typedef struct mibInfo
{
    ClCharT name[CL_MAX_NAME_LENGTH];
    ClListHeadT scalars;
    ClListHeadT tables;
    ClListHeadT traps;
    ClUint32T nEntries;
    ClUint32T nOidEntries;
    ClUint32T nTraps;
} ClSnmpMibInfoT;

typedef struct subAgentInfo
{
    ClCharT file[CL_MAX_NAME_LENGTH];
    ClMedHdlPtrT medHdl;
    ClCntHandleT oidCntHdl;
    ClSnmpMibInfoT mibInfo;
    ClUint32T moCounter;
    ClUint32T attrCounter;
    ClUint32T tableTypeCounter;
} ClSnmpSubagentInfoT;

/* ------------------------------------- */

typedef union ClSnmpIndexData
{
    ClCharT octetStr[CL_MAX_NAME_LENGTH];
    ClUint8T ipAddr[4];
    ClUint32T oid[CL_MAX_NAME_LENGTH];
    ClInt32T integer;
    /* Need to add more fields */
} ClSnmpIndexDataT;

typedef struct ClSnmpTableIndexInfo
{
    ClUint32T type; /* TODO: make this an enum */
    ClUint32T data;
} ClSnmpTableIndexInfoT;

typedef struct ClSnmpReqInfo
{
    ClInt32T tableType;
    ClCharT oid[CL_MAX_NAME_LENGTH];
    ClInt32T oidLen;
    ClSnmpOpCodeT opCode;
    ClUint32T dataLen;    
    union
    {
        ClUint32T scalarType;
        ClSnmpTableIndexInfoT tableInfo;
    } index;
} ClSnmpReqInfoT;

typedef union ClSnmpGenVal
{
    ClInt32T i;
    ClUint32T ui;
    ClCharT octet[CL_MAX_NAME_LENGTH-1];
    ClUint32T oid[CL_SNMP_MAX_OID_LEN];
    ClUint8T ip[4];
    ClUint8T opaque;
} ClSnmpGenValT;

ClRcT clSnmpSubagentClientLibInit(const ClCharT *file);

oid *get_oid_array(const char *oid_in, int *len);

#ifdef __cplusplus
}
#endif
#endif /* _CL_SNMP_SUB_AGENT_CLIENT_H_ */
