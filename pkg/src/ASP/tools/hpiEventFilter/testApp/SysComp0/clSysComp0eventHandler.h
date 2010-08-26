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
* ModuleName  : SysComp0
* File        : clSysComp0eventHandler.h
*******************************************************************************/

/*******************************************************************************
* Description :
* This file contains User callback declarations.
*
*******************************************************************************/


#ifdef __cplusplus
extern "C"{
#endif

#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <clHpiEventFilter.h>


extern void fantray_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void event_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void temperature_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void voltage_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void processor_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void current_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

extern void hotswap_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);

#if 0
extern void chassis_wide_temperature_callback(SaHpiEventT *eventPtr, SaHpiRptEntryT *rptEntryPtr, SaHpiRdrT *rdrPtr);
#endif

#endif

#ifdef __cplusplus
}
#endif
