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
 * File        : alarmMSOMetaStruct.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains definitions for the alarmMSOClass.                
 **************************************************************************/


/* Include Files */
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clAlarmDefinitions.h>
#include <ipi/clAlarmIpi.h>

#define ALARM_INFO_CLASS_ID 1  /* SSS fix this. this has to unique */
#define CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY 1

/* definitions for alarm info containment class */

ClCorAttribPropsT clContainmentBaseClassAttrList [] = 
{
  { "CLCONTAINMENTATTRVALIDENTRY",CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY,CL_COR_UINT8, 0,0,0xff,0},
  {"UNKNOWN_ATTRIBUTE",  0,0,0,0,0,0}
};					
ClCorMoClassDefT clAlarmBaseClass []=
{
  {"CLALARMBASECLASS",
   CL_ALARM_BASE_CLASS, 
   0,
   0,
   clContainmentBaseClassAttrList, 
   NULL, 
   NULL, 
   NULL
  },
  { "UKNOWN_CLASS " , 0 , 0 , 0 , 
  NULL , NULL, NULL, NULL }
};	


ClCorAttribPropsT alarmInfoAttrList [] = 
{
  { "PROBABLECAUSE", CL_ALARM_PROBABLE_CAUSE, CL_COR_UINT32, 0, 0, 0xffff},
  { "GENERATIONRULE", CL_ALARM_GENERATION_RULE, CL_COR_UINT32, 0, 0, 0xffff},  	
  { "SUPPRESSIONRULE", CL_ALARM_SUPPRESSION_RULE, CL_COR_UINT32, 0, 0, 0xffff},
  { "GENERATIONRULERELATION", CL_ALARM_GENERATE_RULE_RELATION, CL_COR_UINT32, 0, 0, 0xffff},
  { "SUPPRESSIONRULERELATION", CL_ALARM_SUPPRESSION_RULE_RELATION, CL_COR_UINT32, 0, 0, 0xffff},  
  { "AFFECTEDALARMS", CL_ALARM_AFFECTED_ALARMS, CL_COR_UINT32, 0, 0, 0xffff},  
  { "ALARMENABLE", CL_ALARM_ENABLE, CL_COR_UINT8, 1, 0, 0xff},
  { "ALARMSUSPEND",CL_ALARM_SUSPEND, CL_COR_UINT8, 0, 0, 0xff},
  { "ALARMACTIVE", CL_ALARM_ACTIVE, CL_COR_UINT8, 0, 0, 0xff},
  { "ALARMCLEAR", CL_ALARM_CLEAR, CL_COR_UINT8, 0, 0, 0xff},
  { "CATEGORY", CL_ALARM_CATEGORY, CL_COR_UINT8, 0, 0, 0xff},  
  { "SPECIFIC_PROBLEM", CL_ALARM_SPECIFIC_PROBLEM, CL_COR_UINT32, 0, 0, 0xffff},  
  { "SEVERITY", CL_ALARM_SEVERITY, CL_COR_UINT8, 0, 0, 0xff},  
  { "EVENTTIME", CL_ALARM_EVENT_TIME, CL_COR_INT64, 0, 0, 0x7fffffffffffffff},  
  {"UNKNOWN_ATTRIBUTE",  0,0,0,0,0}
};

ClCorMoClassDefT alarmInfo[] =
{
  { "ALARMINFO", CL_ALARM_INFO_CLASS , CL_ALARM_BASE_CLASS , 1, 
    alarmInfoAttrList , NULL , NULL, NULL},
  { "UNKNOWN_CLASS", 0 , 0 , 0, 
  NULL, NULL, NULL, NULL}
};


ClCorAttribPropsT	alarmMsoAttrList[] =
{
  { "ALMSTOBEPOLLED", CL_ALARM_MSO_TO_BE_POLLED, CL_COR_UINT8, 0, 0, 0xff },
  { "ALARMPOLLINGINTERVAL", CL_ALARM_POLLING_INTERVAL, CL_COR_UINT32, 0, 0, 0xffff },
  { "ALMS_AFTER_SOAKING_BITMAP", CL_ALARM_ALMS_AFTER_SOAKING_BITMAP, CL_COR_UINT32, 0, 0, 0xffff},      
  { "PUBLISHEDALARMS", CL_ALARM_PUBLISHED_ALARMS, CL_COR_UINT32, 0, 0, 0xffff },
  { "SUPPRESSEDALARMS", CL_ALARM_SUPPRESSED_ALARMS, CL_COR_UINT32, 0, 0, 0xffff },
  { "PARENTENTITYFAILED", CL_ALARM_PARENT_ENTITY_FAILED, CL_COR_UINT8, 0, 0, 0xff },  
  { "UNKNOWN_ATTRIBUTE", 0, 0, 0, 0, 0 }
};

ClCorArrAttribDefT   alarmMsoArrAttrList[] =
{
    { "PROBABLECAUSE", CL_ALARM_ID, CL_COR_UINT32, CL_ALARM_MAX_ALARMS },
    { "UNKNOWN_ATTRIBUTE",0,0,0 }
};

ClCorContAttribDefT alarmMsoContAttrList[] = 
{
{"ALMINFOCONTCLASS", CL_ALARM_INFO_CONT_CLASS , CL_ALARM_INFO_CLASS,
  1 , CL_ALARM_MAX_ALARMS , 0 }, 
{"UNKNOW_ATTRIB", 0 , 0 , 0 , 0 , 0 }
};
    
