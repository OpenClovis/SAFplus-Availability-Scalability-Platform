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

ClRcT clHeapLibCustomInitialize(const ClHeapConfigT *pHeapConfig)
{
    ClRcT rc = CL_ERR_NOT_IMPLEMENTED;
    /*
      rc = clHeapHooksRegister(customMalloc,customRealloc,customCalloc,customFree);
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
      rc = clHeapHooksDeregister();
    */
    return rc;
}
