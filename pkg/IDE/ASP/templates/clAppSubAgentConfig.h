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
 * ModuleName  : clAppSubAgentConfig                                            
* File        : clAppSubAgentConfig.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_SNMP_CONFIG_H_
#define _CL_SNMP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************
* General ASP header files
****************************/
#include <clCorMetaData.h>
#include <clMedApi.h>
#include <clSnmpDefs.h>
#include <clSnmpDataDefinition.h>

/***************************
* Constant definition 
****************************/
#define EO_NAME    	"SampleEo"	/*Name of the eo for snmp subagent*/
#define MODULE_NAME	"clSampleMIB"	/*Module name as specified in the MIB file.*/

#define REL_CODE	'B'
#define	MAJOR_VER	0x01
#define MINOR_VER	0x01

#define PROV_GIGE_TABLE_INDEX    0  /*Entry index in the appMibInfoTbl structure for any given table.*/
#define PROV_T3_TABLE_INDEX      1  /*Entry index in the appMibInfoTbl structure for any given table.*/

/*
 * Information regarding all the tables on which the sub-agent is operating.
 */
enum CLSampleTable {
    CL_PROV_GIGE_TABLE,
    CL_PROV_T3_TABLE,
    CL_SNMP_TABLE_MAX
};

/*
 * Index information for any table.
 */
typedef struct provInfoIndex
{
    ClCharT             dispMoid[CL_SNMP_DISPLAY_STRING_SIZE];
}provInfoIndex_t;

/*The union part of this structure will change according to application MIB
 * indices. The first 5 fields before the union should not be changed*/
 
typedef struct ClSNMPRequestInfo
{
	/*This is the application independent part*/
    ClInt32T    tableType;
    ClCharT     oid[CL_SNMP_MAX_OID_LEN];
    ClInt32T    oidLen;
    ClInt32T    opCode;
	ClUint32T   dataLen;	
	/*Change the definition of union based on the indices for the application
	 * MIB*/
	union
	{
		ClUint32T scalarType;
        provInfoIndex_t provGigeInfo;
        provInfoIndex_t provT3Info;
	}index;
}ClSNMPRequestInfoT;

/*Function prototypes.*/
ClRcT sampleInstXlator (const struct ClMedAgentId *pAgntId,
                     ClCorMOIdPtrT         hmoId,
                     ClCorAttrPathPtrT containedPath,
                     void**         pRetInstData,
                     ClUint32T     *instLen,
                     ClUint32T     create);

ClInt32T  sampleInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);

extern void init_appMIB(void);

/*Global structures.*/
extern ClSnmpOidTableT appOidTable[];
extern ClSnmpIndexTableT appMibInfoTbl[];
extern ClSnmpNotifyCallbackTableT clSnmpAppCallback[];

#ifdef __cplusplus
}
#endif
#endif
