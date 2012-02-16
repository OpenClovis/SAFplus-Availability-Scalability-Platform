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
#ifndef _CL_AMS_MGMT_HOOKS_H_
#define _CL_AMS_MGMT_HOOKS_H_

#include <clCommon.h>
#include <clAmsEntities.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ClAmsMgmtAdminOper
{
    CL_AMS_MGMT_ADMIN_OPER_UNLOCK,
    CL_AMS_MGMT_ADMIN_OPER_LOCKA,
    CL_AMS_MGMT_ADMIN_OPER_LOCKI,
    CL_AMS_MGMT_ADMIN_OPER_SHUTDOWN,
    CL_AMS_MGMT_ADMIN_OPER_RESTART,
    CL_AMS_MGMT_ADMIN_OPER_REPAIRED,
    CL_AMS_MGMT_ADMIN_OPER_SI_SWAP,
    CL_AMS_MGMT_ADMIN_OPER_MAX,
}ClAmsMgmtAdminOperT;

typedef struct ClAmsMgmtEntityAdminResponse
{
    ClHandleT mgmtHandle;
    ClHandleT clientHandle;
    ClAmsMgmtAdminOperT oper;
    ClAmsEntityTypeT type;
    ClUint32T retCode;
}ClAmsMgmtEntityAdminResponseT;

#if defined (CL_AMS_MGMT_HOOKS)

#define CL_AMS_MGMT_DEL_HOOK(entity,oper,retCode) clAmsMgmtHookDel((ClAmsEntityT*)(entity),oper,retCode)

extern 
ClRcT clAmsMgmtHookInitialize(void);

extern
ClRcT clAmsMgmtHookFinalize(void);

extern 
ClRcT clAmsMgmtHookDel(ClAmsEntityT *pEntity,ClAmsMgmtAdminOperT oper,ClRcT retCode);

#else

#define CL_AMS_MGMT_DEL_HOOK(entity,oper,retCode)

#endif

extern ClRcT
clAmsMgmtEntityForceLock(ClAmsMgmtHandleT amsHandle,
                         const ClAmsEntityT *entity);

extern ClRcT
clAmsMgmtEntityForceLockExtended(ClAmsMgmtHandleT amsHandle,
                                 const ClAmsEntityT *entity,
                                 ClUint32T lockFlags);

#ifdef __cplusplus
}
#endif

#endif
