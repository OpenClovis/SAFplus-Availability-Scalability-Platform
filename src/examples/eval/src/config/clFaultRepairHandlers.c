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

ClRcT clFaultDefaultRepairHandler(ClAlarmInfoT *pAlarmInfo,
		SaAmfRecommendedRecoveryT recoveryAction)
{
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n inside ClFaultDefaultRepairHandler \n"));
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


