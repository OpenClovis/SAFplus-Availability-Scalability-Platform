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

/*
 * Default EO configurations
 */
ClEoConfigT clEoConfig CL_WEAK =
{
    CL_EO_DEFAULT_NAME,           /* EO Name                                  */
    CL_OSAL_THREAD_PRI_MEDIUM,    /* EO Thread Priority                       */
    2,                            /* No of EO thread needed                   */
    0,                            /* Required Ioc Port                        */
    (CL_EO_USER_CLIENT_ID_START + 0), 
    CL_EO_USE_THREAD_FOR_APP,     /* Thread Model                             */
    NULL,                         /* Application Initialize Callback          */
    NULL,                         /* Application Terminate Callback           */
    NULL,                         /* Application State Change Callback        */
    NULL                          /* Application Health Check Callback        */
};

/*
 * Basic libraries used by this EO. The first 6 libraries are
 * mandatory, the others can be enabled or disabled by setting to
 * CL_TRUE or CL_FALSE.
 */

ClUint8T clEoBasicLibs[] CL_WEAK =
{
    CL_TRUE,      /* Lib: Operating System Adaptation Layer   */
    CL_TRUE,      /* Lib: Timer                               */
    CL_TRUE,      /* Lib: Buffer Management                   */
    CL_TRUE,      /* Lib: Intelligent Object Communication    */
    CL_TRUE,      /* Lib: Remote Method Dispatch              */
    CL_TRUE,      /* Lib: Execution Object                    */
    CL_FALSE,     /* Lib: Object Management                   */
    CL_FALSE,     /* Lib: Hardware Adaptation Layer           */
    CL_FALSE      /* Lib: Database Adaptation Layer           */
};

/*
 * Client libraries used by this EO. All are optional and can be
 * enabled or disabled by setting to CL_TRUE or CL_FALSE.
 */

ClUint8T clEoClientLibs[] CL_WEAK =
{
    CL_FALSE,      /* Lib: Common Object Repository            */
    CL_FALSE,      /* Lib: Chassis Management                  */
    CL_FALSE,      /* Lib: Name Service                        */
    CL_FALSE,      /* Lib: Log Service                         */
    CL_FALSE,      /* Lib: Trace Service                       */
    CL_FALSE,      /* Lib: Diagnostics                         */
    CL_FALSE,      /* Lib: Transaction Management              */
    CL_FALSE,      /* NA */
    CL_FALSE,      /* Lib: Provisioning Management             */
    CL_FALSE,      /* Lib: Alarm Management                    */
    CL_FALSE,      /* Lib: Debug Service                       */
    CL_FALSE,      /* Lib: Cluster/Group Membership Service    */
    CL_FALSE,      /* Lib: PM */
};


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
