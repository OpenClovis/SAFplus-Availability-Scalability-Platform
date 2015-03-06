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
 * ModuleName  : event
 * File        : clEventServerToClientFuncTable.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module installs the server-to-client RMD functions.
 *****************************************************************************/

#define __SERVER__
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>

#undef __CLIENT__
#include "clEventClientFuncTable.h"
#include <clEventServerToClientFuncTable.h> 

ClRcT clEventClientInstallTables(ClEoExecutionObjT *pEoObj)
{
    return clEoClientInstallTables(pEoObj, CL_EO_SERVER_SYM_MOD(gAspCltFuncTable, EVT));
}

ClRcT clEventClientUninstallTables(ClEoExecutionObjT *pEoObj)
{
    return clEoClientUninstallTables(pEoObj, CL_EO_SERVER_SYM_MOD(gAspCltFuncTable, EVT));
}
