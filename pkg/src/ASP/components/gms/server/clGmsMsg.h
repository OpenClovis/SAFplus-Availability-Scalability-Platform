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
 * File        : clGmsMsg.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Contains internal global data structures and function prototypes for the 
 * GMS Messaging.
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_MSG_H_
#define _CL_GMS_MSG_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clGmsCommon.h>
#include <clIocApi.h>


/* Commands passed in each message 
 */

typedef enum {

    CL_GMS_CLUSTER_JOIN_MSG    = 1,
    CL_GMS_CLUSTER_LEAVE_MSG   = 2,
    CL_GMS_CLUSTER_EJECT_MSG   = 3,
    CL_GMS_GROUP_CREATE_MSG    = 4,
    CL_GMS_GROUP_DESTROY_MSG   = 5,
    CL_GMS_GROUP_JOIN_MSG      = 6,
    CL_GMS_GROUP_LEAVE_MSG     = 7,
    CL_GMS_SYNC_MESSAGE        = 8,
    CL_GMS_COMP_DEATH          = 9,
    CL_GMS_LEADER_ELECT_MSG    = 10,
    CL_GMS_GROUP_MCAST_MSG     = 11
} ClGmsMessageTypeT;


/* Every message has to contain this basic source Info 
 */

typedef struct {

   ClUint64T         nodeId;
   ClNameT           nodeName;
   ClIocAddressT     nodeAddr;

} ClGmsMsgHeaderT;


typedef struct {

    ClUint64T   id;
    ClNameT     name;
#define     CL_GMS_LEADER_BIT           0x1
#define     CL_GMS_DEPUTY_BIT           0x2

/* If this bit is set then its a cluster */

#define     CL_GMS_CLUSTER_BIT          0x4
   ClUint8T          flags;  /* GMS flags can be one of the
                                   above #defines.  */


} ClGmsViewHeaderT;

/* GMS Node Join Message data 
 * A node when it joins a cluster, it sends out a join bcast
 * message.
 */

typedef struct {

    ClTimeT         bootTime;

} ClGmsJoinDataT;

/* Sent as a reply to a join call by the leader
 * to update the recently joined node with its
 * intial view number.
 */

typedef struct {

    ClUint64T       initialViewNumber;

} ClGmsJoinSyncDataT;

/* GMS Command data */

typedef union {

    ClGmsJoinDataT      join;
    //ClGmsLeaveDataT     gmsLeave;
    ClGmsJoinSyncDataT  joinSync;

} ClGmsMessageDataT;

/* GMS server peer messaging data structures */

typedef struct {
   
    ClGmsMsgHeaderT             header; 
    ClGmsViewHeaderT            viewHeader;
    ClGmsMessageTypeT           command;
    ClGmsMessageDataT           data;


} ClGmsSendMessageT;

/* Response from the server for the above message */

typedef struct {
    ClRcT   rc;
} ClGmsMessageResponseT;

#define CL_GMS_RMD_MAX_RETRIES      2
#define CL_GMS_RMD_TIME_OUT         300000

#define CL_GMS_BCAST_ADDRESS        0xffffffff

void _clGmsMessagePrint(ClGmsSendMessageT);

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_MSG_H_ */


