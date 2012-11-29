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

#ifndef _CL_AMS_MGMT_DEBUG_CLI_H_
#define _CL_AMS_MGMT_DEBUG_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clAmsTypes.h>
#include <clAmsEntities.h>
    
extern ClAmsMgmtHandleT gHandle;
extern ClAmsMgmtCCBHandleT gCcbHandle;

extern ClRcT clAmsDebugCliMgmtApi(ClUint32T argc,
                                  ClCharT **argv,
                                  ClCharT **ret);

extern ClRcT clAmsMgmtAdminStateChange(ClAmsEntityT *entity,
                                       ClAmsAdminStateT lastState,
                                       ClAmsAdminStateT newState);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_MGMT_DEBUG_CLI_H_ */
