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
 * ModuleName  : alarm
 * File        : clAlarmClientTable.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 *          This file provide implemantation of the Eonized application main function
 *          This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/

#undef __SERVER__
#define __CLIENT__

/* Standard Inculdes */
#include <string.h>  /* strcpy is needed for eo name */
#include <time.h>
#include <sys/time.h>

/* ASP Includes */
#include <clCommon.h>
#include <clEoApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clAlarmServer.h>
#include <clAlarmServerFuncTable.h>
#include <clAlarmClientFuncTable.h>

ClRcT clAlarmClientTableRegister()
{
    ClRcT rc = CL_OK;

    /* Registering the alarm client -> alarm server table */
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, ALM), CL_IOC_ALARM_PORT);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Alarm client EO table register failed. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Registering the alarm server -> alarm owner table */
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, ALM_CLIENT), CL_IOC_ALARM_PORT);
    if (rc != CL_OK)
    {
        clLogError("ALM", "INT", "Alarm client EO table register failed. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}
