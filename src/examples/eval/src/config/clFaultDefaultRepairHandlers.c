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
 * ModuleName  : fault                                                         
 * File        : clFaultDefaultRepairHandlers.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file provides the default repair handlers for fault global manager.
 *
 ***************************************************************************/
/* system includes */

/* ASP includes */

/* fault includes */
#include <clFaultDefinitions.h>
#include <clDebugApi.h>


   ClRcT ClFaultNodeRestartRequest(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported by Fault Local Mgr \n"));
    return CL_OK;
}
ClRcT ClFaultCompRestartRequest(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported locally \n"));
    return CL_OK;
}
ClRcT clFaultEscalate(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported locally \n"));
    return CL_OK;
}
