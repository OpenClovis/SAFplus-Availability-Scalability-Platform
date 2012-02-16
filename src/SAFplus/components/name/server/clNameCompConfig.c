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
 * ModuleName  : name
 * File        : clNameCompConfig.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Name Service Configuration module
 *******************************************************************************/

/* INCLUDES */
#include <clCommon.h>
#include <clOsalApi.h>
#include <clNameErrors.h>
#include <clNameIpi.h>
#include <clDebugApi.h>

/*local ioc address*/
extern ClUint32T	myId;
extern ClUint32T gClnsMaxNoEntries;
extern ClUint32T gClnsMaxNoGlobalContexts;
extern ClUint32T gClnsMaxNoLocalContexts;


/**
 *  IOC  Layer configuration module entry-point.
 *
 *  This API contains the IOC Layer configuration module entry-point.
 *                                                                        
 *  @param none
 *
 *  @returns CL_OK  - Success<br>
 */

ClRcT clNameCompCfg(void)
{
    ClRcT      rc = CL_OK;
    ClNameSvcConfigT configParams;
                                                                                                                             
                                                                                                                             
    /* Set the configuration parameters */
                                                                                                                             
    configParams.nsMaxNoEntries        = gClnsMaxNoEntries;
    configParams.nsMaxNoGlobalContexts = gClnsMaxNoGlobalContexts;
    configParams.nsMaxNoLocalContexts  = gClnsMaxNoLocalContexts;
                                                                                                                             
    /* call the Comp Mgr Component configuration module entry-point */
    if ((rc = clNameInitialize(&configParams)) != CL_OK )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Name Service Init Failed"));
    }
                                                                                                                             
    return (rc);
}

