/*
 * 
 *   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
 * 
 *   The source code for this program is not published or otherwise divested
 *   of its trade secrets, irrespective of what has been deposited with  the
 *   U.S. Copyright office.
 * 
 *   No part of the source code  for this  program may  be use,  reproduced,
 *   modified, transmitted, transcribed, stored  in a retrieval  system,  or
 *   translated, in any form or by  any  means,  without  the prior  written
 *   permission of OpenClovis Inc
 */
/*
 * Build: 4.0.0
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


/*
 * ---BEGIN_APPLICATION_CODE---
 */

/*
 * ---END_APPLICATION_CODE---
 */

   ClRcT ClFaultNodeRestartRequest(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported by Fault Local Mgr \n"));

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    /*
     * ---END_APPLICATION_CODE---
     */
			
    return CL_OK;
}
ClRcT ClFaultCompRestartRequest(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported locally \n"));

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    /*
     * ---END_APPLICATION_CODE---
     */

    return CL_OK;
}
ClRcT clFaultEscalate(ClFaultRecordPtr hRec)
{
    /* perform actions for faults reported by compMgr */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Taking appropriate action for the \
				fault reported locally \n"));

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    /*
     * ---END_APPLICATION_CODE---
     */

    return CL_OK;
}
