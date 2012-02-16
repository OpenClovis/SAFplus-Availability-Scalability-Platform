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
 * File        : clEventServerToClientFuncTable.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains declaration of the server-to-client client table install functions.
 *****************************************************************************/

#ifndef __CL_EVENT_SERVER_TO_CLIENT_FUNC_TABLE_H__
#define __CL_EVENT_SERVER_TO_CLIENT_FUNC_TABLE_H__


ClRcT clEventClientInstallTables(ClEoExecutionObjT *pEoObj);
ClRcT clEventClientUninstallTables(ClEoExecutionObjT *pEoObj);


#endif
