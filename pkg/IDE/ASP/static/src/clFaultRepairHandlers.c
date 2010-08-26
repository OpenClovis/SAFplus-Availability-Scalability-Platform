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
 * File        : clFaultRepairHandlers.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
** @file clFaultRepairHandlers.c
*
* This file contains repair handlers based on mo class type
*
*******************************************************************************
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clEoApi.h>
#include <clFaultRepairHandlers.h>
#include <clDebugApi.h>
#include <saAmf.h>

/*
 * ---BEGIN_APPLICATION_CODE---
 */

/*
 * ---END_APPLICATION_CODE---
 */

ClRcT clFaultDefaultRepairHandler(ClAlarmInfoT *pAlarmInfo,
		SaAmfRecommendedRecoveryT recoveryAction)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n inside ClFaultDefaultRepairHandler \n"));
    
    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    /*
     * ---END_APPLICATION_CODE---
     */
		
    return CL_OK;
}

ClFaultRepairHandlerTableT gRepairHdlrList[] =
{
    {65536,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {65537,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {65538,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {65541,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {65543,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {65545,  (ClFaultRepairHandlerT) clFaultDefaultRepairHandler}, 
    {0,  (ClFaultRepairHandlerT) NULL}, 

};


