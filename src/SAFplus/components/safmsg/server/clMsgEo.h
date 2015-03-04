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
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#ifndef __CL_MSG_EO_H__
#define __CL_MSG_EO_H__

#include <clCommonErrors.h>
#include <clHandleApi.h>
#include <clList.h>
#include <clIocApi.h>
#include <clMsgCommon.h>
#include <clMsgQueue.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClHandleDatabaseHandleT gMsgClientHandleDb;
extern ClOsalMutexT gClMsgFinalizeLock;
extern ClOsalCondT  gClMsgFinalizeCond;
extern ClInt32T gClMsgSvcRefCnt;

#ifdef __cplusplus
}
#endif

#endif
