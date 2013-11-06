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
 * ModuleName  : eolog
File        : clEoLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provides for reading the Log related functionality 
 *
 *
 ****************************************************************************/

/*
 * Standard header files.
 */
#include <stdarg.h>
#include <string.h>
/*
 * ASP header files.
 */
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clEoLibs.h>

#define EO_LOG_AREA_LOG		"LOG"	
    
extern ClEoEssentialLibInfoT gEssentialLibInfo[];

ClRcT clEoLogInitialize(void)
{
    ClRcT rc = CL_FALSE;
    rc = clLogLibInitialize();
    if ( rc != CL_OK)
    {
        clLogError(EO_LOG_AREA_LOG,CL_LOG_CONTEXT_UNSPECIFIED,
                   "Log Open Failed\n");
    return rc;
    }
    return CL_OK;
}

void clEoLogFinalize(void)
{
    /* Make sure we call this only if open is successfull */
    clLogLibFinalize();
}
