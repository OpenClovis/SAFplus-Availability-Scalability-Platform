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
 * ModuleName  : alarm
 * File        : clAlarmServer.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _CL_ALARM_SERVER_H_
#define _CL_ALARM_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clAlarmDefinitions.h>
#include <clAlarmCommons.h>

/****************************************************************************
 * Constants and macro definitions
 ***************************************************************************/
    
/***************************************************************************
 * forward declaration of functions belonging to function list table
 **************************************************************************/

ClRcT VDECL(clAlarmAddProcess)     (ClEoDataT data, 
                                   ClBufferHandleT  inMsgHandle,
                                   ClBufferHandleT  outMsgHandle);

ClRcT VDECL(clAlarmHandleToInfoGet) (ClEoDataT data, 
                                   ClBufferHandleT  inMsgHandle,
                                   ClBufferHandleT  outMsgHandle);

ClRcT VDECL(clAlarmInfoToAlarmHandleGet) (ClEoDataT data, 
                                   ClBufferHandleT  inMsgHandle,
                                   ClBufferHandleT  outMsgHandle);

ClRcT VDECL_VER(clAlarmServerAlarmReset, 4, 1, 0) (ClEoDataT data, 
                                   ClBufferHandleT  inMsgHandle,
                                   ClBufferHandleT  outMsgHandle);

/***************************************************************************
 * forward declaration of function belonging to the main file.
 **************************************************************************/

ClRcT clAlarmServerAlarmChannelInit();
ClRcT clAlarmClientTableRegister();

/**************************************************************************
 * Externs
 *************************************************************************/

/* Added new parameter 'outMsgHandle' for passing the alarm handle to
 *  the client */

extern ClRcT      clAlarmServerEngineAlarmProcess(ClCorObjectHandleT hMSOObj, 
                                        ClAlarmInfoT *pAlarmInfo,
                                        ClBufferHandleT outMsgHandle);
/**************************************************************************/

#endif /* _CL_ALARM_SERVER_H_ */
