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
 * ModuleName  : ground
 * File        : clGroundCustom.c
 *******************************************************************************/

/*******************************************************************************
 * Description : The grounding definitions for customer or config module.
 * Some of the routines return OK or error but are not assertive.
 * These routines are meant to be defined by the user incase they are
 * required.
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
 * The below are weak and could be overriden by others
 */
ClRcT clHeapLibCustomInitialize(const ClPtrT  pConfig) CL_WEAK;
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

ClUint16T gClRmdMaxRetries=8;


#ifdef __cplusplus
}
#endif
