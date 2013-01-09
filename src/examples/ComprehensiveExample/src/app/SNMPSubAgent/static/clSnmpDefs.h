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
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : static
 * File        : clSnmpDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _CL_SNMP_DEFS_H_
#define _CL_SNMP_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clMedApi.h>
#include <clAlarmUtils.h>

#define CL_SNMP_DISPLAY_STRING_SIZE  255
#define CL_SNMP_MAX_OID_LEN          128 
#define CL_SNMP_MAX_STR_LEN          255
#define CL_SNMP_MAX_MOID_STR_LEN    1024
#define CL_SNMP_MAX_ATTR_FOR_INDEX     8

#define CL_SNMP_ANY_MODULE             0
#define CL_SNMP_ANY_OTHER_ERROR   0xffff

/*
 * Error codes (the value of the field error-status in PDUs)
 */
/*
 * in SNMPv1, SNMPsec, SNMPv2p, SNMPv2c, SNMPv2u, SNMPv2*, and SNMPv3 PDUs
 */
#define CL_SNMP_ERR_NOERROR                (0)     /* XXX  Used only for PDUs? */
#define CL_SNMP_ERR_TOOBIG                  (1)
#define CL_SNMP_ERR_NOSUCHNAME             (2)
#define CL_SNMP_ERR_BADVALUE               (3)
#define CL_SNMP_ERR_READONLY               (4)
#define CL_SNMP_ERR_GENERR                  (5)
   
/*
 * in SNMPv2p, SNMPv2c, SNMPv2u, SNMPv2*, and SNMPv3 PDUs
 */
#define CL_SNMP_ERR_NOACCESS        (6)
#define CL_SNMP_ERR_WRONGTYPE       (7)
#define CL_SNMP_ERR_WRONGLENGTH     (8)
#define CL_SNMP_ERR_WRONGENCODING       (9)
#define CL_SNMP_ERR_WRONGVALUE      (10)
#define CL_SNMP_ERR_NOCREATION      (11)
#define CL_SNMP_ERR_INCONSISTENTVALUE   (12)
#define CL_SNMP_ERR_RESOURCEUNAVAILABLE (13)
#define CL_SNMP_ERR_COMMITFAILED        (14)
#define CL_SNMP_ERR_UNDOFAILED      (15)
#define CL_SNMP_ERR_AUTHORIZATIONERROR  (16)
#define CL_SNMP_ERR_NOTWRITABLE     (17)

/*
 * in SNMPv2c, SNMPv2u, SNMPv2*, and SNMPv3 PDUs
 */
#define CL_SNMP_ERR_INCONSISTENTNAME    (18)

extern ClCntHandleT gClSnmpOidContainerHandle;

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

typedef struct ClSnmpAttrInfo
{
    ClCorAttrIdT attrId;
    ClUint32T    attrLen;
}ClSnmpAttrInfoT;

typedef struct ClAttrTable
{
    ClUint32T  tableType;
    ClUint32T  nAttr;
    ClSnmpAttrInfoT attr[CL_SNMP_MAX_ATTR_FOR_INDEX];
}ClSnmpAttrTableT;

typedef struct
{
    ClUint8T type;
    ClUint32T size;
}ClSnmpAttrTypeT;

typedef struct
{
    ClUint32T       sysErrorType;       /* Error returned by ASP */
    ClUint32T       sysErrorId;             /* Error id returned by ASP */
    ClUint32T       agntErrorId;            /* Error id in SNMP */
}ClSnmpOidErrMapTableItemT;

typedef struct
{
    ClCorAttrPathPtrT pAttrPath;
    ClCorAttrIdT      attrId;
} ClSnmpTableIndexCorAttrIdInfoT;

typedef ClRcT (* ClSnmpTblIndexCorAttrGetCallbackT) (CL_IN ClPtrT pInst, 
                                     CL_OUT ClPtrT pCorAttrValueDescList);

/**
 * You must populate this structure as described in sub-agent template. Clovis SNMP uses this
 * to call back instance translator and compare instance functions.
 */

typedef struct
{
    ClUint8T                          tableType;
    const ClCharT *                   oid;
    ClUint8T                          numAttr;
    ClSnmpAttrTypeT                   attr[CL_SNMP_MAX_ATTR_FOR_INDEX];
    ClSnmpTblIndexCorAttrGetCallbackT fpTblIndexCorAttrGet;
    ClMedInstXlatorCallbackT          fpInstXlator;
    ClCntKeyCompareCallbackT          fpInstCompare;
}ClSnmpOidInfoT;

typedef struct ClTrapPayLoad{
    ClUint32T  oidLen;
    ClUint32T  *objOid;
    ClUint32T  valLen;
    void       *val;
}ClTrapPayLoadT;

typedef struct ClTrapPayLoadList{
    ClUint32T      numTrapPayloadEntry;
    ClTrapPayLoadT *pPayLoad;
}ClTrapPayLoadListT;


typedef ClRcT (* ClSnmpAppNtfyIndexGetCallbackT)(CL_IN ClUint32T objectType, CL_IN ClCorMOIdPtrT pMoId, 
	        CL_OUT ClAlarmUtilTlvInfoPtrT pTlvList);


typedef void (* ClSnmpAppNtfyCallbackT)(CL_IN ClTrapPayLoadListT* pTrapPayLoadList);
/**
 * You must populate this structure as described in sub-agent template. Clovis SNMP uses this
 * to call back instance notification functions.
 */
typedef struct
{
/**
 *   OID of the notification object
 */
    const ClCharT* pTrapOid;
/**
 *  Callback function for generating notification TLV. 
 */
    ClSnmpAppNtfyIndexGetCallbackT fpAppNotifyIndexGet;
/**
 *  Callback function for notification. 
 */
    ClSnmpAppNtfyCallbackT fpAppNotify;
}ClSnmpNtfyCallbackTableT;


#ifdef __cplusplus
}
#endif

#endif
