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
 * File        : clGroundEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>
/*
 * The below are weak and could be overriden by others
 */
ClRcT clEoLibInitialize(void) CL_WEAK;
ClRcT clEoLibFinalize(void) CL_WEAK;

ClRcT clEoLibLog(ClUint32T compId,ClUint32T severity,const ClCharT *msg,...) CL_WEAK;
ClRcT clEoWaterMarkHit(ClCompIdT compId, ClWaterMarkIdT wmId, ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList) CL_WEAK;

ClRcT clEoProgNameGet(ClCharT *pName,ClUint32T maxSize) CL_WEAK;
ClCharT* clEoNameGet(void) CL_WEAK;


/* These files are only used to compile clEo.c in the main component.
   If a library is being used by a component and it fills out the
   ASP_LIBS part in Makefile properly, then this file will not get
   linked into the executable by 'ld'. Yes, 'ld' is more intelligent
   than us. */
ClRcT clEoLibInitialize(void)
{
    /* empty function to satisfy linker, but this should never get
       called. */
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clEoLibFinalize(void)
{
    /* empty function to satisfy linker, but this should never get
       called. */
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clEoLibLog(ClUint32T compId,ClUint32T severity,const ClCharT *msg,...)
{
    return CL_OK;
}

ClRcT clEoWaterMarkHit(ClCompIdT compId, ClWaterMarkIdT wmId, ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList)
{
    return CL_OK;
}

/*
 * Get the prog name of the guy using the MEM module:
 * Stuff the pid inside
*/
ClRcT clEoProgNameGet(ClCharT *pName,ClUint32T maxSize)
{
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(pName == NULL)
    {
        goto out;
    }
    snprintf(pName,maxSize,"%d",(int)getpid());
    rc = CL_OK;
    out:
    return rc;
}

ClCharT* clEoNameGet(void)
{
    return "NOT_AN_EO";
}
