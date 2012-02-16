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
 * ModuleName  : alarm                                                         
 * File        : clAlarmDebug.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *******************************************************************************/

#ifndef _CL_ALARM_DEBUG_H_
#define _CL_ALARM_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCorMetaData.h>
#include <clEoApi.h>


extern ClRcT clAlarmDebugRegister(ClEoExecutionObjT* pEoObj);
extern ClRcT clAlarmDebugDeregister(ClEoExecutionObjT* pEoObj);
extern ClRcT clAlarmCliRaise(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCliClear(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCliQuery(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCliShowRaisedAlarms(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCliHandlesShow(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCliInfoShow(ClUint32T argc, ClCharT **argv , ClCharT** ret);
extern ClRcT clAlarmCorXlateMOPath(char *path, ClCorMOIdPtrT cAddr);
extern ClRcT clAlarmCliShowAssociatedAlarms(ClUint32T argc, ClCharT** argv, ClCharT** ret);
extern ClRcT clAlarmCliShowAlarmSeverityList(ClUint32T argc, ClCharT** argv, ClCharT** ret);


#ifdef __cplusplus
}
#endif


#endif /* _CL_ALARM_DEBUG_H_ */
