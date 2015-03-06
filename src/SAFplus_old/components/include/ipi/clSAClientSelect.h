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
 * ModuleName  : include
 * File        : clSAClientSelect.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_SA_CLIENT_SELECT_H_
#define _CL_SA_CLIENT_SELECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clOsalApi.h>
#include <clEoApi.h>

typedef struct {
    ClHandleT *cpmHandle;
    ClHandleT *evtHandle;
    ClHandleT *gmsHandle;
    ClHandleT *cpsHandle;
    ClHandleT *dlsHandle;
}ClSvcHandleT;

ClRcT clDispatchThreadCreate(ClEoExecutionObjT  *eoObj, 
        ClOsalTaskIdT *taskId,
        ClSvcHandleT  svcHandle);

#ifdef __cplusplus
}
#endif

#endif /* _CL_SA_CLIENT_SELECT_H_ */


