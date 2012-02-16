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
 * ModuleName  : med
 * File        : clMedDebug.h
 *******************************************************************************/

#ifndef _CL_MED_HPI_DEBUG_H_
#define _CL_MED_HPI_DEBUG_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clAlarmDefinitions.h>


typedef enum ClMedDebugInfoType
{
    CL_MED_DEBUG_MOID = 0,
    CL_MED_DEBUG_INST,
}ClMedDebugInfoTypeT;
    
        
typedef struct ClMedDebugXlationWalkData
{
    void* pNativeId;
    ClUint32T length;
    ClBufferHandleT msgHdl;
    ClMedDebugInfoTypeT displayType;
}ClMedDebugXlationWalkDataT;


#define CL_MED_DEBUG_CURRENT_LEVEL 1

#define CL_MED_DEBUG_LEVEL1 1
#define CL_MED_DEBUG_LEVEL2 2
#define CL_MED_DEBUG_LEVEL3 3


#define _MED_FUN_ENTER "Fun.Ent.%s\n",__FUNCTION__
#define _MED_FUN_EXIT  "Fun.Ext.%s---------\n",__FUNCTION__


#if 0
#ifdef MED_FUNCTION_LINE_INFO_ENABLE
#endif
#endif

#define CL_MED_AREA "med"
#define CL_MED_CTX_INT "int"
#define CL_MED_CTX_UTL "uti"
#define CL_MED_CTX_N2S "n2s"
#define CL_MED_CTX_S2N "s2n"
#define CL_MED_CTX_CFG "cfg"
#define CL_MED_CTX_NTF "ntf"



    //    printf("%s:%d",__FUNCTION__,__LINE__);	
#define CL_MED_DEBUG

#ifndef CL_MED_DEBUG

#define CL_MED_DEBUG_PRINT(level, args)	

#define CL_MED_DEBUG_MO_PRINT(moid)	


#define CL_MED_DEBUG_ALARM_DATA_PRINT(alarmPayLoad) 

#define CL_MED_DEBUG_XLATE_INFO_PRINT(operation, agentId, xlateInfo, moId)
#else /* CL_MED_DEBUG */


#define CL_MED_DEBUG_PRINT(level, context, args)	\
  {\
      clLog(level, CL_MED_AREA, context, (args));   \
  }

#define CL_MED_DEBUG_MO_PRINT(moid, context)            \
  {\
    ClNameT moIdName;\
    clCorMoIdToMoIdNameGet(moid,&moIdName);\
    clLog(CL_LOG_DEBUG, CL_MED_AREA, context, "MOID[%s]\n",moIdName.value); \
  }

#define CL_MED_DEBUG_ALARM_DATA_PRINT(alarmPayLoad) \
  {\
      clLog(CL_LOG_DEBUG, CL_MED_AREA, CL_MED_CTX_NTF, "Category[%d] :ProbalCause[%d] : AlarmState[%x] :  severity[%x] : bufferLen[%d]\n", \
	   ((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.category,	\
	   ((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.probCause,	\
	   ((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.alarmState,	\
	   ((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.severity,	\
	   ((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.len);	\
      CL_MED_DEBUG_MO_PRINT(&((ClAlarmHandleInfoT*)(alarmPayLoad))->alarmInfo.moId, CL_MED_CTX_NTF); \
  }


#define CL_MED_DEBUG_XLATE_INFO_PRINT(operation, agentId, xlateInfo, moId, context) \
  {\
    if(operation == CL_MED_CREATION_BY_NORTHBOUND)\
        clLog(CL_LOG_DEBUG, CL_MED_AREA, context, "...From NBI"); \
    else\
        clLog(CL_LOG_DEBUG, CL_MED_AREA, context, "...From OI"); \
    clLog(CL_LOG_DEBUG, CL_MED_AREA, context, "Agent-ID[%s]",(agentId)->id);\
    CL_MED_DEBUG_MO_PRINT(moId,context);\
  }

#endif /* CL_MED_DEBUG */
    ClRcT clMedDebugRegister(ClMedHdlPtrT medHdl); 
    ClRcT clMedDebugDeregister();
    
#ifdef __cplusplus
}
#endif
#if 0

#endif
#endif /* _CL_MED_HPI_DEBUG_H_ */
