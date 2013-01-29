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
 * ModuleName  : gms
 * File        : clGmsApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
#ifndef _CL_GMS_API_H_
#define _CL_GMS_API_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <clOsalApi.h>
#include <clQueueApi.h>
#include <clClmApi.h>
#include <clTmsApi.h>
#include <clGmsErrors.h>
#include <clGmsCommon.h>

#define CL_GMS_SET_CLIENT_VERSION( req ) do { \
            req.clientVersion.releaseCode = 'B'; \
            req.clientVersion.minorVersion = 0x01; \
            req.clientVersion.majorVersion = 0x01; \
} while(0)

#define   NS_IN_MS                  1000000     /* nanosecs in 1 millisec */

typedef struct 
gms_instance {
    /* FIXME: need some field to store something that associates with server */
    ClGmsCallbacksT                 callbacks; /* To store user's view callback
                                                * functions */
    ClGmsClusterManageCallbacksT    cluster_manage_callbacks; 
    ClBoolT                         finalize; 
    ClGmsClusterNotificationBufferT cluster_notification_buffer; 
    ClGmsGroupNotificationBufferT   group_notification_buffer;
    ClOsalMutexIdT response_mutex;
    ClInt32T writeFd;
    ClInt32T readFd;
    ClQueueT cbQueue;
    ClDispatchFlagsT dispatchType;
    /**
     * The messageDelieryCallback is called when an ordered multicast
     * message is sent to the group to which this application has 
     * joined.
     */
    ClTmsGroupMessageDeliveryCallbackT  mcastMessageDeliveryCallback;
}ClGmsLibInstanceT;

extern ClHandleDatabaseHandleT handle_database;

extern ClRcT clGmsClusterTrackCallbackHandler (
        CL_IN   ClGmsClusterTrackCallbackDataT *res);

extern ClRcT clGmsClusterMemberGetCallbackHandler (
        ClGmsClusterMemberGetCallbackDataT *res);

extern ClRcT clGmsClusterMemberEjectCallbackHandler (
        CL_IN   ClGmsClusterMemberEjectCallbackDataT* const res);

extern ClRcT clGmsGroupMemberEjectCallbackHandler(
        ClGmsGroupMemberEjectCallbackDataT *res);

extern ClRcT clTmsGroupTrackCallbackHandler(
    CL_IN   ClGmsGroupTrackCallbackDataT *res);

extern ClRcT clTmsGroupMcastCallbackHandler(
        CL_IN ClGmsGroupMcastCallbackDataT *res);

extern ClRcT clGmsClientTableRegister(ClEoExecutionObjT *eo);
extern ClRcT clGmsClientTableDeregister(ClEoExecutionObjT *eo);
extern ClRcT clGmsClientRmdTableInstall(ClEoExecutionObjT *eo);
extern ClRcT clGmsClientClientTableRegistrer(ClEoExecutionObjT* eo);
extern ClRcT clGmsClientClientTableDeregistrer(ClEoExecutionObjT* eo);
extern ClRcT clGmsClientRmdTableUnInstall(ClEoExecutionObjT* eo);

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_GMS_API_H_ */
