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
 * ModuleName  : ground
 * File        : clGroundOMAlarmStruct.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contain the structure definition for the Alarm and OM associated
 * with the component.
 * These strucutures are separated from the clGroundAlarm.c and clGroundOM.c file
 * as these should be linked only if the definition is missing from the component
 * linking alarm client library for a purify/static build. This would help not to
 * generate the dummy files (alarmMetastruct and OAMP files) in the scenario when 
 * the component is just linked to alarm client library without having any resources
 * associated to it.
 *
 *******************************************************************************/
#include <clAlarmDefinitions.h>
#include <clOmApi.h>
#include <clPMApi.h>

ClAlarmComponentResAlarmsT *appAlarms CL_WEAK;

ClOmClassControlBlockT *pAppOmClassTbl CL_WEAK;

ClUint32T appOmClassCnt CL_WEAK;

CL_WEAK ClPMCallbacksT gClPMCallbacks =
{
    NULL,
    NULL
};
