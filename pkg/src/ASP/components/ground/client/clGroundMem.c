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
 * File        : clGroundMem.c
 *******************************************************************************/

/*******************************************************************************
 * Description : The grounding definitions for MEM module.
 * Some of the routines return OK or error but are not assertive.
 * These routines are meant to be defined by the user incase they are
 * required.
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <assert.h>

/*
 * The below are weak and could be overriden by others
 */
ClRcT clHeapLibInitialize(const ClPtrT pConfig) CL_WEAK;
ClRcT clHeapLibFinalize(void) CL_WEAK;
ClRcT clMemStatsInitialize(const ClPtrT pConfig) CL_WEAK;
ClRcT clMemStatsFinalize(void) CL_WEAK;
ClRcT clHeapLibCustomInitialize(const ClPtrT pConfig) CL_WEAK;
ClRcT clHeapLibCustomFinalize(void) CL_WEAK;

/*
 * clHeapLibCustomInitialize and Finalize for users own heap.
 *
 */

ClRcT clHeapLibCustomInitialize(const ClPtrT pConfig)
{
    return CL_ERR_NOT_IMPLEMENTED;
}

ClRcT clHeapLibCustomFinalize(void)
{
    return CL_ERR_NOT_IMPLEMENTED;
}

/*
 * The below functions shouldnt be invoked without linking
 * with the right library. Hence assert.
 */

ClRcT clHeapLibInitialize(const ClPtrT pConfig)
{
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clHeapLibFinalize(void)
{
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clMemStatsInitialize(const ClPtrT pConfig)
{
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clMemStatsFinalize(void)
{
    assert(CL_FALSE);
    return CL_OK;
}

