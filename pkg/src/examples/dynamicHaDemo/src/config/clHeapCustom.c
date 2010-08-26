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
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : static
 * File        : clHeapCustom.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * Customers should call hooks register with malloc,realloc,calloc,free
 * when heap is configured in custom mode:
 * The configuration is also available for use.
 * Currently disabled:
*/
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHeapApi.h>

/*
 * ---BEGIN_APPLICATION_CODE---
 */
 	
/*
 * ---END_APPLICATION_CODE---
 */

ClRcT clHeapLibCustomInitialize(const ClHeapConfigT *pHeapConfig)
{
    ClRcT rc = CL_ERR_NOT_IMPLEMENTED;

	/*
	 * ---BEGIN_APPLICATION_CODE---
	 */

	
	//rc = clHeapHooksRegister(customMalloc,customRealloc,customCalloc,customFree);
	

	/*
	 * ---END_APPLICATION_CODE---
	 */

    return rc;
}

/*
 * Customers should call hooks deregister at their finalize.
 * Currently disabled:
*/
ClRcT clHeapLibCustomFinalize(void)
{
    ClRcT rc = CL_ERR_NOT_IMPLEMENTED;

	/*
	 * ---BEGIN_APPLICATION_CODE---
	 */

	//rc = clHeapHooksDeregister();
	

	/*
         * ---END_APPLICATION_CODE---
         */

    return rc;
}
