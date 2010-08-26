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
 * ModuleName  : gms
 * File        : clGmsApiHandler.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the API functions declarations that are extended by the
 * server to the clients via the GMS client library.  They are all called with
 * unmarshalled data structures obtained from the ILD/RMD layer and they
 * all return with result data structures to that IDL/RMD layer which the
 * latter should marshall before sending to the client side.
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_API_HANDLER_H_
#define _CL_GMS_API_HANDLER_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clClmApi.h>
#include <clTmsApi.h>


extern ClRcT clGmsClientLibInitHandler( 
        CL_IN  const ClGmsClientInitRequestT*              const  req,
        CL_OUT       ClGmsClientInitResponseT*             const  res);

extern ClRcT clGmsClusterTrackHandler(
        CL_IN  const ClGmsClusterTrackRequestT*            const  req,
        CL_OUT       ClGmsClusterTrackResponseT*           const  res);

extern ClRcT clGmsClusterTrackStopHandler(
        CL_IN  const ClGmsClusterTrackStopRequestT*        const  req,
        CL_OUT       ClGmsClusterTrackStopResponseT*       const  res);

extern ClRcT clGmsClusterMemberGetHandler(
        CL_IN  const ClGmsClusterMemberGetRequestT*        const  req,
        CL_OUT       ClGmsClusterMemberGetResponseT*       const  res);

extern ClRcT clGmsClusterMemberGetAsyncHandler(
        CL_IN  const ClGmsClusterMemberGetAsyncRequestT*   const  req,
        CL_OUT       ClGmsClusterMemberGetAsyncResponseT*  const  res);
/*---------------------------------------------------------------------------*/
extern ClRcT clGmsGroupTrackHandler(
        CL_IN        ClGmsGroupTrackRequestT*                     req,
        CL_OUT       ClGmsGroupTrackResponseT*                    res);

extern ClRcT clGmsGroupTrackStopHandler(
        CL_IN        ClGmsGroupTrackStopRequestT*                 req,
        CL_OUT       ClGmsGroupTrackStopResponseT*                res);

extern ClRcT clGmsGroupMemberGetHandler(
        CL_IN        ClGmsGroupMemberGetRequestT*                 req,
        CL_OUT       ClGmsGroupMemberGetResponseT*                res);

extern ClRcT clGmsGroupMemberGetAsyncHandler(
        CL_IN        ClGmsGroupMemberGetAsyncRequestT*            req,
        CL_OUT       ClGmsGroupMemberGetAsyncResponseT*           res);
/*---------------------------------------------------------------------------*/
extern ClRcT clGmsClusterJoinHandler(
        CL_IN  const ClGmsClusterJoinRequestT*             const  req,
        CL_OUT       ClGmsClusterJoinResponseT*            const  res);

extern ClRcT clGmsClusterLeaveHandler(
        CL_IN  const ClGmsClusterLeaveRequestT*            const  req,
        CL_OUT       ClGmsClusterLeaveResponseT*           const  res);

extern ClRcT clGmsClusterLeaderElectHandler(
        CL_IN        ClGmsClusterLeaderElectRequestT*             req,
        CL_OUT       ClGmsClusterLeaderElectResponseT*            res);

extern ClRcT clGmsClusterMemberEjectHandler(
        CL_IN  const ClGmsClusterMemberEjectRequestT*      const  req,
        CL_OUT       ClGmsClusterMemberEjectResponseT*     const  res);
/*---------------------------------------------------------------------------*/
extern ClRcT clGmsGroupCreateHandler(
        CL_IN        ClGmsGroupCreateRequestT*                    req,
        CL_OUT       ClGmsGroupCreateResponseT*                   res);

extern ClRcT clGmsGroupDestroyHandler(
        CL_IN        ClGmsGroupDestroyRequestT*                   req,
        CL_OUT       ClGmsGroupDestroyResponseT*                  res);

