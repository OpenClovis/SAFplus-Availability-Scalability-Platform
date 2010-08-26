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
 * File        : clAlarmIntr2AlarmXln.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains the function that is supposed to translate the   
 * the interrupt info to alarm information model understood by the alarm 
 * service. This means the user needs to provide info about the failure  
 * status, MOID of interface on which the failure had happened and the   
 * probable cause of the interrupt.                                      
 *
 *
 *****************************************************************************/

#include <clCommon.h>
#include <clAlarmDefinitions.h>


/*
 * When the interrupt is generated in the kernel, the device driver captures
 * the interrupt information and makes use of alarm service API to inform about
 * the interrupt. The device driver provides the interrupt info thru that API.
 * Alarm service takes care of bringing that interrupt info to user space. For the
 * alarm service to process this interrupt, the interupt info needs to be 
 * translated to the information model provided to alarm service during the modeling
 * time. 
 * To achieve this, alarm service calls this function and expects the user to provide
 * the translation. The information needed by alarm service is the following:
 * 1) MOID of the interface on which the interrupt occured (mandatory)
 * 2) whether the interrupt is for assert or clear (mandatory)
 * 3) probable cause of the interrupt (mandatory)
 * 4) Additional info if required (if required)
 * 5) Additional info length (if required)
 * 
 * Parameters:
 * pIntrData [in]  pointer to the interrupt info
 * AEventPtr [out] pointer to the alarm event data strucutre.
 */

ClRcT clAlarmIntr2AlarmXlte(ClAlarmIntrDataT* pIntrData , ClAlarmEventInfoT *AEventPtr)
{
	ClRcT rc = CL_OK;

	return rc;
}	
