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
 * File        : clGroundCor.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <assert.h>

#include <clCommon.h>
#include <clCommonErrors.h>

/* These files are only used to compile clEo.c in the main component.
   If a library is being used by a component and it fills out the
   ASP_LIBS part in Makefile properly, then this file will not get
   linked into the executable by 'ld'. Yes, 'ld' is more intelligent
   than us. */
ClRcT clCorClientInitialize(void)
{
    /* empty function to satisfy linker, but this should never get
       called. */
    assert(CL_FALSE);
    return CL_OK;
}

ClRcT clCorClientFinalize(void)
{
    /* empty function to satisfy linker, but this should never get
       called. */
    assert(CL_FALSE);
    return CL_OK;
}

