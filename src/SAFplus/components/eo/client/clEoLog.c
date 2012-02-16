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
    
extern ClEoEssentialLibInfoT gEssentialLibInfo[];
ClRcT clEoLibLog (ClUint32T compId,ClUint32T severity, const ClCharT *msg, ...)
{
    ClRcT rc = CL_OK;
    ClCharT clEoLogMsg[CL_EOLIB_MAX_LOG_MSG] = "";
    ClEoLibIdT libId = eoCompId2LibId(compId);
    va_list args;
    
    if((rc = eoLibIdValidate(libId)) != CL_OK)
    {
        return rc;
    }
    va_start(args, msg);
    vsnprintf(clEoLogMsg, CL_EOLIB_MAX_LOG_MSG, msg, args);
    va_end(args);
    /*Dont use the return val. as of now*/
    //if(clLogCheckLog(severity) == CL_TRUE)
    {
        clEoQueueLogInfo(libId,severity,(const ClCharT *)clEoLogMsg);
    }
    return CL_OK;
}

ClRcT clEoLogInitialize(void)
{
    ClRcT rc = CL_FALSE;
    rc = clLogLibInitialize();
    if ( rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Log Open Failed\n"));
    return rc;
    }
    return CL_OK;
}

void clEoLogFinalize(void)
{
    /* Make sure we call this only if open is successfull */
    clLogLibFinalize();
}
