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
 * File        : clCompAppPM.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clPMApi.h>
#include <clCompAppPM.h>
#include <clCorMetaStruct.h>
#include <clLogApi.h>

/*
 * ---BEGIN_APPLICATION_CODE#includes#---
 */

/*
 * Additional #includes and globals go here.
 */

/*
 * ---END_APPLICATION_CODE---
 */

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
                                  __VA_ARGS__)

/**
 * The callbacks invoked by the OpenClovis PM module.
 */
ClPMCallbacksT gClPMCallbacks = 
{
    clCompAppPMObjectRead,
    clCompAppPMObjectReset
};

/**
 * This callback will get invoked by OpenClovis PM module in the configured 
 * gClPMTimerInt interval to retrieve the PM attributes values of the MOs registered using 
 * clPMStart() (Please refer OpenClovis API reference guide to know more about clPMStart() and 
 * clPMStop() APIs). Also this is invoked when a PM attribute is read from north bound.
 */
ClRcT clCompAppPMObjectRead(ClHandleT txnHandle, ClPMObjectDataPtrT pObjectData)
{
    ClRcT rc = CL_OK;

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "**** Inside the function : [%s] ****", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc; 
}

/**
 * This callback will get invoked whenever the PM Reset Attribute's value is SET 
 * from north-bound. The user shall reset the PM attributes values in this callback.
 */
ClRcT clCompAppPMObjectReset(ClHandleT txnHandle, ClPMObjectDataPtrT pObjectData)
{
    ClRcT rc = CL_OK;

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "**** Inside the function : [%s] ****", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;
}