extern ClRcT clGmsGroupJoinHandler(
        CL_IN        ClGmsGroupJoinRequestT*                      req,
        CL_OUT       ClGmsGroupJoinResponseT*                     res);

extern ClRcT clGmsGroupLeaveHandler(
        CL_IN        ClGmsGroupLeaveRequestT*                     req,
        CL_OUT       ClGmsGroupLeaveResponseT*                    res);

extern ClRcT clGmsGroupInfoListGetHandler(
        CL_IN        ClGmsGroupsInfoListGetRequestT*              req,
        CL_OUT       ClGmsGroupsInfoListGetResponseT*             res);

extern ClRcT clGmsGroupInfoGetHandler(
        CL_IN        ClGmsGroupInfoGetRequestT*                   req,
        CL_OUT       ClGmsGroupInfoGetResponseT*                  res);
extern ClRcT
clGmsGroupMcastHandler(
        CL_IN        ClGmsGroupMcastRequestT*                     req,
        CL_OUT       ClGmsGroupMcastResponseT*                    res);

extern ClRcT clGmsCompUpNotifyHandler(
        CL_IN        ClGmsCompUpNotifyRequestT*                   req,
        CL_OUT       ClGmsCompUpNotifyResponseT*                  res);

extern ClRcT clGmsGroupLeaderElectHandler(
        CL_IN        ClGmsGroupLeaderElectRequestT*               req,
        CL_OUT       ClGmsGroupLeaderElectResponseT*              res);

extern ClRcT clGmsGroupMemberEjectHandler(
        CL_IN        ClGmsGroupMemberEjectRequestT*               req,
        CL_OUT       ClGmsGroupMemberEjectResponseT*              res);

extern ClRcT _clGmsCallClusterMemberEjectCallBack(
        const        ClGmsMemberEjectReasonT                      reason);

void clGmsEventInit(void);
#define CL_GMS_VERIFY_CLIENT_VERSION(req , res )                             \
do {                                                                         \
    ClRcT _rc = CL_OK;                                                       \
    ClVersionT _version = {0};                                               \
    _version.releaseCode = req->clientVersion.releaseCode;                   \
    _version.majorVersion = req->clientVersion.majorVersion;                 \
    _version.minorVersion = 0x0;                                             \
                                                                             \
    _rc = clVersionVerify(                                                   \
            &gmsGlobalInfo.config.versionsSupported,                         \
            &_version                                                        \
            );                                                               \
    if(_rc != CL_OK ){                                                       \
        res->serverVersion.releaseCode = _version.releaseCode ;              \
        res->serverVersion.majorVersion= _version.majorVersion;              \
        res->serverVersion.minorVersion= _version.minorVersion;              \
        clLog (ERROR,GEN,NA,                                                 \
                 "GMS Client Server Version Mismatch detected");             \
        res->rc = CL_GMS_RC(CL_ERR_VERSION_MISMATCH);                        \
        return CL_GMS_RC(CL_ERR_VERSION_MISMATCH);                           \
    }                                                                        \
}while(0)



/* Cluster Member eject callback data */
typedef struct {
    ClIocAddressT address;
    ClGmsHandleT  handle;
}ClGmsEjectSubscribeT;


extern ClRcT
_clGmsEngineLeaderElect(
        ClGmsGroupIdT            groupid,
        ClGmsViewNodeT          *node ,
        ClGmsGroupChangesT       cond,
        ClGmsNodeIdT            *leaderNodeId ,
        ClGmsNodeIdT            *deputyNodeId );

extern int clGmsSendMsg(
        ClGmsViewMemberT        *memberNodeInfo,
        ClGmsGroupIdT            groupId, 
        ClGmsMessageTypeT        msgType,
        ClGmsMemberEjectReasonT  ejectReason,
        ClUint32T                dataSize,
        ClPtrT                   data);

extern ClBoolT  ringVersionCheckPassed;
extern ClVersionT ringVersion;
#ifdef  __cplusplus
}
#endif

#endif /* _CL_GMS_API_HANDLER_H_ */
