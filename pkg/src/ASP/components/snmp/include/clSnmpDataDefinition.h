/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : snmp                                                          
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * FIXME: TO DO
 *
 *
 *****************************************************************************/

/**
 *  \defgroup group32 Debug
 *  \ingroup group5
 */


/**
 *  \file
 *  \ingroup group32
 */

/**
 *  \addtogroup group32
 *  \{
 */

#ifndef  _CL_SNMP_DATA_DEFINITION_H_
#define _CL_SNMP_DATA_DEFINITION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clMedApi.h>
#include <clAlarmDefinitions.h>

/**
 * This is a function pointer which is called when a trap has to be generated.
 */
typedef void (*ClSnmpAppNotifyCallBackT)(CL_IN ClCharT* trapDes, CL_IN ClCharT *pData, CL_IN ClUint32T dataLen);
    
typedef struct
{
/**
 *  SNMP OID.
 */
    ClMedAgentIdT   	agntId;		

/**
 * COR ID.
 */	
    ClCorMOIdT		moId; 

/**
 *  Attribute in COR MO.
 */                 
     ClInt32T		corAttrId[20];             
}ClSnmpOidMapTableItemT;

typedef struct 
{

/**
 * SNMP OID.
 */
    ClMedAgentIdT         agntId; 

/**
 *  COR OID.
 */
    ClCorMOIdT   	moId;		

/**
 *  Attribute in COR MO.
 */                 
     ClInt32T		corAttrId[20];             

/**
 * Probable Cause. 
 */              
    ClUint32T probableCause; 

/**
 * Specific Problem.
 */
    ClAlarmSpecificProblemT specificProblem;
 
/** 
 * Trap Description.
 */            
    ClUint8T*               pTrapDes;    
}ClSnmpTrapOidMapTableItemT;

/**
 * You must populate this structure as described in sub-agent template. Clovis SNMP uses this
 * to call back instance translator and compare instance functions.
 */

 typedef struct
{
/**
 *   OID of the sub-agent
 */
    ClCharT *oid;
/**
 *  Table Id.
 */
    ClInt32T  tableType;
/**
 * Callback for Instance translator.
 */
    ClMedInstXlatorCallbackT  fpInstXlator;
/**
 * Callback for Instance compare.
 */
    ClCntKeyCompareCallbackT fpInstCompare;
}ClSnmpOidTableT;

/**
 * You must populate this structure as described in sub-agent template. Clovis SNMP uses this
 * to call back instance notification functions.
 */
typedef struct
{
/**
 *   OID of the notification object
 */
    ClCharT* trapOid;
/**
 *  Callback function for notification. 
 */
    ClSnmpAppNotifyCallBackT fpAppNotify;
}ClSnmpNotifyCallbackTableT;
		    

/**
 * Enums for SNMP operations.
 */
typedef enum 
{
/**
 *   Set operation
 */
    CL_SNMP_SET = 1,
/**
 *   Get operation
 */    
     CL_SNMP_GET,
/**
 *   Get next operation
 */    
    CL_SNMP_GET_NEXT,
/**
 *   Create operation
 */    
    CL_SNMP_CREATE,
/**
 *  Delete operation
 */    
    CL_SNMP_DELETE,
/**
 *   Walk operation
 */    
    CL_SNMP_WALK,
/**
 *   Get first operation
 */    
    CL_SNMP_GET_FIRST,
/**
 *   Get services on a MO
 */    
    CL_SNMP_GET_SVCS,
/**
 *   Containment set operation
 */
    CL_SNMP_CONTAINMENT_SET,
/**
 *  Maximum operation
 */    
    CL_SNMP_MAX
} ClSnmpOpCodeT;

extern ClSnmpOidMapTableItemT  clOidMapTable[];
extern ClSnmpTrapOidMapTableItemT  clTrapOidMapTable[];

#ifdef __cplusplus
}
#endif
#endif /* _CL_SNMP_DATA_DEFINITION_H_ */
